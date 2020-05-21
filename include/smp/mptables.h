#ifndef KTF_MPTABLES_H
#define KTF_MPTABLES_H

#include <ktf.h>
#include <lib.h>

#define MPF_SIGNATURE (('_' << 24) | ('P' << 16) | ('M' << 8) | ('_'))

struct mpf {
    uint32_t signature;
    uint32_t mpc_base;
    uint8_t length;
    uint8_t spec_rev;
    uint8_t checksum;
    uint8_t mpc_type;
    uint8_t rsvd0:6, imcrp:1;
    uint8_t rsvd1[3];
} __packed;
typedef struct mpf mpf_t;

/* External declarations */

extern void mptables_init(void);

#endif /* KTF_MPTABLES_H */
