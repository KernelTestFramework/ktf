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
LD := ld

GRUB_FILE := grub-file
GRUB_MKIMAGE := grub-mkimage
GRUB_MODULES := multiboot iso9660 biosdisk
ifneq ($(UNITTEST),)
GRUB_CONFIG := grub/boot/grub/grub-test.cfg
else
GRUB_CONFIG := grub/boot/grub/grub.cfg
endif
XORRISO := xorriso
QEMU_BIN := qemu-system-x86_64
GDB := gdb

COMMON_FLAGS := -I$(ROOT)/include -I$(ROOT)/include/arch/x86 -pipe -MP -MMD -m64 -D__x86_64__
ifneq ($(UNITTEST),)
COMMON_FLAGS += -DKTF_UNIT_TEST
endif

AFLAGS  := $(COMMON_FLAGS) -D__ASSEMBLY__ -nostdlib -nostdinc
CFLAGS  := $(COMMON_FLAGS) -std=gnu99 -O3 -g -Wall -Wextra -ffreestanding
CFLAGS  += -mno-red-zone -mno-mmx -mno-sse -mno-sse2
CFLAGS  += -fno-stack-protector -fno-exceptions -fno-builtin
CFLAGS  += -mcmodel=kernel -fno-pic -fno-asynchronous-unwind-tables -fno-unwind-tables
CFLAGS  += -Wno-unused-parameter -Wno-address-of-packed-member
CFLAGS  += -Werror

-include Makeconf.local

SOURCES     := $(shell find . -name \*.c)
HEADERS     := $(shell find . -name \*.h)
ASM_SOURCES := $(shell find . -name \*.S)
LINK_SCRIPT := $(shell find . -name \*.ld)

PREP_LINK_SCRIPT := $(LINK_SCRIPT:%.ld=%.lds)

OBJS := $(SOURCES:%.c=%.o)
OBJS += $(ASM_SOURCES:%.S=%.o)

TARGET := kernel64.bin

# On Linux systems, we build directly. On non-Linux, we rely on the 'docker%'
# rule below to create an Ubuntu container and perform the Linux-specific build
# steps therein.
ifeq ($(SYSTEM), LINUX)
all: $(TARGET)
else
all: docker$(TARGET)
endif

$(TARGET): $(OBJS)
	@echo "LD " $@
	@ $(CC) $(AFLAGS) -E -P -C -x c $(LINK_SCRIPT) -o $(PREP_LINK_SCRIPT)
	@ $(LD) -T $(PREP_LINK_SCRIPT) -o $@ $^

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

ifneq ($(SYSTEM), LINUX)
$(ISO_FILE): dockerboot.iso
else
$(ISO_FILE): all
	@echo "GEN ISO" $(ISO_FILE)
	@ $(GRUB_FILE) --is-x86-multiboot $(TARGET) || { echo "Multiboot not supported"; exit 1; }
	@ cp $(TARGET) grub/boot/
	@ $(GRUB_MKIMAGE) --format i386-pc-eltorito -c $(GRUB_CONFIG) -p /boot/grub -o grub/boot.img $(GRUB_MODULES)
	@ $(XORRISO) -as mkisofs -U -b boot.img -no-emul-boot -boot-load-size 4 -boot-info-table -o $(ISO_FILE) grub 2>> /dev/null
endif

.PHONY: boot
boot: $(ISO_FILE)
	@echo "QEMU START"
	@$(QEMU_BIN) -cdrom $(ISO_FILE) $(QEMU_PARAMS)

.PHONY: boot_debug
boot_debug: $(ISO_FILE)
	$(QEMU_BIN) -cdrom $(ISO_FILE) $(QEMU_PARAMS) $(QEMU_PARAMS_DEBUG)

.PHONY: run
run: $(TARGET)
	$(QEMU_BIN) -kernel $(TARGET) $(QEMU_PARAMS) $(QEMU_PARAMS_KERNEL)

.PHONY: debug
debug: $(TARGET)
	$(QEMU_BIN) -kernel $(TARGET) $(QEMU_PARAMS) $(QEMU_PARAMS_KERNEL) $(QEMU_PARAMS_DEBUG)

.PHONY: gdb
gdb: debug
	$(GDB) $(TARGET) -ex 'target remote :1234' -ex 'b _start' -ex 'c'
	killall -9 $(QEMU_BIN)

define all_sources
	find $(ROOT) -name "*.[hcsS]"
endef

.PHONY: cscope
cscope:
	@echo "CSCOPE"
	@ $(all_sources) > cscope.files
	@ cscope -b -q -k

.PHONY: style
style:
	@echo "STYLE"
	@ docker run --rm --workdir /src -v $(PWD):/src clang-format-lint --clang-format-executable /clang-format/clang-format10 \
          -r $(SOURCES) $(HEADERS) | grep -v -E '^Processing [0-9]* files:' | patch -s -p1 ||:

DOCKERFILE  := $(shell find $(ROOT) -type f -name Dockerfile)
DOCKERIMAGE := "ktf:build"

.PHONY: dockerimage
dockerimage:
	@echo "Creating docker image"
	@ docker build -t $(DOCKERIMAGE) -f $(DOCKERFILE) .

.PHONY: docker%
docker%: dockerimage
	@echo "running target '$(strip $(subst :,, $*))' in docker"
	@ docker run -it -e UNITTEST=$(UNITTEST) -v $(PWD):$(PWD) -w $(PWD) $(DOCKERIMAGE) bash -c "make -j $(strip $(subst :,, $*))"
