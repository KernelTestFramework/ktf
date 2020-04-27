#!/bin/sh

sudo setfacl -m u:${USER}:rw /dev/kvm
taskset -c 2 qemu-system-x86_64 \
	-display none \
	-vga none \
	-debugcon stdio \
	-kernel $1 \
	-no-reboot \
	-enable-kvm \
	-serial file:/dev/stdout \
	-append "$*"
