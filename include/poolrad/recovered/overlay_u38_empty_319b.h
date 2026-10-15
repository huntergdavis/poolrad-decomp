#ifndef POOLRAD_RECOVERED_OVERLAY_U38_EMPTY_319B_H
#define POOLRAD_RECOVERED_OVERLAY_U38_EMPTY_319B_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef pr_u38_101d_cpu pr_u38_319b_cpu;

typedef enum {
    PR_U38_319B_OK = 0,
    PR_U38_319B_ARGUMENT
} pr_u38_319b_status;

/* Complete GAME.OVR U38 public entry 116, +319B..+31A2. */
pr_u38_319b_status pr_u38_empty_319b(
    uint8_t *ram, size_t ram_size, pr_u38_319b_cpu *cpu);

#endif
