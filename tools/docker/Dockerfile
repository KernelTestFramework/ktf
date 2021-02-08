FROM ubuntu:20.04

# build dependencies
RUN apt-get update -y
RUN apt-get install -y gcc make xorriso qemu-utils qemu qemu-system-x86 patch
# grub is a bit special in containers
RUN DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" install grub2 kmod python

CMD ["/bin/bash"]
