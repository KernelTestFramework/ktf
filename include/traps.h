#ifndef TRAPS_TRAPS_H
#define TRAPS_TRAPS_H

#define MAX_INT 256

#define X86_RET2KERN_INT 32

#ifndef __ASSEMBLY__

extern void init_traps(unsigned int cpu);

#endif /* __ASSEMBLY__ */

#endif /* TRAPS_TRAPS_H */
