#ifndef POOLRAD_RECOVERED_OVERLAY_U38_CLEAR_CYCLE_2E08_H
#define POOLRAD_RECOVERED_OVERLAY_U38_CLEAR_CYCLE_2E08_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef pr_u38_101d_cpu pr_u38_2e08_cpu;

typedef enum {
    PR_U38_2E08_OK = 0,
    PR_U38_2E08_ARGUMENT
} pr_u38_2e08_status;

pr_u38_2e08_status pr_u38_clear_cycle_2e08(
    uint8_t *ram, size_t ram_size, pr_u38_2e08_cpu *cpu);

#endif
