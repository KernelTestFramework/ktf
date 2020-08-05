/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <asm-macros.h>
#include <processor.h>
/*#include <desc.h>*/
#include <segment.h>


/*
* All code and data segment in this module have to be identity mapped meaning linear addresses have to be flat segments with segment base  = 0.
* This also means that virtual address = physical address(after translation)
*/

#define	PAGE_SIZE 0x1000
#define	REAL_MODE_DANCE_STACK (PAGE_SIZE*3)


#define TRAMP_GDT_NULL		0
#define TRAMP_GDT_CS32		1
#define TRAMP_GDT_DS32		2
#define	TRAMP_GDT_CS64		3
#define TRAMP_GDT_REALM_CS	4
#define	TRAMP_GDT_REALM_DS	5

#define __KERN_TRAMP_CS32	(TRAMP_GDT_CS32 << 3)
#define __KERN_TRAMP_DS32	(TRAMP_GDT_DS32 << 3)
#define	__REALM_TRAMP_CS16	(TRAMP_GDT_REALM_CS << 3)
#define	__REALM_TRAMP_DS16	(TRAMP_GDT_REALM_DS << 3)
#define	__KERN_TRAMP_CS64	(TRAMP_GDT_CS64 << 3)

/* Get confirmation from Pawel, where we want the real mode base to be eventually for our piece of code */
#define	REAL_MODE_SEG_BASE	0x0

#define LJMPW_RM(to)    .byte 0xea ; .word (to), REAL_MODE_SEG_BASE

#define LJMPW_IN_32BIT(to)    .byte 0x66, 0xea ; .word (to), __REALM_TRAMP_CS16

.code64
SECTION(.real_prot_tramp, "ax", 16)
/*
clear interrupt, clear caches
save gprs
save cr's and MSRs ; cr0, cr3, gdtr, ldtr, idtr, efer, rsp, tr
*/
GLOBAL(_long_to_real)
	/* Disable interrupts. If we get NMI or MC, hope we get back here */
	/* Once we've Interrupt enabled and APIC handling, we would want below code to be interrupt safe */
	#cli
	/* clean up caches */
	wbinvd				
	/* save registers */
	SAVE_REGS
	/* push flags*/
	pushfq				
	/* save away protected mode gdt in low memory area*/
	sgdt	prot_gdt
	/* save away protected mode idt*/
	sidt	prot_idt		
	/* save away protected mode cr0 value */
	mov	%cr0, %rax
	mov	%rax, prot_cr0
	/* save away cr3*/
	mov	%cr3, %rax	
	mov	%rax, prot_cr3
	/* save away cr4*/
	mov	%cr4, %rax	
	mov	%rax, prot_cr4
	/* save efer msr */
	xor	%rax, %rax
	xor	%rdx, %rdx
	movl	$MSR_EFER, %ecx
	rdmsr
	shl	$32, %rdx
	or	%rdx, %rax
	mov	%rax, efer_msr

	str	task_reg
	
	/* save cs */
	movw	%cs, %ax	
	movw	%ax, seg_cs

	/* save ds */
	movw	%ds, %ax	
	movw	%ax, seg_ds

	/* save es */
	movw	%es, %ax	
	movw	%ax, seg_es

	/* save fs */
	movw	%fs, %ax	
	movw	%ax, seg_fs

	/* save gs */
	movw	%gs, %ax	
	movw	%ax, seg_gs

	/* save ss */
	movw	%ss, %ax	
	movw	%ax, seg_ss

	/* save rsp */
	mov	%rsp, %rax
	mov	%rax, prot_rsp

	/* This is a long indirect jump to long_to_prot32bit_code. From long mode to 32bit mode, indirect form is the only way*/
	ljmpl	*(indir_jmp_long_2_real)

.code32
long_to_prot32bit_code:

	movl	$__KERN_TRAMP_DS32, %eax
	movl	%eax, %ds

	/* everything is saved, let's load up trampoline gdt*/
	lgdt	trampgdt

	/* Disable paging first to get rid of long mode*/
	mov	%cr0, %eax
	and	$~X86_CR0_PG, %eax
	mov	%eax, %cr0


	/* We can probably delay this step as well for later, for now keeping here*/
	/* There is not IDT because there is not BIOS, we're just loading fake entry here*/
	lidt	realidt
	/* Disable LME in EFER */
	movl	$MSR_EFER, %ecx
	rdmsr
	and	$~EFER_LME, %eax
	wrmsr

	
	/* load stack pointer for real mode */
	mov	rm_stack_high, %esp	
	/* Load data seg registers with 16bit compat values, be sure not to do any data references before doing the final 16bit code jump*/
	movl	$__REALM_TRAMP_DS16, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %gs
	movl	%eax, %fs
	movl	%eax, %ss


	ljmpw	$__REALM_TRAMP_CS16, $.L16bit_code_start

label_32bit_code_end:

.code16
	.balign 16
.L16bit_code_start:

        xorl    %ecx, %ecx
        movl    %cr0, %edx
        andl    $(X86_CR0_PE | X86_CR0_ET), %edx
        orl     $(X86_CR0_CD | X86_CR0_NW), %edx
        movl    %edx, %cr0
        movl    %ecx, %cr3
        movl    %cr0, %edx
        testl   $(X86_CR0_CD | X86_CR0_NW), %edx       /* If no cache bits -> no wbinvd */
        jz      1f
        wbinvd
1:
        andb    $(X86_CR0_ET) , %dl
        movl    %edx, %cr0
        LJMPW_RM(2f)
2:
	mov	$0, %eax
	mov	%eax, %ds
	/* Don't enable interrupts here, IDT is all NULL and things will crash
	Not sure if we want to re-enabl interrupts here; INT xx should still work. Check with wipawel@ */

	/* Eventually we'll do callbacks in C routines compiled for 16bit from here or other ASM routines compiled for 16bit */
	/* But for now, we just go back to 32bit protected mode and then to long mode and return to our caller */

	/* Let's start unwinding back to long mode, Below ret is unreachable, placing a int 3 before that*/
	jmp _real_to_prot

	.byte 0xcc
	ret
	
GLOBAL(_real_to_prot)
	.balign 16	

	#cli
	wbinvd
	LJMPW_RM(4f)
4:
	/* No one should have clobbered on IDTR and GDTR but still let's load it up */
        #lidt	realidt 
        #lgdt	trampgdt
	lgdt	prot_gdt
	lidt	prot_idt
        movw	$__KERN_DS32, %dx       # Data segment descriptor

        # Enable protected mode
	# reached here
        movl	$X86_CR0_PE, %eax       # protected mode (PE) bit
        movl	%eax, %cr0              # into protected mode

        # flush prefetch and jump to 32bit code
        ljmp	$__KERN_CS32, $.Lcr0_pe_on 
label_16bitcode_end:

.code32
	.balign 16

.Lcr0_pe_on:
	movl	%edx, %ss
	movl	%edx, %ds
	movl	%edx, %es
	movl	%edx, %fs
	movl	%edx, %gs

	movl	prot_cr4, %eax
	movl	%eax, %cr4              # Enable PAE mode

	# Setup trampoline 4 level pagetables
	movl	prot_cr3, %eax
	movl	%eax, %cr3

	# Set up EFER
	movl	efer_msr, %eax
	or	$EFER_LME, %eax
	movl	efer_msr + 4, %edx
	movl	$MSR_EFER, %ecx
	wrmsr
	# Enable paging and in turn activate Long Mode
	movl	$(X86_CR0_PG | X86_CR0_WP | X86_CR0_PE), %eax
	movl	%eax, %cr0

	/*
	 * At this point we're in long mode but in 32bit compatibility mode
	 * with EFER.LME = 1, CS.L = 0, CS.D = 1 (and in turn
	 * EFER.LMA = 1). Now we want to jump in 64bit mode, to do that we use
	 * the new gdt/idt that has __KERNEL_CS with CS.L = 1.
	 */

	#ljmpl	$__KERN_TRAMP_CS64, $.Llong_mode_on
	ljmpl	$__KERN_CS64, $.Llong_mode_on

.Llong_mode_on:
	wbinvd
	
long_mode_orig_cs:
.code64

	movq	%cr0, %rax
	orq	prot_cr0, %rax
	mov	%rax, %cr0


	/* reload ds */
	movw	seg_ds, %ax	
	movw	%ax, %ds

	/* reload es */
	movw	seg_es, %ax	
	movw	%ax, %es

	/* reload fs */
	movw	seg_fs, %ax	
	movw	%ax, %fs

	/* reload gs */
	movw	seg_gs, %ax	
	movw	%ax, %gs

	/* reload ss */
	movw	seg_ss, %ax	
	movw	%ax, %ss

	/* reload rsp */
	mov	prot_rsp, %rax
	mov	%rax, %rsp
	
	popfq
	RESTORE_REGS
	#sti

	ret


SECTION(.rlm_save_area, "aw", PAGE_SIZE)

rm_stack_low:
	.skip REAL_MODE_DANCE_STACK
rm_stack_high:

	.balign 16
prot_save_area_low:

prot_gdt:
	.word 0
	.quad 0

prot_idt:
	.word 0
	.quad 0

	.balign 16
prot_cr0:
	.quad 0

prot_cr3:
	.quad 0

prot_cr4:
	.quad 0

efer_msr:
	.quad 0

task_reg:
	.word 0
seg_cs:
	.word 0
seg_ds:
	.word 0
seg_es:
	.word 0
seg_fs:
	.word 0
seg_gs:
	.word 0
seg_ss:
	.word 0

prot_rsp:
	.quad 0

prot_save_area_high:

        .balign 16
GLOBAL(machine_tramp_gdt)
	/* Null selector */
	# .quad	GDT_ENTRY(0x0, 0, 0x0)
	
	.word	0, 0
	.byte	0, 0, 0, 0
	

	/* code segment, limit 0xFFFFF (4KiB granularity), present */
	/* type = 32bit code exec/read, DPL = 0 */
	#.quad	GDT_ENTRY((DESC_FLAGS(GR, SZ, P, DPL0, S, CODE, R, A)), 0x0, 0xfffff),
	.quad	GDT_ENTRY(0xcf9b, 0, 0xfffff)

	/* data segment, limit 0xFFFFF (4KiB granularity), present */
	/* type = 32bit data exec/read, DPL = 0 */
	# .quad	GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, DATA, W, A), 0x0, 0xfffff),
	.quad	GDT_ENTRY(0xcf93, 0, 0xfffff)

	/* 64bit code segment, flat segment with base 0*/
	.quad	GDT_ENTRY(0xa09b, 0, 0x00000)

	/* 16bit real mode code segment, limit 0xFFFFF (1B granularity), present */
	/* type = 16bit code exec/read only/confirming, DPL = 0 */
	# .quad	GDT_ENTRY(0x9b, _long_to_real, 0xffff)
	.word	0xffff
	.long	0x9b000000  # + _long_to_real
	.word	0

	/* 16bit real mode data segment, limit 0xFFFFF (1B granularity), present */
	/* type = 16bit data read/write, DPL = 0 */
	# .quad	GDT_ENTRY(0x93, rm_stack_low, 0xffff)
	.word	0xffff
	.long	0x93000000 #+ rm_stack_low
	.word	0
	
	
end_machine_tramp_gdt:


trampgdt:
	.word end_machine_tramp_gdt - machine_tramp_gdt - 1
	.quad machine_tramp_gdt
realidt:
	.word 0x400
	.quad 0

indir_jmp_long_2_real:
	.long long_to_prot32bit_code
	.word __KERN_TRAMP_CS32


indir_jmp_long_2_long:
	.long long_mode_orig_cs
	.word __KERN_CS64
