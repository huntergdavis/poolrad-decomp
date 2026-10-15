#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0874_DI_LOAD_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0874_DI_LOAD_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0874_DI_LOAD_OK = 0,
    PR_U3B0_0874_DI_LOAD_ARGUMENT
} pr_u3b0_0874_di_load_status;

/* Execute only original +0874 MOV DI,SS:[BP+6] on supplied CPU/RAM with exact loaded bytes. */
pr_u3b0_0874_di_load_status pr_u3b0_0874_di_load(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
