#ifndef POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_PREFIX_08AE_001F_H
#define POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_PREFIX_08AE_001F_H

#include <stdint.h>

/* Captured 16-bit CPU words in DOSBox debugger order. */
typedef struct pr_rec_res_08ae_001f_cpu {
    uint16_t cs, ip, ds, es, ss, sp, bp;
    uint16_t ax, bx, cx, dx, si, di, flags;
} pr_rec_res_08ae_001f_cpu;

/* RES_08AE:+001F..+0029, stopped before the far CALL at +002A.
 * local_word points to SS:[BP-6] at entry. The arithmetic callee and
 * its call-frame stores are outside this prefix. */
void pr_rec_res_08ae_001f_prepare_multiply(pr_rec_res_08ae_001f_cpu *cpu,
                                            const uint8_t local_word[2]);

#endif
