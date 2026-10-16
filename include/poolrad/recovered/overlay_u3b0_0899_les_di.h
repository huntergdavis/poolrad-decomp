#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0899_LES_DI_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0899_LES_DI_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0899_LES_DI_OK = 0,
    PR_U3B0_0899_LES_DI_ARGUMENT
} pr_u3b0_0899_les_di_status;

/* Execute only original +0899 LES DI,SS:[BP-4] on a supplied +0898 successor state. */
pr_u3b0_0899_les_di_status pr_u3b0_0899_les_di(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
