#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_GATE_2AE6_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_GATE_2AE6_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_2AE6_OK = 0,
    PR_U38_2AE6_ARGUMENT,
    PR_U38_2AE6_CALL_ABORT
} pr_u38_2ae6_status;

/* Complete U38 public entry 100. select_target handles the unresolved
 * resident far call made by the provisional +101D dependency. */
pr_u38_2ae6_status pr_u38_target_gate_2ae6(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

#endif
