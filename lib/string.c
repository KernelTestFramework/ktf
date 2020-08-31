/*-
 * Copyright © 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Copyright © 2011 The FreeBSD Foundation
 * All rights reserved.
 * Portions of this software were developed by David Chisnall
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ktf.h>
#include <string.h>

/* clang-format off */

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long strtoul(const char *nptr, char **endptr, int base)
{
        const char *s = nptr;
        unsigned long acc;
        unsigned char c;
        unsigned long cutoff;
        int neg = 0, any, cutlim;

        /*
         * See strtol for comments as to the logic used.
         */
        do {
                c = *s++;
        } while (isspace(c));
        if (c == '-') {
                neg = 1;
                c = *s++;
        } else if (c == '+')
                c = *s++;
        if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X')) {
                c = s[1];
                s += 2;
                base = 16;
        }
        if (base == 0)
                base = c == '0' ? 8 : 10;
        cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
        cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
        for (acc = 0, any = 0;; c = *s++) {
                if (!isascii(c))
                        break;
                if (isdigit(c))
                        c -= '0';
                else if (isalpha(c))
                        c -= isupper(c) ? 'A' - 10 : 'a' - 10;
                else
                        break;
                if (c >= base)
                        break;
                if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
                        any = -1;
                else {
                        any = 1;
                        acc *= base;
                        acc += c;
                }
        }
        if (any < 0) {
                acc = ULONG_MAX;
        } else if (neg)
                acc = -acc;
        if (endptr != 0)
                *endptr = (char *)(const void *) (any ? s - 1 : nptr);
        return (acc);
}

long strtol(const char *nptr, char **endptr, int base)
{
    const char *s;
    unsigned long acc;
    char c;
    unsigned long cutoff;
    int neg, any, cutlim;

    /*
     * Skip white space and pick up leading +/- sign if any.
     * If base is 0, allow 0x for hex and 0 for octal, else
     * assume decimal; if base is already 16, allow 0x.
     */
    s = nptr;
    do {
        c = *s++;
    } while ( isspace(c) );
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else {
        neg = 0;
        if (c == '+')
            c = *s++;
    }
    if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X') && isxdigit(s[1])) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;
    acc = any = 0;
    if (base < 2 || base > 36)
        goto noconv;

    /*
     * Compute the cutoff value between legal numbers and illegal
     * numbers.  That is the largest legal value, divided by the
     * base.  An input number that is greater than this value, if
     * followed by a legal input character, is too big.  One that
     * is equal to this value may be valid or not; the limit
     * between valid and invalid numbers is then based on the last
     * digit.  For instance, if the range for longs is
     * [-2147483648..2147483647] and the input base is 10,
     * cutoff will be set to 214748364 and cutlim to either
     * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
     * a value > 214748364, or equal but the next digit is > 7 (or 8),
     * the number is too big, and we will return a range error.
     *
     * Set 'any' if any `digits' consumed; make it negative to indicate
     * overflow.
     */

    cutoff = neg ? (unsigned long)-(LONG_MIN + LONG_MAX) + LONG_MAX
        : LONG_MAX;
    cutlim = cutoff % base;
    cutoff /= base;
    for ( ; ; c = *s++) {
        if (isdigit(c))
            c -= '0';
        else if (c >= 'A' && c <= 'Z')
            c -= 'A' - 10;
        else if (c >= 'a' && c <= 'z')
            c -= 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = neg ? LONG_MIN : LONG_MAX;
    } else if (!any) {
noconv:
        ;
    } else if (neg)
        acc = -acc;
    if (endptr != NULL)
        *endptr = (char *)(any ? s - 1 : nptr);
    return (acc);
}

static char const hex2ascii_data[] = "0123456789abcdefghijklmnopqrstuvwxyz";

/*
 * Put a NUL-terminated ASCII number (base <= 36) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */
static char * ksprintn(char *nbuf, uintmax_t num, int base, int *lenp, int upper)
{
        char *p, c;

        p = nbuf;
        *p = '\0';
        do {
                c = hex2ascii_data[num % base];
                *++p = upper ? toupper(c) : c;
        } while (num /= base);
        if (lenp)
                *lenp = p - nbuf;
        return (p);
}

/*
 * Scaled down version of printf(3).
 */
int vsnprintf(char *str, size_t size, char const *fmt, va_list ap)
{
#define PCHAR(c) { if (size >= 2) { *str++ = c; size--; } retval++; }
#define MAXNBUF 65
        char nbuf[MAXNBUF];
        const char *p, *percent;
        int ch, n;
        uint64_t num;
        int base, lflag, qflag, tmp, width, ladjust, sharpflag, neg, sign, dot;
        int cflag, hflag, jflag, tflag, zflag;
        int dwidth, upper;
        char padc;
        int stop = 0, retval = 0;

        num = 0;

        if (fmt == NULL)
                fmt = "(fmt null)\n";

        for (;;) {
                padc = ' ';
                width = 0;
                while ((ch = (unsigned char)*fmt++) != '%' || stop) {
                        if (ch == '\0') {
                                if (size >= 1)
                                        *str++ = '\0';
                                return (retval);
                        }
                        PCHAR(ch);
                }
                percent = fmt - 1;
                qflag = 0; lflag = 0; ladjust = 0; sharpflag = 0; neg = 0;
                sign = 0; dot = 0; dwidth = 0; upper = 0;
                cflag = 0; hflag = 0; jflag = 0; tflag = 0; zflag = 0;
reswitch:       switch (ch = (unsigned char)*fmt++) {
                case '.':
                        dot = 1;
                        goto reswitch;
                case '#':
                        sharpflag = 1;
                        goto reswitch;
                case '+':
                        sign = 1;
                        goto reswitch;
                case '-':
                        ladjust = 1;
                        goto reswitch;
                case '%':
                        PCHAR(ch);
                        break;
                case '*':
                        if (!dot) {
                                width = va_arg(ap, int);
                                if (width < 0) {
                                        ladjust = !ladjust;
                                        width = -width;
                                }
                        } else {
                                dwidth = va_arg(ap, int);
                        }
                        goto reswitch;
                case '0':
                        if (!dot) {
                                padc = '0';
                                goto reswitch;
                        }
                        /* fallthrough */
                case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                                for (n = 0;; ++fmt) {
                                        n = n * 10 + ch - '0';
                                        ch = *fmt;
                                        if (ch < '0' || ch > '9')
                                                break;
                                }
                        if (dot)
                                dwidth = n;
                        else
                                width = n;
                        goto reswitch;
                case 'c':
                        PCHAR(va_arg(ap, int));
                        break;
                case 'd':
                case 'i':
                        base = 10;
                        sign = 1;
                        goto handle_sign;
                case 'h':
                        if (hflag) {
                                hflag = 0;
                                cflag = 1;
                        } else
                                hflag = 1;
                        goto reswitch;
                case 'j':
                        jflag = 1;
                        goto reswitch;
                case 'l':
                        if (lflag) {
                                lflag = 0;
                                qflag = 1;
                        } else
                                lflag = 1;
                        goto reswitch;
                case 'n':
                        if (jflag)
                                *(va_arg(ap, intmax_t *)) = retval;
                        else if (qflag)
                                *(va_arg(ap, int64_t *)) = retval;
                        else if (lflag)
                                *(va_arg(ap, long *)) = retval;
                        else if (zflag)
                                *(va_arg(ap, size_t *)) = retval;
                        else if (hflag)
                                *(va_arg(ap, short *)) = retval;
                        else if (cflag)
                                *(va_arg(ap, char *)) = retval;
                        else
                                *(va_arg(ap, int *)) = retval;
                        break;
                case 'o':
                        base = 8;
                        goto handle_nosign;
                case 'p':
                        base = 16;
                        sharpflag = (width == 0);
                        sign = 0;
                        num = (uintptr_t)va_arg(ap, void *);
                        goto number;
                case 'q':
                        qflag = 1;
                        goto reswitch;
                case 'r':
                        base = 10;
                        if (sign)
                                goto handle_sign;
                        goto handle_nosign;
                case 's':
                        p = va_arg(ap, char *);
                        if (p == NULL)
                                p = "(null)";
                        if (!dot)
                                n = strlen (p);
                        else
                                for (n = 0; n < dwidth && p[n]; n++)
                                        continue;

                        width -= n;

                        if (!ladjust && width > 0)
                                while (width--)
                                        PCHAR(padc);
                        while (n--)
                                PCHAR(*p++);
                        if (ladjust && width > 0)
                                while (width--)
                                        PCHAR(padc);
                        break;
                case 't':
                        tflag = 1;
                        goto reswitch;
                case 'u':
                        base = 10;
                        goto handle_nosign;
                case 'X':
                        upper = 1;
                        #if defined(__GNUC__) && !defined(__clang__)
                        __attribute__ ((fallthrough));
                        #endif
                case 'x':
                        base = 16;
                        goto handle_nosign;
                case 'y':
                        base = 16;
                        sign = 1;
                        goto handle_sign;
                case 'z':
                        zflag = 1;
                        goto reswitch;
handle_nosign:
                        sign = 0;
                        if (jflag)
                                num = va_arg(ap, uintmax_t);
                        else if (qflag)
                                num = va_arg(ap, uint64_t);
                        else if (tflag)
                                num = va_arg(ap, ptrdiff_t);
                        else if (lflag)
                                num = va_arg(ap, unsigned long);
                        else if (zflag)
                                num = va_arg(ap, size_t);
                        else if (hflag)
                                num = (unsigned short)va_arg(ap, int);
                        else if (cflag)
                                num = (unsigned char)va_arg(ap, int);
                        else
                                num = va_arg(ap, unsigned int);
                        goto number;
handle_sign:
                        if (jflag)
                                num = va_arg(ap, intmax_t);
                        else if (qflag)
                                num = va_arg(ap, int64_t);
                        else if (tflag)
                                num = va_arg(ap, ptrdiff_t);
                        else if (lflag)
                                num = va_arg(ap, long);
                        else if (zflag)
                                num = va_arg(ap, long);
                        else if (hflag)
                                num = (short)va_arg(ap, int);
                        else if (cflag)
                                num = (char)va_arg(ap, int);
                        else
                                num = va_arg(ap, int);
number:
                        if (sign && (intmax_t)num < 0) {
                                neg = 1;
                                num = -(intmax_t)num;
                        }
                        p = ksprintn(nbuf, num, base, &n, upper);
                        tmp = 0;
                        if (sharpflag && num != 0) {
                                if (base == 8)
                                        tmp++;
                                else if (base == 16)
                                        tmp += 2;
                        }
                        if (neg)
                                tmp++;

                        if (!ladjust && padc == '0')
                                dwidth = width - tmp;
                        width -= tmp + (dwidth > n ? dwidth : n);
                        dwidth -= n;
                        if (!ladjust)
                                while (width-- > 0)
                                        PCHAR(' ');
                        if (neg)
                                PCHAR('-');
                        if (sharpflag && num != 0) {
                                if (base == 8) {
                                        PCHAR('0');
                                } else if (base == 16) {
                                        PCHAR('0');
                                        PCHAR('x');
                                }
                        }
                        while (dwidth-- > 0)
                                PCHAR('0');

                        while (*p)
                                PCHAR(*p--);

                        if (ladjust)
                                while (width-- > 0)
                                        PCHAR(' ');

                        break;
                default:
                        while (percent < fmt)
                                PCHAR(*percent++);
                        /*
                         * Since we ignore a formatting argument it is no
                         * longer safe to obey the remaining formatting
                         * arguments as the arguments will no longer match
                         * the format specs.
                         */
                        stop = 1;
                        break;
                }
        }
#undef PCHAR
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    int retval = 0;

    va_start(args, fmt);
    retval = vsnprintf(buf, size, fmt, args);
    va_end(args);

    return retval;
}
/* clang-format on */
