#!/usr/bin/env python
#
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# In order to generate the patch .h & .c files in libpfm had to be patched
# (almost 200-300 of them). So I wrote a python script (see tools/libpfm/patch_headers.py) 
# to do that. libpfm uses standard C headers while we compile KTF without standard C headers
# So those includes need to be patched out. We also have to include ktf.h in C and h files.
# This script does that as well.
import sys
import fnmatch
import os

std_includes = [
            "<sys/types.h>",
            "<ctype.h>",
            "<stdlib.h>",
            "<stdio.h>",
            "<stdarg.h>",
            "<limits.h>",
            "<unistd.h>",
            "<sys/syscall.h>",
            "<sys/ioctl.h>",
            "<sys/prctl.h>",
            ]

ktf_patch_disclaimer = "/* Auto generated disclaimer by KTF project to patch headers */\n"
ktf_include = "#include <ktf.h>\n"
ktf_header_insert = ktf_patch_disclaimer + ktf_include

c_and_h_files = []

for root, dirnames, filenames in os.walk(os.curdir):
    for filename in fnmatch.filter(filenames, '*.c'):
        c_and_h_files.append(os.path.join(root, filename))
    for filename in fnmatch.filter(filenames, '*.h'):
        c_and_h_files.append(os.path.join(root, filename))

for source_file in c_and_h_files:
    f = open(source_file)
    lines = f.readlines()

    lines = [
        "" if any(include in line for include in std_includes) else line
        for line in lines
    ]

    for index, line in enumerate(lines):
        if line.startswith('#include'):
            if ktf_include not in line:
                lines.insert(index, ktf_header_insert)
            break

    with open(source_file, "w") as f:
            f.writelines(lines)
