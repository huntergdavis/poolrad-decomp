#ifndef POOLRAD_RECOVERED_OVERLAY_U7D_CARRY_02F2_H
#define POOLRAD_RECOVERED_OVERLAY_U7D_CARRY_02F2_H
#include <stddef.h>
#include <stdint.h>
#define PR_U7D_02F2_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7d_02f2_cpu;
typedef enum { PR_U7D_02F2_OK = 0, PR_U7D_02F2_ARGUMENT } pr_u7d_02f2_status;
/* Complete +02F2..+03A2, entered after CALL, far array pointer at SS:SP+4.
 * RETF 4. Normalize seven words against DS:363A, advancing the next word
 * for carries 0..5; carry 6 increments party member words at +30.
 * Models shared 1 MiB A20-off RAM and all returned CPU words. Cyclic or
 * malformed alias states can fail to return in both DOS and C; no cap.
 * Only terminating source paths are claimed. */
pr_u7d_02f2_status pr_u7d_carry_02f2(
    uint8_t *ram, size_t ram_size, pr_u7d_02f2_cpu *cpu);
#endif
