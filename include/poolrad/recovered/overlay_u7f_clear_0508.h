#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_CLEAR_0508_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_CLEAR_0508_H
#include "poolrad/recovered/overlay_u7f_record_subtract_0020.h"
typedef pr_u7f_0020_cpu pr_u7f_0508_cpu;
typedef enum {
    PR_U7F_0508_OK = 0, PR_U7F_0508_ARGUMENT,
    PR_U7F_0508_NODE_LIMIT, PR_U7F_0508_ITEM_LIMIT,
    PR_U7F_0508_TRANSFER
} pr_u7f_0508_status;
/* Complete U7F entry 2 [0508,05E0), after far CALL, no arguments.
 * CPU14 plus exact 1 MiB A20-off RAM; CPU must not overlap RAM. Loaded
 * caller/helper code and SYSTEM binding are immutable; writes must avoid code.
 * Initial IP=0508, SP in [0014,FFFC]. Nonzero budgets count visits before
 * +0520 (including the final null test) and +054D (shared across records).
 * Limits retain exact partial CPU/RAM before the declined instruction.
 * Changed internal return targets stop immediately with TRANSFER.
 * Word/LES bytes are physically contiguous; effective offsets wrap at 16 bits.
 * See docs/re/U7F_CLEAR_0508.md for evidence and native scope. */
pr_u7f_0508_status pr_u7f_clear_0508(
    uint8_t *ram, size_t size, pr_u7f_0508_cpu *cpu, uint16_t system_cs,
    size_t max_nodes, size_t max_items);
#endif
