#ifndef POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_MATCH_12B2_H
#define POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_MATCH_12B2_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_12B2_OK = 0,
    PR_U38_12B2_ARGUMENT
} pr_u38_12b2_status;

/* GAME.OVR U38 public entry 41, +12B2..+12FB (73 bytes). The conditional
 * near call at +12E8 composes the verified U38 +0C06 effect leaf and its
 * actual far return frame. */
pr_u38_12b2_status pr_u38_effect_match_12b2(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu);

#endif
