#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_RESET_2B7D_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_RESET_2B7D_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_2B7D_OK = 0,
    PR_U38_2B7D_ARGUMENT,
    PR_U38_2B7D_CALL_ABORT
} pr_u38_2b7d_status;

/* Complete U38 public entry 103. The selector callback supplies the
 * effects of the merged +101D helper's resident far-call dependency. */
pr_u38_2b7d_status pr_u38_target_reset_2b7d(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

#endif
