#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_DISPATCH_1086_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_DISPATCH_1086_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_1086_OK = 0,
    PR_U38_1086_ARGUMENT,
    PR_U38_1086_SELECT_ABORT,
    PR_U38_1086_DISPATCH_ABORT
} pr_u38_1086_status;

/* Enters +0FAC with its near-call frame. On success, this model applies
 * +0FAC's RETF 6 to the callback state. */
typedef int (*pr_u38_1086_dispatch)(void *context, uint8_t *ram,
                                     size_t ram_size, pr_u38_101d_cpu *cpu);

/* Complete 59-byte U38 public entry 35 at +1086..+10C1. The resident
 * +101D far call and the distinct +0FAC callee are callback boundaries. */
pr_u38_1086_status pr_u38_target_dispatch_1086(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, pr_u38_1086_dispatch dispatch,
    void *context);

#endif
