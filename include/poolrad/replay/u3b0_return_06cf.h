#ifndef POOLRAD_REPLAY_U3B0_RETURN_06CF_H
#define POOLRAD_REPLAY_U3B0_RETURN_06CF_H
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_U3B0_RETURN_LOOP = 0, /* stopped at +048E; no loop effects */
    PR_U3B0_RETURN_DONE,     /* this restricted epilogue executed RETF 16h */
    PR_U3B0_RETURN_ARGUMENT,
    PR_U3B0_RETURN_TRANSFER  /* stopped at RETF; prior effects persist */
} pr_u3b0_return_status;

typedef struct { uint16_t cs, ip; } pr_u3b0_return_target;

/* Entry +06CF on caller-owned 1 MiB A20-off RAM/CPU from a live prompt
 * frame. Source identity, previous prompt effects, loaded code stability and
 * expected return target are caller supplied, not proven here. The target is
 * checked at RETF after earlier effects; mismatch stops at +06F8, preserving
 * partial effects. A successful epilogue does not prove prior prompt execution
 * or set any entry2 hook's provided flag. No continuation after LOOP/TRANSFER
 * is supplied. Guest memory is never rolled back. */
pr_u3b0_return_status pr_u3b0_return_06cf(uint8_t *ram, size_t ram_size,
    pr_r0899_cpu *cpu, const pr_u3b0_return_target *target);
#endif
