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
#ifndef KTF_CMDLINE_H
#define KTF_CMDLINE_H

#ifndef __ASSEMBLY__

#define PARAM_MAX_LENGTH 32

struct __packed ktf_param {
    char name[PARAM_MAX_LENGTH];
    enum { STRING, ULONG, BOOL } type;
    void *var;
};

#define __ktfparam static __cmdline __used __aligned(1) struct ktf_param

/* compile time check for param name size */
#define __param_size_check(_name, _sizename)                                             \
    char __unused_##_name[(sizeof(_sizename) >= PARAM_MAX_LENGTH) ? -1 : 0]

#define cmd_param(_name, _var, _type)                                                    \
    __param_size_check(_var, _name);                                                     \
    __ktfparam __cmd_##_var = {_name, _type, &_var};

#define bool_cmd(_cmdname, _varname)   cmd_param(_cmdname, _varname, BOOL)
#define ulong_cmd(_cmdname, _varname)  cmd_param(_cmdname, _varname, ULONG)
#define string_cmd(_cmdname, _varname) cmd_param(_cmdname, _varname, STRING)

#endif /* __ASSEMBLY__ */

#endif /* KTF_CMDLINE_H */