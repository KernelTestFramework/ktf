# KTF - Kernel Test Framework

![GitHub release](https://img.shields.io/github/v/release/awslabs/ktf)
![C/C++ CI](https://github.com/awslabs/ktf/workflows/C/C++%20CI/badge.svg?branch=mainline)
![Docker Build Test](https://github.com/awslabs/ktf/workflows/Docker%20Build%20Test/badge.svg)
![test-clang-format](https://github.com/awslabs/ktf/workflows/test-clang-format/badge.svg?branch=mainline&event=push)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/awslabs/ktf.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/awslabs/ktf/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/awslabs/ktf.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/awslabs/ktf/context:cpp)

KTF is a small and simple OS kernel, that enables writing low-level software tests for supported machine architectures (currently: x86-64).

### Features overview

* Machine architecture: `x86-64`, `x86-32` in the baking
* SMP support with basic Per-CPU pages
* (very) Basic Physical Memory Management (PMM) and Virtual Memory Management (VMM)
* Very basic slab allocator
* Local APIC support
* Initial MP tables parsing
* Basic ACPI tables parsing
* Simple UART driver
* (very) Simple VGA driver

Some more features are in the making. Check out the issues.

## Getting Started

The quick start is to run the following commands. The content of the file tests/test.c
will be executed. Note, the last command will block.

Build a boot.iso from scratch, in the docker container.

```
make clean
make docker:boot.iso
```

Boot the resulting image:

```
make boot
```

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
make boot.iso
```

* Docker

```
make docker:boot.iso
```

The `make` command generates the `kernel64.bin` multiboot-compatible ELF file, that you can directly boot with QEMU.
The `make boot.iso` command takes the `kernel64.bin`, places it in `grub/boot/` directory hierarchy and generates a `boot.iso`
out of the `grub/` (using `grub/boot/grub/grub.cfg` as a default GRUB config).

#### Fedora

KTF builds and runs on Fedora, but you will need to tweak some of the commands. Create a Makeconf.local file with the
following content (tested with Fedora 32):

```
DIST=$(shell grep NAME= /etc/os-release | cut -d= -f2)
ifeq ($(DIST),Fedora)
GRUB_FILE := grub2-file
GRUB_MKIMAGE := grub2-mkimage
GRUB_MODULES += normal
QEMU_BIN := qemu-kvm
DOCKER_MOUNT_OPTS := :Z
endif
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
```python
name="kernel64"
builder="hvm"
memory=1024

serial= [ 'file:/tmp/kernel.log', 'pty' ]

disk = [ '/home/user/boot.iso,,hdc,cdrom' ]

on_reboot = "destroy"

vcpus=1
```

You need to generate a bootable ISO for this.

### Adding new tests

New tests can be added by adding a new function in a file in the `tests` folder. Each test signature must
be the same as `test_fn` provided in `test.h`. Tests can be enabled in `grub.cfg` by adding the option with key `tests` and values
the comma-separated list of function names, such as `tests=test1,test2,unit_tests`.

## Style

The style for this project is defined in `.clang-format` file in the main directory of this repository.

Use the following command to apply the style automatically to the file you modify:

```bash
clang-format -style=file -Werror -i MODIFIED_FILE
```

For more information refer to: https://clang.llvm.org/docs/ClangFormat.html

This project uses https://github.com/DoozyX/clang-format-lint-action action workflow to detect style mismatches automatically.
For more information refer to: https://github.com/marketplace/actions/clang-format-lint

### Running the clang-format workflow locally

#### Build `clang-format-lint` container

```bash
docker build -t clang-format-lint github.com/DoozyX/clang-format-lint-action
```

This has to be done only once.

#### Patch your files

```bash
make style
```

### Running the one-line-scan workflow locally

#### Build the `ktf-one-line-scan` container

* first, download the `one-line-scan` base image
```bash
docker build -t one-line-scan \
    https://raw.githubusercontent.com/awslabs/one-line-scan/master/tools/Dockerfile
```

* create the KTF specific one-line-scan image, including necessary tools for building
```
 docker build -t ktf-one-line-scan \
    --build-arg USER_ID=$(id -u) \
    --build-arg GROUP_ID=$(id -g) \
    --build-arg USER=$USER \
    --file tools/docker/OnelineScanDockerfile .
```

This has to be done only once.

#### Scan your current working directory

* per default, one-line-scan scans against the diff between `HEAD` and `origin/mainline` commit

```bash
make onelinescan
```

## Credits and Attributions

* Parts of the KTF project are inspired by and based on XTF project [1] developed by Andrew Cooper of Citrix.

[1] http://xenbits.xenproject.org/docs/xtf/

## Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more information.

## License

![GitHub](https://img.shields.io/github/license/awslabs/ktf)

This project is licensed under the BSD 2-Clause License.

