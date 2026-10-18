#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY17_110D_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY17_110D_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_110D_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_110d_cpu;

typedef enum {
    PR_U9E_110D_OK = 0,
    PR_U9E_110D_ARGUMENT
} pr_u9e_110d_status;

/* Execute GAME.OVR U9E entry 17, +110D..+115B, from an entry frame.
 * SS:SP holds far return IP/CS, then A/B/C far-pointer arguments.
 * RETF 12 removes all three pointers. */
pr_u9e_110d_status pr_u9e_entry17_110d(
    uint8_t *ram, size_t ram_size, pr_u9e_110d_cpu *cpu);

#endif
