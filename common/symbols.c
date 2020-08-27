/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <ktf.h>
#include <lib.h>
#include <mm/pmm.h>
#include <string.h>
#include <symbols.h>

static inline bool is_symbol_address(const void *addr, unsigned index) {
    return addr >= symbol_addresses[index] &&
           addr < symbol_addresses[index] + symbol_sizes[index];
}

/* This function uses binary search and requires symbol_addresses[] to be sorted */
static long symbol_index_by_address(const void *addr) {
    unsigned left, right;

    if (!in_text_section(addr))
        return -1;

    left = 0;
    right = symbol_count - 1;

    while (left != right) {
        unsigned mid = (left + right) / 2;

        if (is_symbol_address(addr, mid))
            return mid;
        else if (addr < symbol_addresses[mid])
            right = mid;
        else
            left = mid;
    }

    if (is_symbol_address(addr, left))
        return left;

    return -1;
}

static long symbol_index_by_name(const char *name) {
    /* FIXME: this probably should use better search algorithm */
    for (unsigned int i = 0; i < symbol_count; i++) {
        if (!strcmp(symbol_names_ptr[i], name))
            return i;
    }

    return -1;
}

const char *symbol_name(const void *addr) {
    long index = symbol_index_by_address(addr);

    return index < 0 ? NULL : symbol_names_ptr[index];
}

void *symbol_address(const char *name) {
    long index = symbol_index_by_name(name);

    return index < 0 ? NULL : symbol_addresses[index];
}

void print_symbol(const void *addr) {
    long index = symbol_index_by_address(addr);

    if (index < 0)
        return;

    printk("0x%016lx: %s + <0x%lx> [0x%x]\n", _ul(addr), symbol_names_ptr[index],
           _ul(addr - symbol_addresses[index]), symbol_sizes[index]);
}
