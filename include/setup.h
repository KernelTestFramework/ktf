#ifndef SETUP_SETUP_H
#define SETUP_SETUP_H

#include <ktf.h>
#include <page.h>

extern uint8_t kernel_stack[3 * PAGE_SIZE];

#define GET_KERN_STACK()    (&kernel_stack[1 * PAGE_SIZE])
#define GET_KERN_EX_STACK() (&kernel_stack[2 * PAGE_SIZE])
#define GET_KERN_EM_STACK() (&kernel_stack[3 * PAGE_SIZE])

#endif /* SETUP_SETUP_H */
