#ifndef SETUP_SETUP_H
#define SETUP_SETUP_H

#define KERN_STACK_SIZE (3 * PAGE_SIZE)

#ifndef __ASSEMBLY__
#include <page.h>

extern uint8_t kernel_stack[KERN_STACK_SIZE + 2 * PAGE_SIZE];
extern void arch_setup(void);

#define GET_KERN_STACK()    (&kernel_stack[KERN_STACK_SIZE])
#define GET_KERN_EX_STACK() (&kernel_stack[KERN_STACK_SIZE + PAGE_SIZE])
#define GET_KERN_EM_STACK() (&kernel_stack[KERN_STACK_SIZE + PAGE_SIZE])

#endif /* __ASSEMBLY__ */

#endif /* SETUP_SETUP_H */
