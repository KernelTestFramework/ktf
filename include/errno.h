/*
 * Copyright (c) 2020 Amazon.com, Inc. or its affiliates.
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
#ifndef _KTF_ERRNO_H
#define _KTF_ERRNO_H

#define    ESUCCESS        0
#define    EPERM           1
#define    ENOENT          2
#define    ESRCH           3
#define    EINTR           4
#define    EIO             5
#define    ENXIO           6
#define    E2BIG           7
#define    ENOEXEC         8
#define    EBADF           9
#define    ECHILD          10
#define    EAGAIN          11
#define    ENOMEM          12
#define    EACCES          13
#define    EFAULT          14
#define    ENOTBLK         15
#define    EBUSY           16
#define    EEXIST          17
#define    EXDEV           18
#define    ENODEV          19
#define    ENOTDIR         20
#define    EISDIR          21
#define    EINVAL          22
#define    ENFILE          23
#define    EMFILE          24
#define    ENOTTY          25
#define    ETXTBSY         26
#define    EFBIG           27
#define    ENOSPC          28
#define    ESPIPE          29
#define    EROFS           30
#define    EMLINK          31
#define    EPIPE           32
#define    EDOM            33
#define    ERANGE          34
#define    EDEADLK         35
#define    ENAMETOOLONG    36
#define    ENOLCK          37
#define    ENOSYS          38
#define    ENOTEMPTY       39
#define    ELOOP           40
#define    EWOULDBLOCK     41
#define    ENOMSG          42
#define    EIDRM           43
#define    ECHRNG          44
#define    EL2NSYNC        45
#define    EL3HLT          46
#define    EL3RST          47
#define    ELNRNG          48
#define    EUNATCH         49
#define    ENOCSI          50
#define    EL2HLT          51
#define    EBADE           52
#define    EBADR           53
#define    EXFULL          54
#define    ENOANO          55
#define    EBADRQC         56
#define    EBADSLT         57
#define    EDEADLOCK       58
#define    EBFONT          59
#define    ENOSTR          60
#define    ENODATA         61
#define    ETIME           62
#define    ENOSR           63
#define    ENONET          64
#define    ENOPKG          65
#define    EREMOTE         66
#define    ENOLINK         67
#define    EADV            68
#define    ESRMNT          69
#define    ECOMM           70
#define    EPROTO          71
#define    EMULTIHOP       72
#define    EDOTDOT         73
#define    EBADMSG         74
#define    EOVERFLOW       75
#define    ENOTUNIQ        76
#define    EBADFD          77
#define    EREMCHG         78
#define    ELIBACC         79
#define    ELIBBAD         80
#define    ELIBSCN         81
#define    ELIBMAX         82
#define    ELIBEXEC        83
#define    EILSEQ          84
#define    ERESTART        85
#define    ESTRPIPE        86
#define    EUSERS          87
#define    ENOTSOCK        88
#define    EDESTADDRREQ    89
#define    EMSGSIZE        90
#define    EPROTOTYPE      91
#define    ENOPROTOOPT     92
#define    EPROTONOSUPPORT 93
#define    ESOCKTNOSUPPORT 94
#define    EOPNOTSUPP      95
#define    EPFNOSUPPORT    96
#define    EAFNOSUPPORT    97
#define    EADDRINUSE      98
#define    EADDRNOTAVAIL   99
#define    ENETDOWN        100
#define    ENETUNREACH     101
#define    ENETRESET       102
#define    ECONNABORTED    103
#define    ECONNRESET      104
#define    ENOBUFS         105
#define    EISCONN         106
#define    ENOTCONN        107
#define    ESHUTDOWN       108
#define    ETOOMANYREFS    109
#define    ETIMEDOUT       110
#define    ECONNREFUSED    111
#define    EHOSTDOWN       112
#define    EHOSTUNREACH    113
#define    EALREADY        114
#define    EINPROGRESS     115
#define    ESTALE          116
#define    EUCLEAN         117
#define    ENOTNAM         118
#define    ENAVAIL         119
#define    EISNAM          120
#define    EREMOTEIO       121

#endif /*_KTF_ERRNO_H */
