#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_COMPOSED_0FD2_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_COMPOSED_0FD2_H

#include "poolrad/recovered/overlay_u7f_roll_call_0fde.h"

typedef pr_u7f_0fde_cpu pr_u7f_0fd2_composed_cpu;

typedef enum {
    PR_U7F_0FD2_COMPOSED_RETURNED = 0,
    PR_U7F_0FD2_COMPOSED_ARGUMENT,
    PR_U7F_0FD2_COMPOSED_CALL_ABORT,
    PR_U7F_0FD2_COMPOSED_TRANSFER
} pr_u7f_0fd2_composed_status;

/* Compose original U7F +0FD2..+100F around the full-state B0:0048 hook.
 * The hook owns the stub, U9E +0DE3, SYSTEM, and RETF 4 effects. The
 * wrapper makes no claim that any supplied hook is the original callee. */
pr_u7f_0fd2_composed_status pr_u7f_roll_composed_0fd2(
    uint8_t *ram, size_t ram_size, pr_u7f_0fd2_composed_cpu *cpu,
    pr_u7f_0fde_roll roll, void *context);

#endif
