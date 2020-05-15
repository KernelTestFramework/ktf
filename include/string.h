#ifndef KTF_STRING_H
#define KTF_STRING_H
#include <asm-macros.h>

static inline __used int isspace(int c) {
    return c == ' ' || c == '\t';
}

static inline __used int isdigit(int c) {
    return c >= '0' && c <= '9';
}

static inline __used int isxdigit(int c) {
    return (isdigit(c) || (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f'));
}

static inline __used int isascii(int c) {
    return c >= 0 && c <= 127;
}

static inline __used int islower(int c) {
    return c >= 'a' && c <= 'z';
}

static inline int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

static inline int isalpha(int c) {
    return islower(c) || isupper(c);
}

static inline size_t strlen(const char *str) {
    size_t len = 0;

    while (str[len])
        len++;

    return len;
}

static inline unsigned char tolower(unsigned char c)
{
        return isupper(c) ? c - ('A'-'a') : c;
}

static inline unsigned char toupper(unsigned char c)
{
        return islower(c) ? c + ('A'-'a') : c;
}

#if defined(__i386__)
#define SPRAY_VAL 0x01010101U
#define STOS stosl
#define MOVS movsl
#elif defined (__x86_64__)
#define SPRAY_VAL 0x0101010101010101UL
#define STOS stosq
#define MOVS movsq
#endif

#define ARCH_SIZE (sizeof(void *))

static inline void *memset(void *s, int c, size_t n) {
    unsigned long d0;

    asm volatile(
        "mov %1, %%" STR(_ASM_CX) "\n"
        "rep stosb\n"
        "mov %2, %%" STR(_ASM_CX) "\n"
        "rep " STR(STOS) "\n"
    : "=&D" (d0)
    : "ir" (n & (ARCH_SIZE - 1)),
      "ir" (n / ARCH_SIZE),
      "0" (s),
      "a" (SPRAY_VAL * c)
    : STR(_ASM_CX), "memory"
    );

   return s;
}

static inline void *memcpy(void *d, void *s, size_t n) {
    unsigned long d0;

    asm volatile(
        "mov %2, %%" STR(_ASM_CX) "\n"
        "rep movsb \n"
        "mov %3, %%" STR(_ASM_CX) "\n"
        "rep " STR(MOVS) "\n"
    : "=&D" (d0), "+&S" (s)
    : "ir" (n & (ARCH_SIZE - 1)),
      "ir" (n / ARCH_SIZE),
      "0" (d)
    : STR(_ASM_CX), "memory"
    );

   return d;
}

static inline char *strcpy(char *d, const char *s) {
    int d0, d1, d2;

    asm volatile ("1: lodsb \n"
                  "   stosb \n"
                  "   testb %%al,%%al \n"
                  "   jne 1b"
    : "=&S" (d0), "=&D" (d1), "=&a" (d2)
    : "0" (s), "1" (d)
    : "memory");

    return d;
}

/* External declarations */

extern unsigned long strtoul(const char *nptr, char **endptr, int base);
extern long strtol(const char *nptr, char **endptr, int base);
extern int vsnprintf(char *str, size_t size, char const *fmt, va_list ap);
extern void snprintf(char *buf, size_t size, const char *fmt, ...);

#endif /* KTF_STRING_H */
