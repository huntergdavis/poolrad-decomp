#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_RETURN_0FE3_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_RETURN_0FE3_H

#include <stddef.h>
#include <stdint.h>

#define PR_U7F_0FE3_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_0fe3_cpu;

typedef enum {
    PR_U7F_0FE3_RETURNED = 0,
    PR_U7F_0FE3_ARGUMENT
} pr_u7f_0fe3_status;

/* Resume at original +0FE3 with the live post-callee RAM and CPU state. */
pr_u7f_0fe3_status pr_u7f_roll_return_0fe3(
    uint8_t *ram, size_t ram_size, pr_u7f_0fe3_cpu *cpu);

#endif
