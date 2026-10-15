#ifndef POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_SCALE_CLEAR_2D88_H
#define POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_SCALE_CLEAR_2D88_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_2D88_OK = 0,
    PR_U38_2D88_ARGUMENT,
    PR_U38_2D88_CALL_ABORT
} pr_u38_2d88_status;

/* GAME.OVR U38 public entry 107, +2D88..+2DE4 (92 bytes). The near call to
 * +101D is composed; its resident BA:2E8B far call uses select_target. */
pr_u38_2d88_status pr_u38_effect_scale_clear_2d88(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

#endif
