#!/bin/bash
#
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# remove all previous files and stale dir structure

# I really don't know why pushd is not working on this one 
# while it's working for me in build_libpfm.sh. 
# I am invoking both them in similar way from Makefile.
# Using cd for now.

cd ../..
# Save KTF root
KTF_ROOT=$PWD

cd ${KTF_ROOT}/third-party/libpfm
rm -rf config.mk COPYING include lib Makefile README rules.mk libpfm_diff.patch build_libpfm.sh
