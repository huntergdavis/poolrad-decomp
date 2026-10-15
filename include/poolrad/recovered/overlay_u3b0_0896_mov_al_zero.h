#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0896_MOV_AL_ZERO_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0896_MOV_AL_ZERO_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0896_MOV_AL_ZERO_OK = 0,
    PR_U3B0_0896_MOV_AL_ZERO_ARGUMENT
} pr_u3b0_0896_mov_al_zero_status;

/* Execute original +0896 MOV AL,0 on a supplied +0895 successor state. */
pr_u3b0_0896_mov_al_zero_status pr_u3b0_0896_mov_al_zero(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
