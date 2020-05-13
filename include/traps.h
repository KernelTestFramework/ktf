#ifndef TRAPS_TRAPS_H
#define TRAPS_TRAPS_H

#define X86_RET2KERN_INT 32

#ifndef __ASSEMBLY__

extern void init_traps(void);

#endif /* __ASSEMBLY__ */

#endif /* TRAPS_TRAPS_H */
