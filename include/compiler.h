#ifndef KTF_COMPILER_H
#define KTF_COMPILER_H

#define _TOKEN(x, y) (x ## y)

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
#define _ptr(val) ((void *)(unsigned long)(val))
#define _ul(val) ((unsigned long)(val))
#endif

#define __aligned(x)    __attribute__((__aligned__(x)))
#define __noreturn      __attribute__((__noreturn__))
#define __packed        __attribute__((__packed__))
#define __used          __attribute__((__used__))
#define __noinline      __attribute__((__noinline__))
#define __section(s)    __attribute__((__section__(s)))

#define barrier() __asm__ __volatile__ ("" ::: "memory")

#define ARRAY_SIZE(a)    (sizeof(a) / sizeof(*a))

#endif /* KTF_COMPILER_H */
