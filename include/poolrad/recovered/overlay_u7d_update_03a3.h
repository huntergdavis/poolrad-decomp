#ifndef POOLRAD_RECOVERED_OVERLAY_U7D_UPDATE_03A3_H
#define POOLRAD_RECOVERED_OVERLAY_U7D_UPDATE_03A3_H

#include "poolrad/recovered/overlay_u7d_carry_02f2.h"

#define PR_U7D_03A3_RAM_SIZE PR_U7D_02F2_RAM_SIZE
typedef pr_u7d_02f2_cpu pr_u7d_03a3_cpu;

typedef enum {
    PR_U7D_03A3_OK = 0,
    PR_U7D_03A3_ARGUMENT,
    PR_U7D_03A3_CALLEE_FAILED,
    PR_U7D_03A3_CALLEE_TRANSFER
} pr_u7d_03a3_status;

/* Complete GAME.OVR unit 0x7D:+03A3..+03D4, entered after a far CALL.
 * The sole +03AC near CALL composes the complete +02F2 carry C model on
 * shared 1 MiB A20-off RAM/CPU state. A return elsewhere from that callee
 * stops at its live transfer state. The original code is assumed immutable;
 * malformed/cyclic callee inputs can fail to return as in DOS. */
pr_u7d_03a3_status pr_u7d_update_03a3(
    uint8_t *ram, size_t ram_size, pr_u7d_03a3_cpu *cpu);

#endif
