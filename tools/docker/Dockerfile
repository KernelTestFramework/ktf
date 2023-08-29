FROM ubuntu:latest

# build dependencies
RUN apt-get update -y
RUN apt-get install -y build-essential make xorriso qemu-utils qemu qemu-system-x86 patch
# grub is a bit special in containers
RUN DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" install grub2 kmod python3

CMD ["/bin/bash"]
