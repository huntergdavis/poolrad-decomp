#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_MASK81_25BC_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_MASK81_25BC_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_25BC_OK = 0,
    PR_U38_25BC_ARGUMENT,
    PR_U38_25BC_CALL_ABORT
} pr_u38_25bc_status;

/* Reachable 73-byte prefix of U38 public entry 78; the 11-byte tail is data.
 * select_target handles the resident far call made by +101D. */
pr_u38_25bc_status pr_u38_target_mask81_25bc(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

#endif
