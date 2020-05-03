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
	rm -f *.d *.o *.bin
