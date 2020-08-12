# KTF - Kernel Test Framework

KTF is a small and simple OS kernel, that enables writing low-level software tests for supported machine architectures (currently: x86-64).

### Features overview

* Machine architecture: `x86-64`, `x86-32` in the baking
* SMP support with basic Per-CPU pages
* (very) Basic Physical Memory Management (PMM) and Virtual Memory Management (VMM)
* Local APIC support
* Initial MP tables parsing
* Basic ACPI tables parsing
* Simple UART driver
* (very) Simple VGA driver

Some more features are in the making. Check out the issues.

### Build instructions

#### Requirements

You may need to install the following (unless you already have it):
* GRUB2 bootloader tools - `grub2-common` package (e.g. `apt install grub2-common`)
* ISO generation tools - `xorriso` package (e.g. `apt install xorriso`)

#### Kernel image build (for example to be used with QEMU)

* Native
```
make
```

* Docker

```
make docker:all
```

#### bootable ISO generation (for example to boot KTF as a guest under Xen or on a bare-metal machine)

* Native
```
make iso
```

* Docker

```
make docker:iso
```

The `make` command generates the `kernel64.bin` multiboot-compatible ELF file, that you can directly boot with QEMU.
The `make iso` command takes the `kernel64.bin`, places it in `grub/boot/` directory hierarchy and generates a `boot.iso`
out of the `grub/` (using `grub/boot/grub/grub.cfg` as a default GRUB config).

#### Fedora

KTF builds and runs on Fedora, but you will need to tweak some of the commands. Create a Makeconf.local file with the
following content (tested with Fedora 32):

```
GRUB_FILE := grub2-file
GRUB_MKIMAGE := grub2-mkimage
GRUB_MODULES += normal
QEMU_BIN := qemu-kvm
```

### Running the kernel

#### QEMU (KVM or not)

Main `Makefile` has several targets that make booting KTF with QEMU easier. The `Makefile` detects if the host system is linux and enables KVM support if so.
Default parameters for QEMU can be found in the `Makefile` under `QEMU_PARAMS` variable.

For booting run:
```
make boot
```

For debugging ISO run:
```
make boot_debug
```

For debugging kernel image run:
```
make debug
```

#### Xen guest

Use the following guest domain config example for booting KTF with Xen:
```
name="kernel64"
builder="hvm"
memory=1024

serial= [ 'file:/tmp/kernel.log', 'pty' ]

disk = [ '/home/user/boot.iso,,hdc,cdrom' ]

on_reboot = "destroy"

vcpus=1
```

You need to generate a bootable ISO for this.

## Style

The style for this project is defined in `.clang-format` file in the main directory of this repository.

Use the following command to apply the style automatically to the file you modify:

```
clang-format -style=file -Werror -i MODIFIED_FILE
```

For more information refer to: https://clang.llvm.org/docs/ClangFormat.html

## Credits and Attributions

* Parts of the KTF project are inspired by and based on XTF project [1] developed by Andrew Cooper of Citrix.

[1] http://xenbits.xenproject.org/docs/xtf/

## Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more information.

## License

This project is licensed under the BSD 2-Clause License.

