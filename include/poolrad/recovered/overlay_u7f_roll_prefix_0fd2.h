#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_PREFIX_0FD2_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_PREFIX_0FD2_H

#include <stddef.h>
#include <stdint.h>

#define PR_U7F_0FD2_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_0fd2_cpu;

typedef enum {
    PR_U7F_0FD2_AT_CALL = 0,
    PR_U7F_0FD2_ARGUMENT
} pr_u7f_0fd2_status;

/* Execute original +0FD2..+0FDD and stop before LCALL B0:0048 at +0FDE.
 * The caller owns that far call and all callee effects. */
pr_u7f_0fd2_status pr_u7f_roll_prefix_0fd2(
    uint8_t *ram, size_t ram_size, pr_u7f_0fd2_cpu *cpu);

#endif
