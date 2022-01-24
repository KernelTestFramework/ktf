/*
 * Copyright (c) 2021 Amazon.com, Inc. or its affiliates.
 * Copyright (c) 2022 Open Source Security, Inc.
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
#include <drivers/serial.h>
#include <ktf.h>
#include <mm/regions.h>
#include <string.h>

#ifdef KTF_DEBUG
bool opt_debug = true;
#else
bool opt_debug = false;
#endif
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

static char opt_com1[20];
string_cmd("com1", opt_com1);

static char opt_com2[20];
string_cmd("com2", opt_com2);

static char opt_com3[20];
string_cmd("com3", opt_com3);

static char opt_com4[20];
string_cmd("com4", opt_com4);

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

static __text_init bool _parse_com_port(char *port_str, io_port_t *port) {
    unsigned long tmp;

    if (string_empty(port_str))
        return false;

    tmp = strtoul(port_str, NULL, 16);
    switch (tmp) {
    case COM1_PORT:
    case COM2_PORT:
    case COM3_PORT:
    case COM4_PORT:
        *port = tmp;
        break;
    default:
        return false;
    }

    return true;
}

static __text_init bool _parse_com_baud(char *baud_str, com_baud_t *baud) {
    unsigned long tmp;

    if (string_empty(baud_str))
        return false;

    tmp = strtoul(baud_str, NULL, 0);
    switch (tmp) {
    case COM_BAUD_300:
    case COM_BAUD_1200:
    case COM_BAUD_2400:
    case COM_BAUD_4800:
    case COM_BAUD_9600:
    case COM_BAUD_19200:
    case COM_BAUD_38400:
    case COM_BAUD_57600:
    case COM_BAUD_115200:
        *baud = tmp;
        break;
    default:
        return false;
    }

    return true;
}

static __text_init bool _parse_com_width(char *frame_size_str,
                                         com_frame_size_t *frame_size) {
    unsigned long tmp;

    if (string_empty(frame_size_str))
        return false;

    tmp = strtoul(frame_size_str, NULL, 0);
    switch (tmp) {
    case 8:
        *frame_size = COM_FRAME_SIZE_8_BITS;
        break;
    case 7:
        *frame_size = COM_FRAME_SIZE_7_BITS;
        break;
    case 6:
        *frame_size = COM_FRAME_SIZE_6_BITS;
        break;
    case 5:
        *frame_size = COM_FRAME_SIZE_5_BITS;
        break;
    default:
        return false;
    }

    return true;
}

static __text_init bool _parse_com_parity(char *parity_str, com_parity_t *parity) {
    if (string_empty(parity_str) || strlen(parity_str) > 1)
        return false;

    switch (parity_str[0]) {
    case 'n':
        *parity = COM_NO_PARITY;
        break;
    case 'o':
        *parity = COM_ODD_PARITY;
        break;
    case 'e':
        *parity = COM_EVEN_PARITY;
        break;
    case 'h':
        *parity = COM_HIGH_PARITY;
        break;
    case 'l':
        *parity = COM_LOW_PARITY;
        break;
    default:
        return false;
    }

    return true;
}

static __text_init bool _parse_com_stop_bit(char *stop_bit_str,
                                            com_stop_bit_t *stop_bit) {
    unsigned long tmp;

    if (string_empty(stop_bit_str))
        return false;

    tmp = strtoul(stop_bit_str, NULL, 0);
    switch (tmp) {
    case 1:
        *stop_bit = COM_STOP_BIT_1;
        break;
    case 2:
        *stop_bit = COM_STOP_BIT_2;
        break;
    default:
        return false;
    }

    return true;
}

__text_init bool parse_com_port(com_idx_t com, uart_config_t *cfg) {
    char *tmp_str = NULL;
    char *opt_com;

    switch (com) {
    case COM1:
        opt_com = opt_com1;
        break;
    case COM2:
        opt_com = opt_com2;
        break;
    case COM3:
        opt_com = opt_com3;
        break;
    case COM4:
        opt_com = opt_com4;
        break;
    default:
        return false;
    }

    if (string_empty(opt_com))
        return false;

    tmp_str = strtok(opt_com, ",");
    if (!_parse_com_port(tmp_str, &cfg->port))
        return false;

    tmp_str = strtok(NULL, ",");
    if (string_empty(tmp_str))
        cfg->baud = DEFAULT_BAUD_SPEED;
    else if (!_parse_com_baud(tmp_str, &cfg->baud))
        return false;

    tmp_str = strtok(NULL, ",");
    if (string_empty(tmp_str))
        cfg->frame_size = COM_FRAME_SIZE_8_BITS;
    else if (!_parse_com_width(tmp_str, &cfg->frame_size))
        return false;

    tmp_str = strtok(NULL, ",");
    if (string_empty(tmp_str))
        cfg->parity = COM_NO_PARITY;
    else if (!_parse_com_parity(tmp_str, &cfg->parity))
        return false;

    tmp_str = strtok(NULL, ",");
    if (string_empty(tmp_str))
        cfg->stop_bit = COM_STOP_BIT_1;
    else if (!_parse_com_stop_bit(tmp_str, &cfg->stop_bit))
        return false;

    return true;
}
