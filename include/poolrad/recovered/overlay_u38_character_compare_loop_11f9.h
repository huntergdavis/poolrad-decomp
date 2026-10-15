#ifndef POOLRAD_RECOVERED_OVERLAY_U38_CHARACTER_COMPARE_LOOP_11F9_H
#define POOLRAD_RECOVERED_OVERLAY_U38_CHARACTER_COMPARE_LOOP_11F9_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_11F9_OK = 0,
    PR_U38_11F9_ARGUMENT
} pr_u38_11f9_status;

/* GAME.OVR U38 public entry 39, +11F9..+1276 (125 bytes). The reachable
 * instructions end at +1268; the remaining bytes are adjacent constants.
 * The loaded SYSTEM AF8:0C10 comparison is composed on each loop iteration. */
pr_u38_11f9_status pr_u38_character_compare_loop_11f9(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu);

#endif
