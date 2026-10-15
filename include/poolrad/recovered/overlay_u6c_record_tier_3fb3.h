#ifndef POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_TIER_3FB3_H
#define POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_TIER_3FB3_H

#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"

#define PR_U6C_3FB3_RAM_SIZE PR_U6C_3F31_RAM_SIZE
typedef pr_u6c_3f31_cpu pr_u6c_3fb3_cpu;

typedef enum {
    PR_U6C_3FB3_OK = 0,
    PR_U6C_3FB3_ARGUMENT,
    PR_U6C_3FB3_DIVIDE_ERROR,
    PR_U6C_3FB3_TRANSFER
} pr_u6c_3fb3_status;

/* Complete +3FB3..+408B with merged +3F31 composed as the real near CALL.
 * Far record argument at SS:SP+4; byte count/sum, signed callee adjustment,
 * clamp or signed division, partial registers, locals and aliases retained.
 * DIVIDE_ERROR stops before faulting IDIV, with IP at 4056/4078; no CPU
 * exception-vector frame is synthesized. TRANSFER exposes an internal
 * callee's altered CS:IP. OK performs RETF 4 through actual RAM.
 * Arithmetic FLAGS (08D5) are undefined after IDIV; clamp/fault FLAGS are
 * defined and exact. A20 off; 16-bit offsets/SP, contiguous wide operands,
 * immutable code. No original instruction bytes or callback input needed.
 */
pr_u6c_3fb3_status pr_u6c_record_tier_3fb3(
    uint8_t *ram, size_t ram_size, pr_u6c_3fb3_cpu *cpu);

#endif
