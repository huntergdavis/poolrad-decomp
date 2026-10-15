#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_088F_ADD_AX_DX_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_088F_ADD_AX_DX_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_088F_ADD_AX_DX_OK = 0,
    PR_U3B0_088F_ADD_AX_DX_ARGUMENT
} pr_u3b0_088f_add_ax_dx_status;

/* Execute original +088F ADD AX,DX on a supplied +088E successor state. */
pr_u3b0_088f_add_ax_dx_status pr_u3b0_088f_add_ax_dx(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
