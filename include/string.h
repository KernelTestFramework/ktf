#ifndef KTF_STRING_H
#define KTF_STRING_H

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

extern unsigned long strtoul(const char *nptr, char **endptr, int base);
extern long strtol(const char *nptr, char **endptr, int base);
extern int vsnprintf(char *str, size_t size, char const *fmt, va_list ap);

#endif /* KTF_STRING_H */
