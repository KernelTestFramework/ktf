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

ifeq ($(CC),cc) # overwrite on default, otherwise use whatever is in the CC env variable
CC := gcc
endif
LD := ld

NM := nm
PYTHON := python
SHELL := bash

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
CFLAGS  := $(COMMON_FLAGS) -std=gnu99 -O3 -g -Wall -Wextra -ffreestanding -nostdlib -nostdinc
CFLAGS  += -Iinclude
CFLAGS  += -mno-red-zone -mno-mmx -mno-sse -mno-sse2
CFLAGS  += -fno-stack-protector -fno-exceptions -fno-builtin
CFLAGS  += -mcmodel=kernel -fno-pic -fno-asynchronous-unwind-tables -fno-unwind-tables
CFLAGS  += -Wno-unused-parameter -Wno-address-of-packed-member
CFLAGS  += -Werror

ifneq ($(V), 1)
VERBOSE=@
endif

-include Makeconf.local

SOURCES     := $(shell find . -name \*.c)
HEADERS     := $(shell find . -name \*.h)
ASM_SOURCES := $(shell find . -name \*.S)
LINK_SCRIPT := $(shell find . -name \*.ld)

SYMBOLS_NAME := symbols
SYMBOLS_TOOL := symbols.py
SYMBOLS_DIR  := tools/symbols

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

$(PREP_LINK_SCRIPT) : $(LINK_SCRIPT)
	$(VERBOSE) $(CC) $(AFLAGS) -E -P -C -x c $< -o $@

$(TARGET): $(OBJS) $(PREP_LINK_SCRIPT)
	@echo "LD " $@
	$(VERBOSE) $(LD) -T $(PREP_LINK_SCRIPT) -o $@ $^
	@echo "GEN " $(SYMBOLS_NAME).S
	$(VERBOSE) $(NM) -p --format=posix $(TARGET) | $(PYTHON) $(SYMBOLS_DIR)/$(SYMBOLS_TOOL)
	@echo "CC " $(SYMBOLS_NAME).S
	$(VERBOSE) $(CC) -c -o $(SYMBOLS_NAME).o $(AFLAGS) $(SYMBOLS_NAME).S
	$(VERBOSE) rm -rf $(SYMBOLS_NAME).S
	@echo "LD " $(TARGET) $(SYMBOLS_NAME).o
	$(VERBOSE) $(LD) -T $(PREP_LINK_SCRIPT) -o $@ $(OBJS) $(SYMBOLS_NAME).o

%.o: %.S
	@echo "AS " $@
	$(VERBOSE) $(CC) -c -o $@ $(AFLAGS) $<

%.o: %.c
	@echo "CC " $@
	$(VERBOSE) $(CC) -c -o $@ $(CFLAGS) $<

DEPFILES := $(OBJS:.o=.d)
-include $(wildcard $(DEPFILES))

clean:
	@echo "CLEAN"
	$(VERBOSE) find $(ROOT) -name \*.d -delete
	$(VERBOSE) find $(ROOT) -name \*.o -delete
	$(VERBOSE) find $(ROOT) -name \*.lds -delete
	$(VERBOSE) find $(ROOT) -name \*.bin -delete
	$(VERBOSE) find $(ROOT) -name \*.iso -delete
	$(VERBOSE) find $(ROOT) -name \*.img -delete
	$(VERBOSE) find $(ROOT) -name cscope.\* -delete

ifeq ($(SYSTEM),LINUX)
QEMU_PARAMS := -cpu host
else
QEMU_PARAMS := -cpu max
endif
QEMU_PARAMS += -m 8192
QEMU_PARAMS += -display none -vga none -vnc none
QEMU_PARAMS += -serial stdio
QEMU_PARAMS += -no-reboot -no-shutdown
QEMU_PARAMS += -smp cpus=2
ifeq ($(SYSTEM),LINUX)
ifneq ($(USE_KVM), false) # you can hard-disable KVM use with the USE_KVM environment variable
HAVE_KVM=$(shell lsmod | awk '/^kvm / {print $$1}')
ifeq ($(HAVE_KVM), kvm)
QEMU_PARAMS += -enable-kvm
endif # HAVE_KVM
endif # USE_KVM
endif # SYSTEM == LINUX

QEMU_PARAMS_KERNEL := -append "param1 param2 param3"
QEMU_PARAMS_DEBUG := -s &

ISO_FILE := boot.iso

ifneq ($(SYSTEM), LINUX)
$(ISO_FILE): dockerboot.iso
else
$(ISO_FILE): $(TARGET)
	@echo "GEN ISO" $(ISO_FILE)
	$(VERBOSE) $(GRUB_FILE) --is-x86-multiboot $(TARGET) || { echo "Multiboot not supported"; exit 1; }
	$(VERBOSE) cp $(TARGET) grub/boot/
	$(VERBOSE) $(GRUB_MKIMAGE) --format i386-pc-eltorito -c $(GRUB_CONFIG) -p /boot/grub -o grub/boot.img $(GRUB_MODULES)
	$(VERBOSE) $(XORRISO) -as mkisofs -U -b boot.img -no-emul-boot -boot-load-size 4 -boot-info-table -o $(ISO_FILE) grub 2>> /dev/null
endif

.PHONY: boot
boot: $(ISO_FILE)
	@echo "QEMU START"
	$(VERBOSE)$(QEMU_BIN) -cdrom $(ISO_FILE) $(QEMU_PARAMS)

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
	$(VERBOSE) $(all_sources) > cscope.files
	$(VERBOSE) cscope -b -q -k

.PHONY: style
style:
	@echo "STYLE"
	$(VERBOSE) docker run --rm --workdir /src -v $(PWD):/src$(DOCKER_MOUNT_OPTS) clang-format-lint --clang-format-executable /clang-format/clang-format10 \
          -r $(SOURCES) $(HEADERS) | grep -v -E '^Processing [0-9]* files:' | patch -s -p1 ||:

DOCKERFILE  := $(shell find $(ROOT) -type f -name Dockerfile)
DOCKERIMAGE := "ktf:build"
DOCKERUSERFLAGS := --user $(shell id -u):$(shell id -g) $(shell printf -- "--group-add=%q " $(shell id -G))

ifeq ($(SYSTEM), LINUX)
	DOCKER_BUILD_ARGS=--network=host
endif

.PHONY: dockerimage
dockerimage:
	@echo "Creating docker image"
	$(VERBOSE) docker build -t $(DOCKERIMAGE) -f $(DOCKERFILE) \
		$(DOCKER_BUILD_ARGS) .

.PHONY: docker%
docker%: dockerimage
	@echo "running target '$(strip $(subst :,, $*))' in docker"
	$(VERBOSE) docker run -t $(DOCKERUSERFLAGS) -e UNITTEST=$(UNITTEST) -v $(PWD):$(PWD)$(DOCKER_MOUNT_OPTS) -w $(PWD) $(DOCKERIMAGE) bash -c "make -j $(strip $(subst :,, $*))"

.PHONY: onelinescan
onelinescan:
	@echo "scanning current working directory with one-line-scan"
	$(VERBOSE) docker run -t -e BASE_COMMIT=origin/mainline \
		-e REPORT_NEW_ONLY=true -e OVERRIDE_ANALYSIS_ERROR=true \
		-e INFER_ANALYSIS_EXTRA_ARGS="--bufferoverrun" \
		-e CPPCHECK_EXTRA_ARG=" --enable=style --enable=performance --enable=information --enable=portability" \
		-e VERBOSE=0 -e BUILD_COMMAND="make -B all" \
		-v $(PWD):$(PWD)$(DOCKER_MOUNT_OPTS) -w $(PWD) ktf-one-line-scan