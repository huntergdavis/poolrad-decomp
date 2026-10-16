#ifndef POOLRAD_RECOVERED_SYSTEM_RANDOM_FULLSTATE_11C0_H
#define POOLRAD_RECOVERED_SYSTEM_RANDOM_FULLSTATE_11C0_H

#include "poolrad/recovered/system_seed_fullstate_120f.h"

typedef pr_system_120f_cpu pr_system_11c0_cpu;

typedef enum {
    PR_SYSTEM_11C0_RETURNED = 0,
    PR_SYSTEM_11C0_ARGUMENT,
    PR_SYSTEM_11C0_CALLEE_FAILED,
    PR_SYSTEM_11C0_CALLEE_TRANSFER
} pr_system_11c0_status;

/* Execute SYSTEM +11C0..+11D5 around the merged +120F seed step.
 * Enter after a far CALL with return IP, CS, and range WORD on SS:SP.
 * Loaded CS:11C0..11D5 and CS:120F..1244 bytes must be original.
 * Code aliases caused by the near call or seed writes reject unchanged.
 * A changed near return IP stops at the callee transfer boundary. */
pr_system_11c0_status pr_system_random_fullstate_11c0(
    uint8_t *ram, size_t ram_size, pr_system_11c0_cpu *cpu);

#endif
