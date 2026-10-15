#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0885_MOV_DX_AX_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0885_MOV_DX_AX_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0885_MOV_DX_AX_OK = 0,
    PR_U3B0_0885_MOV_DX_AX_ARGUMENT
} pr_u3b0_0885_mov_dx_ax_status;

/* Execute only original +0885 MOV DX,AX on supplied +0883 successor state. */
pr_u3b0_0885_mov_dx_ax_status pr_u3b0_0885_mov_dx_ax(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
