#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_STATE_FLAGS_0F7E_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_STATE_FLAGS_0F7E_H
#include <stddef.h>
#include <stdint.h>
#define PR_U7F_0F7E_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_0f7e_cpu;
typedef enum { PR_U7F_0F7E_OK = 0, PR_U7F_0F7E_ARGUMENT } pr_u7f_0f7e_status;
/* Complete +0F7E..+0FD1. Enter after CALL; head-output far pointer at
 * SS:SP+4 and shared-dwords-output far pointer at SS:SP+8. RETF 8.
 * For disjoint state, output bytes report nonzero DS:6810/6812 and any
 * of seven dwords at DS:67F4. Ordered RAM aliases are retained. Some
 * malformed aliases prevent return in DOS and C; no host cap is added.
 * Logical AF is undefined and represented as zero. */
pr_u7f_0f7e_status pr_u7f_state_flags_0f7e(
    uint8_t *ram, size_t ram_size, pr_u7f_0f7e_cpu *cpu);
#endif
