#!/bin/bash
#
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# This script builds libpfm
set -e

# Export variable that'll be used in makefile of pfmlib to do conditional compile
export KTF_PFMLIB_COMPILE=1
# Get tools directory saved 
PFMLIB_TOOLS_ROOT=$PWD
# Go to KTF root
pushd ../..
# Save KTF root
KTF_ROOT=$PWD
# Export KTF root, it'll be used in makefile of pfmlib
export KTF_ROOT
# change directory to libpfm folder 
pushd ${KTF_ROOT}/third-party/libpfm 

# Start clean, remove all previous files and stale dir structure
rm -rf config.mk COPYING include lib Makefile README rules.mk
# Untar only required directory and files and at current dir level
tar --strip-components=1 -xf libpfm-4.10.1.tar.gz libpfm-4.10.1/lib libpfm-4.10.1/include libpfm-4.10.1/rules.mk libpfm-4.10.1/config.mk libpfm-4.10.1/Makefile libpfm-4.10.1/README libpfm-4.10.1/COPYING -C ./

# Patch the files
patch -p1 <${PFMLIB_TOOLS_ROOT}/libpfm_diff.patch
# build solution
make -j lib
#clean up source (c and h) files, we only need object files
find . -name \*.h -delete 
find . -name \*.c -delete

popd
popd