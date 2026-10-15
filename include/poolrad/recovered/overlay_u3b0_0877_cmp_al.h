#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0877_CMP_AL_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0877_CMP_AL_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0877_CMP_AL_OK = 0,
    PR_U3B0_0877_CMP_AL_ARGUMENT
} pr_u3b0_0877_cmp_al_status;

/* Execute only original +0877 SS-override CMP AL,[DI-54h] on supplied state with exact loaded bytes. */
pr_u3b0_0877_cmp_al_status pr_u3b0_0877_cmp_al(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
