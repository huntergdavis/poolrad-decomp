#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_CALL_0FDE_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_CALL_0FDE_H

#include <stddef.h>
#include <stdint.h>

#define PR_U7F_0FDE_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_0fde_cpu;

/* At the original B0:0048 stub, after LCALL has pushed return CS/IP.
 * The hook owns stub dispatch, U9E +0DE3 and its SYSTEM calls, including
 * all RAM/CPU effects and the live RETF 4. Zero aborts at its current state. */
typedef int (*pr_u7f_0fde_roll)(void *context, uint8_t *ram,
                                size_t ram_size, pr_u7f_0fde_cpu *cpu);

typedef enum {
    PR_U7F_0FDE_RETURNED = 0,
    PR_U7F_0FDE_ARGUMENT,
    PR_U7F_0FDE_CALL_ABORT,
    PR_U7F_0FDE_TRANSFER
} pr_u7f_0fde_status;

/* Enter at U7F CS:+0FDE, after its two argument pushes. RETURNED means
 * the hook reached the original CS:+0FE3 continuation; TRANSFER leaves a
 * different live destination intact. No callee register/flag effects are
 * fabricated here. */
pr_u7f_0fde_status pr_u7f_roll_call_0fde(
    uint8_t *ram, size_t ram_size, pr_u7f_0fde_cpu *cpu,
    pr_u7f_0fde_roll roll, void *context);

#endif
