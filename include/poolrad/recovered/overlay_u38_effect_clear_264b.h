#ifndef POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_CLEAR_264B_H
#define POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_CLEAR_264B_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_264B_OK = 0,
    PR_U38_264B_ARGUMENT,
    PR_U38_264B_CALL_ABORT
} pr_u38_264b_status;

/* GAME.OVR U38 public entry 80, +264B..+268A (63 bytes). The near call to
 * +101D is composed; its resident BA:2E8B far call uses select_target. */
pr_u38_264b_status pr_u38_effect_clear_264b(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

#endif
