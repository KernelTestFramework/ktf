#ifndef KTF_KTF_H
#define KTF_KTF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <limits.h>

#include <compiler.h>

typedef uint16_t io_port_t;

typedef int (*user_func_t)(void *arg);

extern bool opt_debug;

extern int usermode_call(user_func_t fn, void *fn_arg);

extern void kernel_main(void);
extern void test_main(void);

#endif /* KTF_KTF_H */
