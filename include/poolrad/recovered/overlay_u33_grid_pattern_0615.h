#ifndef POOLRAD_RECOVERED_OVERLAY_U33_GRID_PATTERN_0615_H
#define POOLRAD_RECOVERED_OVERLAY_U33_GRID_PATTERN_0615_H
#include "poolrad/recovered/overlay_u33_cell_pair_01ad.h"

typedef pr_u33_01ad_cpu pr_u33_0615_cpu;
typedef enum {
    PR_U33_0615_OK = 0,
    PR_U33_0615_ARGUMENT,
    PR_U33_0615_CODE_ALIAS,
    PR_U33_0615_CALLEE,
    PR_U33_0615_TRANSFER
} pr_u33_0615_status;

/* Complete U33 [0615,0810), with fixed +01AD/+012B/U3BD lookup and +0221
 * callees. Enter at IP=0615, SP in [0080,FFFA]; SS:SP+4 is an inherited
 * frame offset. Exactly 1 MiB synchronous RAM and distinct stable CPU/RAM.
 * Loaded code/stubs must be original and immutable; all data writes must
 * avoid code. Reserved caller-segment stack/code aliases reject atomically.
 * Live frame, grid and table aliases are preserved. Actual RETF2 exposed.
 * CALLEE/TRANSFER preserve reached state without substituting scalar results.
 * Undefined logical AF follows the dependencies' zero profile. No hooks,
 * artificial cycle cap or resume API. See U33_GRID_PATTERN_0615.md.
 */
pr_u33_0615_status pr_u33_grid_pattern_0615(
    uint8_t *ram, size_t ram_size, pr_u33_0615_cpu *cpu);
#endif
