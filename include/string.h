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
#ifndef KTF_STRING_H
#define KTF_STRING_H
#include <asm-macros.h>
#include <slab.h>

static inline __used int isspace(int c) { return c == ' ' || c == '\t'; }

static inline __used int iseostr(int c) { return c == '\0'; }

static inline __used int isdigit(int c) { return c >= '0' && c <= '9'; }

static inline __used int isxdigit(int c) {
    return (isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'));
}

static inline __used int isascii(int c) { return c >= 0 && c <= 127; }

static inline __used int islower(int c) { return c >= 'a' && c <= 'z'; }

static inline int isupper(int c) { return c >= 'A' && c <= 'Z'; }

static inline int isalpha(int c) { return islower(c) || isupper(c); }

static inline size_t strlen(const char *str) {
    size_t len = 0;

    while (str[len])
        len++;

    return len;
}

static inline unsigned char tolower(unsigned char c) {
    return isupper(c) ? c - ('A' - 'a') : c;
}

static inline unsigned char toupper(unsigned char c) {
    return islower(c) ? c + ('A' - 'a') : c;
}

#if defined(__i386__)
#define SPRAY_VAL 0x01010101U
#define STOS      stosl
#define MOVS      movsl
#elif defined(__x86_64__)
#define SPRAY_VAL 0x0101010101010101UL
#define STOS      stosq
#define MOVS      movsq
#endif

#define ARCH_SIZE (sizeof(void *))

static inline void *memset(void *s, int c, size_t n) {
    unsigned long d0;

    /* clang-format off */
    asm volatile("mov %1, %%" STR(_ASM_CX) "\n"
                 "rep stosb\n"
                 "mov %2, %%" STR(_ASM_CX) "\n"
                 "rep " STR(STOS) "\n"
                 : "=&D"(d0)
                 : "ir"(n & (ARCH_SIZE - 1)), "ir"(n / ARCH_SIZE), "0"(s),
                   "a"(SPRAY_VAL * c)
                 : STR(_ASM_CX), "memory");
    /* clang-format on */

    return s;
}

static inline void *memcpy(void *d, void *s, size_t n) {
    unsigned long d0;

    /* clang-format off */
    asm volatile("mov %2, %%" STR(_ASM_CX) "\n"
                 "rep movsb \n"
                 "mov %3, %%" STR(_ASM_CX) "\n"
                 "rep " STR(MOVS) "\n"
                 : "=&D"(d0), "+&S"(s)
                 : "ir"(n & (ARCH_SIZE - 1)), "ir"(n / ARCH_SIZE), "0"(d)
                 : STR(_ASM_CX), "memory");
    /* clang-format on */

    return d;
}

static inline void *memmove(void *d, const void *s, size_t n) {

    /* dst and src are the same */
    if (d == s) {
        return d;
    }

    /* if we don't have a range overlap, just use memcpy */
    if ((d > s && d > s + n) || (d < s && d + n < s)) {
        return memcpy(d, (void *) s, n);
    }

    /*
     * s ------
     * d    ------
     * needs reverse moving
     */
    if (s < d) {
        while (n--)
            *((char *) d + n) = *((char *) s + n);
    }
    else
        /*
         * s     ------
         * d   -----
         * normal copy
         */
        return memcpy(d, (void *) s, n);

    return d;
}

static inline int memcmp(const void *m1, const void *m2, size_t n) {
    const uint8_t *_m1 = m1;
    const uint8_t *_m2 = m2;
    register uint8_t res = 0;

    if (m1 == m2)
        return 0;

    while (n--) {
        if ((res = (*_m1++ - *_m2++)) != 0)
            break;
    }

    return res;
}

static inline char *strcpy(char *d, const char *s) {
    int d0, d1, d2;

    asm volatile("1: lodsb \n"
                 "   stosb \n"
                 "   testb %%al,%%al \n"
                 "   jne 1b"
                 : "=&S"(d0), "=&D"(d1), "=&a"(d2)
                 : "0"(s), "1"(d)
                 : "memory");

    return d;
}

static inline int strcmp(const char *s1, const char *s2) {
    register char res;

    if (s1 == s2)
        return 0;

    while (1) {
        if ((res = *s1 - *s2++) != 0 || !*s1++)
            break;
    }

    return res;
}

static inline char *strncpy(char *d, const char *s, size_t n) {
    char *copy = d;

    while (n) {
        *copy++ = *s++;
        if (*s == '\0')
            break;
        n--;
    }

    memset(copy, '\0', n);

    return d;
}

static inline char *strchr(const char *s, int c) {
    if (NULL == s)
        return NULL;

    while (*s != (char) c) {
        if ('\0' == *s)
            return NULL;
        s++;
    }

    return (char *) s;
}

static inline int strncmp(const char *s1, const char *s2, size_t n) {
    register char res;

    if (s1 == s2)
        return 0;

    while (n--) {
        if ((res = *s1 - *s2++) != 0 || !*s1++)
            break;
    }

    return res;
}

static inline const char *string_trim_whitspace(const char *s) {
    while (isspace(*s))
        s++;

    return s;
}

static inline int string_empty(const char *s) { return !s || *s == '\0'; }

static inline int string_equal(const char *s1, const char *s2) {
    return (!s1 || !s2) ? s1 == s2 : !strcmp(s1, s2);
}

static inline char *strdup(const char *s1) {
    char *s2;
    size_t len = 0;

    if (string_empty(s1))
        return NULL;

    len = strlen(s1);
    s2 = (char *) ktf_alloc(len);

    if (NULL == s2)
        return NULL;

    return strcpy(s2, s1);
}

static inline size_t strspn(const char *s1, const char *s2) {
    size_t ret = 0;

    if (NULL == s1 || NULL == s2)
        return 0;

    while (*s1 && strchr(s2, *s1)) {
        s1++;
        ret++;
    }

    return ret;
}

static inline size_t strcspn(const char *s1, const char *s2) {
    size_t ret = 0;

    if (NULL == s1 || NULL == s2)
        return 0;

    while (*s1 && !strchr(s2, *s1)) {
        s1++;
        ret++;
    }

    return ret;
}

static inline char *strtok(char *s, const char *delim) {

    static char *lasts;
    int ch;

    if (NULL == s)
        s = lasts;

    do {
        if ((ch = *s++) == '\0')
            return NULL;
    } while (strchr(delim, ch));

    s--;
    lasts = s + strcspn(s, delim);
    *lasts = '\0';

    return s;
}

static inline char *strpbrk(const char *s, const char *chars) {
    for (int i = 0; s[i] != '\0'; i++) {
        for (int j = 0; chars[j] != '\0'; j++) {
            if (s[i] == chars[j]) {
                return (char *) &s[i];
            }
        }
    }

    return NULL;
}

static inline char *strstr(const char *s1, const char *s2) {
    char *a = NULL, *b = NULL;

    b = (char *) s2;
    /* s2 is NULL terminated, return entire s1 */
    if ('\0' == *b) {
        return (char *) s1;
    }

    /* first char match in s1 */
    for (; *s1 != '\0'; s1++) {
        if (*s1 != *b) {
            continue;
        }
        /* record the position */
        a = (char *) s1;

        while (1) {
            /* s2 has been searched, return s1 */
            if (*b == '\0') {
                return (char *) s1;
            }
            /* no match, break and find new a */
            if (*a++ != *b++) {
                break;
            }
        }
        /* rewind b all the way back to s2 start */
        b = (char *) s2;
    }

    return NULL;
}

static inline char *strsep(char **str, char *delim) {
    char *spanp = NULL, *s = NULL;
    int c, sc;
    char *tok = NULL;

    s = *str;
    if (NULL == s)
        return NULL;

    for (tok = s;; ++s) {
        c = *s;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == '\0') {
                    *str = NULL;
                    return (tok == s ? NULL : tok);
                }
                *s++ = '\0';
                *str = s;
                return tok;
            }
        } while (sc);
    }
}

static inline int strcasecmp(const char *s1, const char *s2) {
    unsigned char res, c1, c2;

    if (s1 == s2)
        return 0;

    while (1) {
        c1 = tolower(*(unsigned char *) s1);
        s1++;
        c2 = tolower(*(unsigned char *) s2);
        s2++;
        res = c1 - c2;
        if ((res != 0 || c1 == 0 || c2 == 0))
            break;
    }

    return res;
}

static inline int strncasecmp(const char *s1, const char *s2, size_t n) {
    unsigned char res = 0, c1, c2;

    if (s1 == s2)
        return 0;

    while (n--) {
        c1 = tolower(*(unsigned char *) s1);
        s1++;
        c2 = tolower(*(unsigned char *) s2);
        s2++;
        res = c1 - c2;
        if (res != 0 || c1 == 0 || c2 == 0)
            break;
    }

    return res;
}

/* External declarations */

extern unsigned long strtoul(const char *nptr, char **endptr, int base);
extern long strtol(const char *nptr, char **endptr, int base);
extern int vsnprintf(char *str, size_t size, char const *fmt, va_list ap);
extern int snprintf(char *buf, size_t size, const char *fmt, ...);

#endif /* KTF_STRING_H */
