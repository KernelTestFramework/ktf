ROOT := $(abspath $(CURDIR))
export ROOT

ifeq ($(OS),Windows_NT)
SYSTEM := WIN
else
UNAME := $(shell uname -s)

ifeq ($(UNAME),Linux)
SYSTEM := LINUX
endif

ifeq ($(UNAME),Darwin)
SYSTEM := MACOS
endif
endif

CC := gcc

COMMON_FLAGS := -I$(ROOT)/include -pipe -MP -MMD -m64 -D__x86_64__

AFLAGS  := $(COMMON_FLAGS) -D__ASSEMBLY__ -nostdlib -nostdinc
CFLAGS  := $(COMMON_FLAGS) -std=gnu99 -O3 -g -Wall -ffreestanding
CFLAGS  += -mno-red-zone -mno-mmx -mno-sse -mno-sse2
CFLAGS  += -fno-stack-protector -fno-exceptions -fno-builtin
CFLAGS  += -mcmodel=kernel -fno-pic -fno-asynchronous-unwind-tables -fno-unwind-tables

SOURCES     := $(shell find . -name \*.c)
ASM_SOURCES := $(shell find . -name \*.S)
LINK_SCRIPT := $(shell find . -name \*.ld)

PREP_LINK_SCRIPT := $(LINK_SCRIPT:%.ld=%.lds)

OBJS := $(SOURCES:%.c=%.o)
OBJS += $(ASM_SOURCES:%.S=%.o)

TARGET := kernel64.bin

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "LD " $@
	@ $(CC) $(AFLAGS) -E -P -C -x c $(LINK_SCRIPT) -o $(PREP_LINK_SCRIPT)
	@ ld -T $(PREP_LINK_SCRIPT) -o $@ $^

%.o: %.S
	@echo "AS " $@
	@ $(CC) -c -o $@ $(AFLAGS) $<

%.o: %.c
	@echo "CC " $@
	@ $(CC) -c -o $@ $(CFLAGS) $<

clean:
	@echo "CLEAN"
	@ find $(ROOT) -name \*.d -delete
	@ find $(ROOT) -name \*.o -delete
	@ find $(ROOT) -name \*.lds -delete
	@ find $(ROOT) -name \*.bin -delete
	@ find $(ROOT) -name \*.iso -delete
	@ find $(ROOT) -name \*.img -delete
	@ find $(ROOT) -name cscope.\* -delete

ifeq ($(SYSTEM),LINUX)
QEMU_PARAMS := -cpu host
else
QEMU_PARAMS := -cpu max
endif
QEMU_PARAMS += -m 8192
QEMU_PARAMS += -display none -vga none -vnc none
QEMU_PARAMS += -serial stdio
QEMU_PARAMS += -no-reboot -no-shutdown
QEMU_PARAMS += -smp cpus=4
ifeq ($(SYSTEM),LINUX)
QEMU_PARAMS += -enable-kvm
endif

QEMU_PARAMS_KERNEL := -append "param1 param2 param3"
QEMU_PARAMS_DEBUG := -s &

ISO_FILE := boot.iso

.PHONY: iso
iso: all
	@echo "GEN ISO" $(ISO_FILE)
	@ grub-file --is-x86-multiboot $(TARGET) || { echo "Multiboot not supported"; exit 1; }
	@ cp $(TARGET) grub/boot/
	@ grub-mkimage --format i386-pc-eltorito -p /boot/grub -o grub/boot.img multiboot iso9660 biosdisk
	@ xorriso -as mkisofs -U -b boot.img -no-emul-boot -boot-load-size 4 -boot-info-table -o $(ISO_FILE) grub 2>> /dev/null

.PHONY: boot
boot: all
	@echo "QEMU START"
	@qemu-system-x86_64 -cdrom $(ISO_FILE) $(QEMU_PARAMS)

.PHONY: boot_debug
boot_debug: all iso
	qemu-system-x86_64 -cdrom $(ISO_FILE) $(QEMU_PARAMS) $(QEMU_PARAMS_DEBUG)

.PHONY: run
run: all
	qemu-system-x86_64 -kernel $(TARGET) $(QEMU_PARAMS) $(QEMU_PARAMS_KERNEL)

.PHONY: debug
debug: all
	qemu-system-x86_64 -kernel $(TARGET) $(QEMU_PARAMS) $(QEMU_PARAMS_KERNEL) $(QEMU_PARAMS_DEBUG)

.PHONY: gdb
gdb: debug
	gdb $(TARGET) -ex 'target remote :1234' -ex 'b _start' -ex 'c'
	killall -9 qemu-system-x86_64

define all_sources
	find $(ROOT) -name "*.[hcsS]"
endef

.PHONY: cscope
cscope:
	@echo "CSCOPE"
	@ $(all_sources) > cscope.files
	@ cscope -b -q -k

DOCKERFILE  := $(shell find $(ROOT) -type f -name Dockerfile)
DOCKERIMAGE := "ktf:build"

.PHONY: dockerimage
dockerimage:
	@echo "Creating docker image"
	@ docker build -t $(DOCKERIMAGE) -f $(DOCKERFILE) .

.PHONY: docker%
docker%: dockerimage
	@echo "running target '$(strip $(subst :,, $*))' in docker"
	@ docker run -it -v $(PWD):$(PWD) -w $(PWD) $(DOCKERIMAGE) bash -c "make -j $(strip $(subst :,, $*))"
