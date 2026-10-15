#ifndef POOLRAD_RECOVERED_OVERLAY_U38_CHARACTER_COMPARE_1276_H
#define POOLRAD_RECOVERED_OVERLAY_U38_CHARACTER_COMPARE_1276_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_1276_OK = 0,
    PR_U38_1276_ARGUMENT
} pr_u38_1276_status;

/* GAME.OVR U38 public entry 40, +1276..+12B2 (60 bytes), composed with the
 * original SYSTEM AF8:0C10 Pascal-string comparator on both call paths.
 * Caller data and the two adjacent game constants are read from RAM. */
pr_u38_1276_status pr_u38_character_compare_1276(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu);

#endif
