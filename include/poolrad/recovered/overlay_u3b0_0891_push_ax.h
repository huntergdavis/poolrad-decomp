#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0891_PUSH_AX_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0891_PUSH_AX_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0891_PUSH_AX_OK = 0,
    PR_U3B0_0891_PUSH_AX_ARGUMENT
} pr_u3b0_0891_push_ax_status;

/* Execute original +0891 PUSH AX on a supplied +088F successor state. */
pr_u3b0_0891_push_ax_status pr_u3b0_0891_push_ax(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
