#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0892_SS_AL_LOAD_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0892_SS_AL_LOAD_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0892_SS_AL_LOAD_OK = 0,
    PR_U3B0_0892_SS_AL_LOAD_ARGUMENT
} pr_u3b0_0892_ss_al_load_status;

/* Execute original +0892 MOV AL,SS:[BP-9] on supplied successor state. */
pr_u3b0_0892_ss_al_load_status pr_u3b0_0892_ss_al_load(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
