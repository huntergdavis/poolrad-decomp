#ifndef POOLRAD_RECOVERED_OVERLAY_U7D_UPDATE_047A_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7D_UPDATE_047A_CPU_H
#include "poolrad/recovered/overlay_u7d_update_03a3.h"
typedef pr_u7d_03a3_cpu pr_u7d_047a_cpu;
typedef enum {
    PR_U7D_047A_RETURNED, PR_U7D_047A_ARGUMENT, PR_U7D_047A_CODE,
    PR_U7D_047A_CALLEE, PR_U7D_047A_TRANSFER, PR_U7D_047A_LIMIT
} pr_u7d_047a_status;
typedef struct { int callee, status; } pr_u7d_047a_diagnostic;
/* Complete [047A,0563), RETF 4. Exact 1 MiB A20-off flat RAM, entry IP
 * 047A, SP 0020..FFF8, immutable caller/callee code and nonwrapping reached
 * return frames. CPU/diagnostic storage must not overlap RAM or one another. Arguments at
 * entry SP+4/+6 are index/amount words; only their low bytes are consumed.
 * max_steps counts caller instructions, stopping BEFORE the next instruction.
 * Delegated +03A3/+02F2 retain their terminating-input precondition and have
 * no traversal cap. Live RAM aliases are preserved. Callee 0=FillChar, 1=03A3,
 * -1=unreached; raw status is retained. CODE stops before a changed call.
 * TRANSFER retains a successful callee's unexpected live return state.
 * Logical/shift undefined AF uses zero; see the scoped evidence profile.
 * No IRQ, MMIO, faults, or self-modifying code. See evidence document. */
pr_u7d_047a_status pr_u7d_update_047a_cpu(
    uint8_t *ram, size_t ram_size, pr_u7d_047a_cpu *cpu,
    unsigned max_steps, pr_u7d_047a_diagnostic *diagnostic);
#endif
