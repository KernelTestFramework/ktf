FROM ubuntu:20.04

ARG USER
ARG USER_ID
ARG GROUP_ID

# build dependencies
RUN apt-get update -y
RUN apt-get install -y gcc make xorriso qemu-utils
# grub is a bit special in containers
RUN DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" install grub2 python

# Create proper users so that our build artifacts
# can be shared with the outside user
# https://vsupalov.com/docker-shared-permissions/
RUN addgroup --gid $GROUP_ID $USER
RUN adduser --disabled-password --gecos '' --uid $USER_ID --gid $GROUP_ID $USER
USER $USER

CMD ["/bin/bash"]
