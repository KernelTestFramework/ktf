ROOT := $(abspath $(CURDIR))
export ROOT

CC := gcc

COMMON_FLAGS := -I$(ROOT)/include -pipe -MP -MMD

AFLAGS  := $(COMMON_FLAGS) -D__ASSEMBLY__ -nostdlib -nostdinc
CFLAGS  := $(COMMON_FLAGS) -std=gnu99 -O -g -Wall -ffreestanding
CFLAGS  += -mno-red-zone -mno-mmx -mno-sse -mno-sse
CFLAGS  += -fno-stack-protector -fno-exceptions -fno-builtin
CFLAGS  += -fno-asynchronous-unwind-tables -fno-unwind-tables

LDFLAGS := -nostdlib -lgcc

SOURCES := $(wildcard *.c)
ASM_SOURCES := $(wildcard *.S)
LINK_SCRIPT := $(wildcard *.ld)

OBJS := $(SOURCES:%.c=%.o)
OBJS += $(ASM_SOURCES:%.S=%.o)

all: kernel32.bin

kernel32.bin: kernel64.bin
	objcopy -O elf32-i386 $< $@

kernel64.bin: $(OBJS)
	ld -T $(LINK_SCRIPT) -o $@ $^

%.o: %.S
	$(CC) -c -o $@ $(AFLAGS) $<

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $<

clean:
	rm -f *.d *.o *.bin cscope.*

QEMU_PARAMS := -machine q35,accel=kvm -m 1024
QEMU_PARAMS += -display none -vga none -vnc none
QEMU_PARAMS += -debugcon stdio -serial file:/dev/stdout
QEMU_PARAMS += -no-reboot -no-shutdown
QEMU_PARAMS += -enable-kvm
QEMU_PARAMS += -append "param1 param2 param3"

.PHONY: run
run: all
	sudo "$$QEMU_PATH"/qemu-system-x86_64 -kernel kernel64.bin $(QEMU_PARAMS)

.PHONY: debug
debug: all
	sudo "$$QEMU_PATH"/qemu-system-x86_64 -kernel kernel64.bin $(QEMU_PARAMS) -S -s &

.PHONY: gdb
gdb: debug
	gdb kernel64.bin -ex 'target remote :1234' -ex 'b _start' -ex 'c'
	sudo killall -9 qemu-system-x86_64

define all_sources
	find $(ROOT) -name "*.[hcsS]"
endef

.PHONY: cscope
cscope:
	$(all_sources) > cscope.files
	cscope -b -q -k
