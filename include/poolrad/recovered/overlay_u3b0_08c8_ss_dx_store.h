#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_08C8_SS_DX_STORE_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_08C8_SS_DX_STORE_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_08C8_SS_DX_STORE_OK = 0,
    PR_U3B0_08C8_SS_DX_STORE_ARGUMENT
} pr_u3b0_08c8_ss_dx_store_status;

/* Execute only original +08C8 MOV SS:[BP-2],DX on supplied taken state. */
pr_u3b0_08c8_ss_dx_store_status pr_u3b0_08c8_ss_dx_store(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
