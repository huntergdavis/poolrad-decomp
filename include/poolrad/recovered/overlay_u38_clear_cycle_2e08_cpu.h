#ifndef POOLRAD_RECOVERED_OVERLAY_U38_CLEAR_CYCLE_2E08_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_CLEAR_CYCLE_2E08_CPU_H
#include "poolrad/recovered/overlay_u38_helper_cluster_cpu.h"
typedef pr_u38c_cpu pr_u38_2e08_cpu;
typedef enum {
    PR_U38_2E08_RETURNED, PR_U38_2E08_ARGUMENT, PR_U38_2E08_CODE,
    PR_U38_2E08_CALLEE, PR_U38_2E08_TRANSFER
} pr_u38_2e08_status;
/* Complete [2E08,2E37), RETF 0Ah. Four raw +0000 calls use keys
 * 0Bh,35h,34h,37h in order; AH remains live. If DS:682A is zero after
 * them, set DS:6816=64h. All RAM aliases and CPU14 outputs are retained.
 * Exact 1 MiB A20-off flat RAM, entry IP2E08, SP000A..FFFC; immutable
 * caller/helper code, nonwrapping reached far-return frames, no MMIO,
 * faults or interrupts. CPU/status storage must be distinct from RAM
 * and one another. Invalid arguments reject before mutation.
 * Live near-call opcode/relative-target checks stop at CODE before CALL
 * (after PUSH AX/PUSH CS). CALLEE/TRANSFER preserve partial state and the
 * raw helper result. Optional callee_status starts at -1 (unreached).
 * Outer RETURNED retains the actual live RETF destination. No host cap is
 * needed: four finite helper calls, with all arithmetic flags defined. */
pr_u38_2e08_status pr_u38_clear_cycle_2e08_cpu(
    uint8_t *ram, size_t ram_size, pr_u38_2e08_cpu *cpu, int *callee_status);
#endif
