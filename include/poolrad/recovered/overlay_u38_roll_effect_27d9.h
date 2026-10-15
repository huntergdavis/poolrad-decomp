#ifndef POOLRAD_RECOVERED_OVERLAY_U38_ROLL_EFFECT_27D9_H
#define POOLRAD_RECOVERED_OVERLAY_U38_ROLL_EFFECT_27D9_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_27D9_OK = 0,
    PR_U38_27D9_ARGUMENT,
    PR_U38_27D9_ROLL_ABORT,
    PR_U38_27D9_EFFECT_ABORT
} pr_u38_27d9_status;

/* Callback enters either loaded far target with its call frame intact.
 * On success, this model applies the respective RETF 4 or RETF 12. */
typedef int (*pr_u38_27d9_call)(void *context, uint8_t *ram,
                                 size_t ram_size, pr_u38_101d_cpu *cpu);

/* Complete U38 public entry 84, +27D9..+2814 (59 bytes). */
pr_u38_27d9_status pr_u38_roll_effect_27d9(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_27d9_call roll, pr_u38_27d9_call effect, void *context);

#endif
