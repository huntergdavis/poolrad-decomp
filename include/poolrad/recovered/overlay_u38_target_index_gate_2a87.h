#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_INDEX_GATE_2A87_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_INDEX_GATE_2A87_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_2A87_OK = 0,
    PR_U38_2A87_ARGUMENT,
    PR_U38_2A87_CALL_ABORT
} pr_u38_2a87_status;

/* Complete U38 public entry 99. select_target handles the unresolved
 * resident far call made by the provisional +101D dependency. */
pr_u38_2a87_status pr_u38_target_index_gate_2a87(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

#endif
