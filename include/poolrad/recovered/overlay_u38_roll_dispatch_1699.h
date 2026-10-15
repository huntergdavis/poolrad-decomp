#ifndef POOLRAD_RECOVERED_OVERLAY_U38_ROLL_DISPATCH_1699_H
#define POOLRAD_RECOVERED_OVERLAY_U38_ROLL_DISPATCH_1699_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_1699_OK = 0,
    PR_U38_1699_ARGUMENT,
    PR_U38_1699_ROLL_ABORT,
    PR_U38_1699_DISPATCH_ABORT
} pr_u38_1699_status;

/* Callback enters either the loaded roll far target at :0048 or U38:15EB.
 * A successful callback leaves the call-entry frame for this model to
 * return through using the measured RETF 4 or RETF 8 ABI. */
typedef int (*pr_u38_1699_call)(void *context, uint8_t *ram,
                                size_t ram_size, pr_u38_101d_cpu *cpu);

/* Complete U38 public entry 55, +1699..+16BB (34 original bytes). */
pr_u38_1699_status pr_u38_roll_dispatch_1699(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_1699_call call, void *context);

#endif
