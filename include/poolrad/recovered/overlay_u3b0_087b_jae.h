#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_087B_JAE_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_087B_JAE_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_087B_JAE_OK = 0,
    PR_U3B0_087B_JAE_ARGUMENT
} pr_u3b0_087b_jae_status;

/* Execute only original +087B JAE, stopping at +087D or +08BA. */
pr_u3b0_087b_jae_status pr_u3b0_087b_jae(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
