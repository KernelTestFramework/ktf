#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
 Copyright © 2020 Amazon.com, Inc. or its affiliates.
 All Rights Reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

import argparse
import fileinput
import struct
import sys


header_files = ["asm-macros.h"]


def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate assembly file with symbols information')
    parser.add_argument(
        '-s', '--section-name',
        dest='section_name',
        default='symbols',
        type=str,
        help='Name of a section holding symbols'
    )
    parser.add_argument(
        '-o', '--output',
        dest='output_file',
        default='symbols.S',
        type=str,
        help='Name of the output assembly file'
    )

    return parser.parse_args()


def add_headers(output, headers):
    output_str = "\n".join(["#include <%s>" % h for h in headers])
    output.write(output_str)


def add_type_detection(output):
    output_str = """
# if defined(__x86_64__)
# define TYPE .quad
# else
# define TYPE .long
# endif
    """
    output.write(output_str)


def add_section_start(output, name, align=8):
    output_str = "SECTION(.%s, \"a\", %d)\n" % (name, align)
    output_str += "GLOBAL(__start_%s)\n\n" % name
    output.write(output_str)


def add_section_end(output, name, align=0x1000):
    output_str = ".align 0x%x\n" % align
    output_str += "GLOBAL(__end_%s)\n" % name
    output.write(output_str)


def add_object(output, name, content):
    output_str = "GLOBAL(%s)\n" % name
    output_str += "%s\n" % content
    output_str += "END_OBJECT(%s)\n" % name
    output.write(output_str)


def main():
    symbols_array = []
    symbol_names = []
    symbol_addresses = []
    symbol_sizes = []

    args = parse_args()

    for line in fileinput.input():
        sym = line.split()

        # Skip symbols without size
        if len(sym) < 4:
            continue

        symbols_array.append(sym)

    symbols_array.sort(key=lambda x: int(x[2], 16))

    for sym in symbols_array:
        symbol_names.append(sym[0])
        symbol_addresses.append(int(sym[2], 16))
        symbol_sizes.append(int(sym[3], 16))

    if len(symbol_addresses) != len(symbol_names) or len(symbol_addresses) != len(symbol_sizes):
        print("Input parsing failed. Incorrect number of fields: symbol_addresses=%u, symbol_names=%u, symbol_sizes=%u\n" % (
            symbol_addresses, symbol_names, symbol_sizes))
        return 1

    with open(args.output_file, "w") as output:
        add_headers(output, header_files)
        add_type_detection(output)

        add_section_start(output, args.section_name)

        output_str = '\n'.join(['TYPE 0x%lx' % x for x in symbol_addresses])
        add_object(output, "symbol_addresses", output_str)

        output_str = '\n'.join(['.long 0x%x' % x for x in symbol_sizes])
        add_object(output, "symbol_sizes", output_str)

        output_str = '\n'.join(['.ascii "%s"\n.byte 0x0' %
                                x for x in symbol_names])
        add_object(output, "symbol_names", output_str)

        output_str = ""
        current_len = 0
        for name in symbol_names:
            output_str += "TYPE symbol_names + 0x%lx\n" % current_len
            current_len += len(name) + 1
        add_object(output, "symbol_names_ptr", output_str)

        add_object(output, "symbol_count", '.long 0x%x' %
                   len(symbol_addresses))

        add_section_end(output, args.section_name)

    return 0


if __name__ == '__main__':
    sys.exit(main())
