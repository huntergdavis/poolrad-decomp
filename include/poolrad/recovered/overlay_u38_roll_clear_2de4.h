#ifndef POOLRAD_RECOVERED_OVERLAY_U38_ROLL_CLEAR_2DE4_H
#define POOLRAD_RECOVERED_OVERLAY_U38_ROLL_CLEAR_2DE4_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef pr_u38_101d_cpu pr_u38_2de4_cpu;

/* Enter the loaded :0048 roll stub with the original far-call frame.
 * A successful callback executes its RETF 4 and returns at caller CS:2DF2. */
typedef int (*pr_u38_2de4_roll)(void *context, uint8_t *ram,
                                 size_t ram_size, pr_u38_2de4_cpu *cpu);

typedef enum {
    PR_U38_2DE4_OK = 0,
    PR_U38_2DE4_ARGUMENT,
    PR_U38_2DE4_ROLL_ABORT
} pr_u38_2de4_status;

pr_u38_2de4_status pr_u38_roll_clear_2de4(
    uint8_t *ram, size_t ram_size, pr_u38_2de4_cpu *cpu,
    pr_u38_2de4_roll roll, void *context);

#endif
