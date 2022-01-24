/*
 * Copyright (c) 2021 Amazon.com, Inc. or its affiliates.
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

#include <cmdline.h>
#include <ktf.h>
#include <mm/regions.h>
#include <string.h>

bool opt_debug = false;
bool_cmd("debug", opt_debug);

bool opt_keyboard = true;
bool_cmd("keyboard", opt_keyboard);

bool opt_pit = false;
bool_cmd("pit", opt_pit);

bool opt_apic_timer = false;
bool_cmd("apic_timer", opt_apic_timer);

bool opt_hpet = false;
bool_cmd("hpet", opt_hpet);

bool opt_fpu = false;
bool_cmd("fpu", opt_fpu);

bool opt_qemu_console = false;
bool_cmd("qemu_console", opt_qemu_console);

bool opt_poweroff = true;
bool_cmd("poweroff", opt_poweroff);

const char *kernel_cmdline;

void __text_init cmdline_parse(const char *cmdline) {
    static __bss_init char opt[PAGE_SIZE];
    char *optval, *optkey, *q;
    const char *p = cmdline;
    struct ktf_param *param;

    if (cmdline == NULL)
        return;

    for (;;) {
        p = string_trim_whitspace(p);

        if (iseostr(*p))
            break;

        q = optkey = opt;
        while ((!isspace(*p)) && (!iseostr(*p))) {
            ASSERT(_ul(q - opt) < sizeof(opt) - 1);
            *q++ = *p++;
        }
        *q = '\0';

        /* split on '=' */
        optval = strchr(opt, '=');
        if (optval != NULL)
            *optval++ = '\0';
        else
            /* assume a bool later */
            optval = opt;

        for (param = __start_cmdline; param < __end_cmdline; param++) {
            if (strcmp(param->name, optkey))
                continue;

            switch (param->type) {
            case STRING:
                strncpy(param->var, optval, param->varlen);
                if (strlen(optval) >= param->varlen) {
                    ((char *) param->var)[param->varlen - 1] = '\0';
                    printk("WARNING: The commandline parameter value for %s does not fit "
                           "into the preallocated buffer (size %lu >= %u)\n",
                           param->name, strlen(optval), param->varlen);
                }
                break;
            case ULONG:
                *(unsigned long *) param->var = strtoul(optval, NULL, 0);
                break;
            case BOOL:
                *(bool *) param->var =
                    !!parse_bool(!strcmp(optval, optkey) ? "1" : optval);
                break;
            default:
                panic("Unkown cmdline type detected...");
                break;
            }
        }
    }
}
