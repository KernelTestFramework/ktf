#ifndef SETUP_SETUP_H
#define SETUP_SETUP_H

#define KERN_STACK_SIZE (3 * PAGE_SIZE)

#ifndef __ASSEMBLY__
#include <page.h>

extern uint8_t kernel_stack[KERN_STACK_SIZE + 2 * PAGE_SIZE];
extern void arch_setup(void);

#define GET_KERN_STACK()    (&kernel_stack[KERN_STACK_SIZE])
#define GET_KERN_EX_STACK() (&kernel_stack[KERN_STACK_SIZE + PAGE_SIZE])
#define GET_KERN_EM_STACK() (&kernel_stack[KERN_STACK_SIZE + PAGE_SIZE])

extern unsigned long __start_text[], __end_text[];
extern unsigned long __start_data[], __end_data[];
extern unsigned long __start_bss[],  __end_bss[];
extern unsigned long __start_rodata[], __end_rodata[];

extern unsigned long __start_text_init[], __end_text_init[];
extern unsigned long __start_data_init[], __end_data_init[];
extern unsigned long __start_bss_init[], __end_bss_init[];

#endif /* __ASSEMBLY__ */

#endif /* SETUP_SETUP_H */
