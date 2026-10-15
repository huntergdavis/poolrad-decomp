#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_RESET_CALL_133F_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_RESET_CALL_133F_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_133F_OK = 0,
    PR_U38_133F_ARGUMENT,
    PR_U38_133F_CALL_ABORT
} pr_u38_133f_status;

/* The callback enters the loaded BA:2637 resident target-reset routine with
 * its far return address and character far pointer on SS:SP. On success, the
 * wrapper applies that routine's RETF 4 to the callback state. */
typedef int (*pr_u38_133f_reset)(void *context, uint8_t *ram,
                                  size_t ram_size, pr_u38_101d_cpu *cpu);

/* Complete GAME.OVR U38 public entry 43, +133F..+1355 (22 bytes). */
pr_u38_133f_status pr_u38_target_reset_call_133f(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_133f_reset reset, void *context);

#endif
