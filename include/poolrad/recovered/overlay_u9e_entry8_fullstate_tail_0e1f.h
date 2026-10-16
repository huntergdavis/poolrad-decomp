#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_TAIL_0E1F_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_TAIL_0E1F_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_ENTRY8_TAIL_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_entry8_tail_cpu;

typedef enum {
    PR_U9E_ENTRY8_TAIL_RETURNED = 0,
    PR_U9E_ENTRY8_TAIL_ARGUMENT
} pr_u9e_entry8_tail_status;

/* Execute original U9E entry8 +0E1F..+0E2D from a supplied live state.
 * RETF 4 reads the caller frame through 16-bit-wrapped SS offsets.
 * Loaded CS:+0DE3..+0E2D code must be the pinned original body. */
pr_u9e_entry8_tail_status pr_u9e_entry8_fullstate_tail_0e1f(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_tail_cpu *cpu);

#endif
