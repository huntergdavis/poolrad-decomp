#ifndef POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_WEIGHTED_408C_H
#define POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_WEIGHTED_408C_H

#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"

#define PR_U6C_408C_RAM_SIZE PR_U6C_3F31_RAM_SIZE
typedef pr_u6c_3f31_cpu pr_u6c_408c_cpu;

typedef enum {
    PR_U6C_408C_OK = 0,
    PR_U6C_408C_ARGUMENT,
    PR_U6C_408C_DIVIDE_ERROR,
    PR_U6C_408C_TRANSFER
} pr_u6c_408c_status;

/* Complete +408C..+4234, argument record at SS:SP+4. Weighted byte sum
 * uses that record; composed +3F31 receives the separate DS:5D92/5D94
 * active pointer. Real frames, register side effects and aliases retained.
 * DIVIDE_ERROR stops before IDIV 41FF/4221; TRANSFER exposes an altered
 * internal return. Arithmetic FLAGS are undefined only after successful
 * IDIV; clamp/fault flags are exact. A20 off, 16-bit offsets/SP, contiguous
 * wide operands, immutable code. OK uses actual RAM for RETF 4.
 */
pr_u6c_408c_status pr_u6c_record_weighted_408c(
    uint8_t *ram, size_t ram_size, pr_u6c_408c_cpu *cpu);

#endif
