KTF_ROOT := $(abspath $(CURDIR))
export KTF_ROOT

CONFIG := $(KTF_ROOT)/.config
include $(CONFIG)

THIRD_PARTY := third-party
PATCH := patch
TOOLS_DIR := tools

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

PFMLIB_ARCHIVE :=
PFMLIB_LINKER_FLAGS :=
PFMLIB_INCLUDE :=
PFMLIB_NAME := libpfm
PFMLIB_VER := 4.10.1
PFMLIB_DIR := $(KTF_ROOT)/$(THIRD_PARTY)/$(PFMLIB_NAME)
PFMLIB_TARBALL := $(PFMLIB_DIR)/$(PFMLIB_NAME)-$(PFMLIB_VER).tar.gz
ifeq ($(CONFIG_LIBPFM),y)
KTF_PFMLIB_COMPILE := 1
export KTF_PFMLIB_COMPILE
TAR_CMD_PFMLIB := tar --exclude=.git --exclude=.gitignore --strip-components=1 -xf
PFMLIB_TOOLS_DIR := $(KTF_ROOT)/$(TOOLS_DIR)/$(PFMLIB_NAME)
PFMLIB_ARCHIVE := $(PFMLIB_DIR)/$(PFMLIB_NAME).a
PFMLIB_UNTAR_FILES := $(PFMLIB_NAME)-$(PFMLIB_VER)/lib
PFMLIB_UNTAR_FILES += $(PFMLIB_NAME)-$(PFMLIB_VER)/include
PFMLIB_UNTAR_FILES += $(PFMLIB_NAME)-$(PFMLIB_VER)/rules.mk
PFMLIB_UNTAR_FILES += $(PFMLIB_NAME)-$(PFMLIB_VER)/config.mk
PFMLIB_UNTAR_FILES += $(PFMLIB_NAME)-$(PFMLIB_VER)/Makefile
PFMLIB_UNTAR_FILES += $(PFMLIB_NAME)-$(PFMLIB_VER)/README
PFMLIB_UNTAR_FILES += $(PFMLIB_NAME)-$(PFMLIB_VER)/COPYING
PFMLIB_PATCH_FILE := $(PFMLIB_TOOLS_DIR)/libpfm_diff.patch
PFMLIB_LINKER_FLAGS += -L$(PFMLIB_DIR) -lpfm
PFMLIB_INCLUDE += $(PFMLIB_DIR)/include
endif

ACPICA_DEST_DIR := $(KTF_ROOT)/drivers/acpi/acpica
ifeq ($(CONFIG_ACPICA),y)
TAR_CMD_ACPICA := tar --exclude=.git --exclude=.gitignore --strip-components=1 -C $(ACPICA_DEST_DIR) -xf
ACPICA_VER := unix-20210730
ACPICA_NAME := acpica
ACPICA_DIR := $(KTF_ROOT)/$(THIRD_PARTY)/$(ACPICA_NAME)
ACPICA_TARBALL := $(ACPICA_DIR)/$(ACPICA_NAME)-$(ACPICA_VER).tar.gz
ACPICA_PATCH := $(ACPICA_DIR)/acpica_ktf.patch
ACPICA_UNTAR_DIRS := $(ACPICA_NAME)-$(ACPICA_VER)/source/components/dispatcher
ACPICA_UNTAR_DIRS += $(ACPICA_NAME)-$(ACPICA_VER)/source/components/events
ACPICA_UNTAR_DIRS += $(ACPICA_NAME)-$(ACPICA_VER)/source/components/executer
ACPICA_UNTAR_DIRS += $(ACPICA_NAME)-$(ACPICA_VER)/source/components/hardware
ACPICA_UNTAR_DIRS += $(ACPICA_NAME)-$(ACPICA_VER)/source/components/namespace
ACPICA_UNTAR_DIRS += $(ACPICA_NAME)-$(ACPICA_VER)/source/components/parser
ACPICA_UNTAR_DIRS += $(ACPICA_NAME)-$(ACPICA_VER)/source/components/resources
ACPICA_UNTAR_DIRS += $(ACPICA_NAME)-$(ACPICA_VER)/source/components/tables
ACPICA_UNTAR_DIRS += $(ACPICA_NAME)-$(ACPICA_VER)/source/components/utilities
ACPICA_UNTAR_DIRS += $(ACPICA_NAME)-$(ACPICA_VER)/source/include
ACPICA_INCLUDE := $(ACPICA_DEST_DIR)/source/include
endif

ifeq ($(CC),cc) # overwrite on default, otherwise use whatever is in the CC env variable
CC := gcc
endif
LD := ld

NM := nm
PYTHON := python
SHELL := bash
RM := rm
LN := ln
SYMLINK := $(LN) -s -f
HARDLINK := $(LN) -f
OBJCOPY := objcopy
STRIP := strip
ifeq ($(SYSTEM), MACOS)
STRIP_OPTS := -S
else
STRIP_OPTS := -s
endif

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

COMMON_INCLUDES := -I$(KTF_ROOT)/include -I$(KTF_ROOT)/include/arch/x86
ifeq ($(CONFIG_LIBPFM),y)
COMMON_INCLUDES += -I$(PFMLIB_INCLUDE)
endif
ifeq ($(CONFIG_ACPICA),y)
COMMON_INCLUDES += -I$(ACPICA_INCLUDE)
endif

COMMON_FLAGS := $(COMMON_INCLUDES) -pipe -MP -MMD -m64 -D__x86_64__ -D__KTF__ -DEARLY_VIRT_MEM=$(CONFIG_EARLY_VIRT_MEM)

ifeq ($(CONFIG_LIBPFM),y)
COMMON_FLAGS += -DKTF_PMU
endif

ifeq ($(CONFIG_ACPICA),y)
COMMON_FLAGS += -DKTF_ACPICA
endif

AFLAGS  := $(COMMON_FLAGS) -D__ASSEMBLY__ -nostdlib -nostdinc
CFLAGS  := $(COMMON_FLAGS) -std=gnu99 -O3 -g -Wall -Wextra -ffreestanding -nostdlib -nostdinc
CFLAGS  += -mno-red-zone -mno-mmx -mno-sse -mno-sse2
CFLAGS  += -fno-stack-protector -fno-exceptions -fno-builtin -fomit-frame-pointer -fcf-protection="none"
CFLAGS  += -mcmodel=kernel -fno-pic -fno-asynchronous-unwind-tables -fno-unwind-tables
CFLAGS  += -Wno-unused-parameter -Wno-address-of-packed-member
CFLAGS  += -Werror

ifneq ($(V), 1)
VERBOSE=@
endif

-include Makeconf.local

ifeq ($(CONFIG_ACPICA),y)
ACPICA_INSTALL := $(shell [ -d $(ACPICA_DEST_DIR)/source ] ||                         \
                          $(TAR_CMD_ACPICA) $(ACPICA_TARBALL) $(ACPICA_UNTAR_DIRS) && \
                          $(PATCH) -p0 < $(ACPICA_PATCH) &&                           \
                          $(HARDLINK) $(ACPICA_DEST_DIR)/acktf.h $(ACPICA_INCLUDE)/platform/acktf.h)
endif

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
TARGET_DEBUG := $(TARGET).debug

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
	$(VERBOSE) $(LD) -T $(PREP_LINK_SCRIPT) -o $@ $(OBJS) $(PFMLIB_LINKER_FLAGS)
	@echo "GEN " $(SYMBOLS_NAME).S
	$(VERBOSE) $(NM) -p --format=posix $(TARGET) | $(PYTHON) $(SYMBOLS_DIR)/$(SYMBOLS_TOOL)
	@echo "CC " $(SYMBOLS_NAME).S
	$(VERBOSE) $(CC) -c -o $(SYMBOLS_NAME).o $(AFLAGS) $(SYMBOLS_NAME).S
	$(VERBOSE) rm -rf $(SYMBOLS_NAME).S
	@echo "LD " $(TARGET) $(SYMBOLS_NAME).o
	$(VERBOSE) $(LD) -T $(PREP_LINK_SCRIPT) -o $@ $(OBJS) $(PFMLIB_LINKER_FLAGS) $(SYMBOLS_NAME).o
	@echo "STRIP"
	$(VERBOSE) $(OBJCOPY) --only-keep-debug $(TARGET) $(TARGET_DEBUG)
	$(VERBOSE) $(STRIP) $(STRIP_OPTS) $(TARGET)

$(PFMLIB_ARCHIVE): $(PFMLIB_TARBALL)
	@echo "UNTAR libpfm"
	$(VERBOSE) cd $(PFMLIB_DIR) &&\
	$(TAR_CMD_PFMLIB) $(PFMLIB_TARBALL) $(PFMLIB_UNTAR_FILES) -C ./ &&\
	$(PATCH) -s -p1 < $(PFMLIB_PATCH_FILE) &&\
	cd -
	@echo "BUILD libpfm"
	$(VERBOSE) $(MAKE) -C $(PFMLIB_DIR) lib
	$(VERBOSE) cp $(PFMLIB_DIR)/lib/$(PFMLIB_NAME).a $(PFMLIB_DIR)/
	$(VERBOSE) find $(PFMLIB_DIR) -name \*.c -delete

%.o: %.S
	@echo "AS " $@
	$(VERBOSE) $(CC) -c -o $@ $(AFLAGS) $<

%.o: %.c $(PFMLIB_ARCHIVE)
	@echo "CC " $@
	$(VERBOSE) $(CC) -c -o $@ $(CFLAGS) $<

DEPFILES := $(OBJS:.o=.d)
-include $(wildcard $(DEPFILES))

clean:
	@echo "CLEAN"
	$(VERBOSE) find $(KTF_ROOT) -name \*.d -delete
	$(VERBOSE) find $(KTF_ROOT) -name \*.o -delete
	$(VERBOSE) find $(KTF_ROOT) -name \*.lds -delete
	$(VERBOSE) find $(KTF_ROOT) -name \*.bin -delete
	$(VERBOSE) find $(KTF_ROOT) -name \*.iso -delete
	$(VERBOSE) find $(KTF_ROOT) -name \*.img -delete
	$(VERBOSE) find $(KTF_ROOT) -name cscope.\* -delete
	$(VERBOSE) find $(PFMLIB_DIR) -mindepth 1 ! -name $(PFMLIB_NAME)-$(PFMLIB_VER).tar.gz -delete
	$(VERBOSE) $(RM) -rf $(ACPICA_DEST_DIR)/source
	$(VERBOSE) $(RM) -f $(TARGET_DEBUG)

# Check whether we can use kvm for qemu
ifeq ($(SYSTEM),LINUX)
ifneq ($(USE_KVM), false) # you can hard-disable KVM use with the USE_KVM environment variable
HAVE_KVM=$(shell lsmod | awk '/^kvm / {print $$1}')
endif # USE_KVM
endif # SYSTEM == LINUX

# Set qemu parameters
ifeq ($(SYSTEM)$(HAVE_KVM),LINUXkvm)
QEMU_PARAMS := -cpu host
else
QEMU_PARAMS := -cpu max
endif
ifeq ($(HAVE_KVM), kvm)
QEMU_PARAMS += -enable-kvm
endif # HAVE_KVM
QEMU_PARAMS += -m 8192
QEMU_PARAMS += -display none -vga none -vnc none
QEMU_PARAMS += -serial stdio
QEMU_PARAMS += -smp cpus=2

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
	$(VERBOSE) $(QEMU_BIN) -cdrom $(ISO_FILE) $(QEMU_PARAMS)

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
	find $(KTF_ROOT) -name "*.[hcsS]"
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

DOCKERFILE  := $(shell find $(KTF_ROOT) -type f -name Dockerfile)
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
	$(VERBOSE) docker run -t $(DOCKERUSERFLAGS) -e UNITTEST=$(UNITTEST) -e CONFIG_LIBPFM=$(CONFIG_LIBPFM) -e CONFIG_ACPICA=$(CONFIG_ACPICA) -v $(PWD):$(PWD)$(DOCKER_MOUNT_OPTS) -w $(PWD) $(DOCKERIMAGE) bash -c "make -j $(strip $(subst :,, $*))"

.PHONY: onelinescan
onelinescan:
	@echo "scanning current working directory with one-line-scan"
	$(VERBOSE) docker run -t -e BASE_COMMIT=origin/mainline \
		-e REPORT_NEW_ONLY=true -e OVERRIDE_ANALYSIS_ERROR=true \
		-e INFER_ANALYSIS_EXTRA_ARGS="--bufferoverrun" \
		-e CPPCHECK_EXTRA_ARG=" --enable=style --enable=performance --enable=information --enable=portability" \
		-e VERBOSE=0 -e BUILD_COMMAND="make -B all" \
		-v $(PWD):$(PWD)$(DOCKER_MOUNT_OPTS) -w $(PWD) ktf-one-line-scan
