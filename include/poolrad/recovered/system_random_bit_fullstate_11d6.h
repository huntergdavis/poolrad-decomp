#ifndef POOLRAD_RECOVERED_SYSTEM_RANDOM_BIT_FULLSTATE_11D6_H
#define POOLRAD_RECOVERED_SYSTEM_RANDOM_BIT_FULLSTATE_11D6_H

#include "poolrad/recovered/system_seed_fullstate_120f.h"

typedef pr_system_120f_cpu pr_system_11d6_cpu;

typedef enum {
    PR_SYSTEM_11D6_RETURNED = 0,
    PR_SYSTEM_11D6_ARGUMENT,
    PR_SYSTEM_11D6_CALLEE_FAILED,
    PR_SYSTEM_11D6_CALLEE_TRANSFER
} pr_system_11d6_status;

/* Execute pinned START.EXE SYSTEM +11D6..+11F2 on live A20-off RAM.
 * The far caller's IP/CS are read from SS:SP after the near seed call.
 * Loaded CS:+11D6..+11F2 and CS:+120F..+1244 must be original code. */
pr_system_11d6_status pr_system_random_bit_fullstate_11d6(
    uint8_t *ram, size_t ram_size, pr_system_11d6_cpu *cpu);

#endif
