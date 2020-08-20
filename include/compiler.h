/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * Copyright © 2014,2015 Citrix Systems Ltd.
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
#ifndef KTF_COMPILER_H
#define KTF_COMPILER_H

#define _STR(x) #x
#define STR(x)  _STR(x)

#define _TOKEN(x, y) (x##y)

#ifdef __ASSEMBLY__
#define _INTEGER(num, suffix) num
#else
#define _INTEGER(num, suffix) _TOKEN(num, suffix)
#endif

#define _U32(x) _INTEGER(x, U)
#define _U64(x) _INTEGER(x, ULL)

#define KB(x) (_U64(x) << 10)
#define MB(x) (_U64(x) << 20)
#define GB(x) (_U64(x) << 30)

#ifndef __ASSEMBLY__
#define _ptr(val) ((void *) (unsigned long) (val))
#define _ul(val)  ((unsigned long) (val))
#define _int(val) ((int) (val))
#endif

#define __aligned(x) __attribute__((__aligned__(x)))
#define __noreturn   __attribute__((__noreturn__))
#define __packed     __attribute__((__packed__))
#define __used       __attribute__((__used__))
#define __noinline   __attribute__((__noinline__))
#define __section(s) __attribute__((__section__(s)))

#define UNREACHABLE()                                                                    \
    do {                                                                                 \
        __builtin_unreachable();                                                         \
    } while (0)

#define __text __section(".text")
#define __data __section(".data")
#define __bss  __section(".bss")

#define __text_init __section(".text.init")
#define __data_init __section(".data.init")
#define __bss_init  __section(".bss.init")

#define IS_INIT_SECTION(name)                                                            \
    (!strcmp(name, ".text.init") || !strcmp(name, ".data.init") ||                       \
     !strcmp(name, ".bss.init"))

#define __user_text __section(".text.user")
#define __user_data __section(".data.user")
#define __user_bss  __section(".bss.user")

#define __cmdline __section(".cmdline")

#define barrier()      asm volatile("" ::: "memory")
#define ACCESS_ONCE(x) (*(volatile typeof(x) *) &(x))

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#ifndef offsetof
#define offsetof(type, member) ((off_t) & ((type *) 0)->member)
#endif

#define BUILD_BUG_ON(cond) ({ _Static_assert(!(cond), "!(" STR(cond) ")"); })

#ifndef __same_type
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#endif

#define container_of(ptr, type, member)                                                  \
    ({                                                                                   \
        void *_ptr = (void *) (ptr);                                                     \
        BUILD_BUG_ON(!__same_type(*(ptr), ((type *) 0)->member) &&                       \
                     !__same_type(*(ptr), void));                                        \
        ((type *) (_ptr - offsetof(type, member)));                                      \
    })

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#define VA_NARGS_(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N

#define VA_NARGS(...) VA_NARGS_(_, ##__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define COUNT_MACRO_ARGS__(macro, tok, count, ...) macro##count(tok, ##__VA_ARGS__)
#define COUNT_MACRO_ARGS_(macro, tok, count, ...)                                        \
    COUNT_MACRO_ARGS__(macro, tok, count, ##__VA_ARGS__)
#define COUNT_MACRO_ARGS(macro, tok, ...)                                                \
    COUNT_MACRO_ARGS_(macro, tok, VA_NARGS(__VA_ARGS__), ##__VA_ARGS__)

#define TOKEN_OR0(t)          (0)
#define TOKEN_OR1(t, x)       (t##x)
#define TOKEN_OR2(t, x, ...)  (t##x | TOKEN_OR1(t, ##__VA_ARGS__))
#define TOKEN_OR3(t, x, ...)  (t##x | TOKEN_OR2(t, ##__VA_ARGS__))
#define TOKEN_OR4(t, x, ...)  (t##x | TOKEN_OR3(t, ##__VA_ARGS__))
#define TOKEN_OR5(t, x, ...)  (t##x | TOKEN_OR4(t, ##__VA_ARGS__))
#define TOKEN_OR6(t, x, ...)  (t##x | TOKEN_OR5(t, ##__VA_ARGS__))
#define TOKEN_OR7(t, x, ...)  (t##x | TOKEN_OR6(t, ##__VA_ARGS__))
#define TOKEN_OR8(t, x, ...)  (t##x | TOKEN_OR7(t, ##__VA_ARGS__))
#define TOKEN_OR9(t, x, ...)  (t##x | TOKEN_OR8(t, ##__VA_ARGS__))
#define TOKEN_OR10(t, x, ...) (t##x | TOKEN_OR9(t, ##__VA_ARGS__))
#define TOKEN_OR11(t, x, ...) (t##x | TOKEN_OR10(t, ##__VA_ARGS__))

#define TOKEN_OR(t, ...) COUNT_MACRO_ARGS(TOKEN_OR, t, ##__VA_ARGS__)

#endif /* KTF_COMPILER_H */
