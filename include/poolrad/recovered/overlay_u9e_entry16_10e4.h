#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY16_10E4_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY16_10E4_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_10E4_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_10e4_cpu;

typedef enum {
    PR_U9E_10E4_OK = 0,
    PR_U9E_10E4_ARGUMENT
} pr_u9e_10e4_status;

/* Execute all of GAME.OVR U9E entry 16, +10E4..+110C, from an entry frame.
 * SS:SP holds far return IP, CS, first word argument, second word argument.
 * The original RETF 4 removes both arguments. */
pr_u9e_10e4_status pr_u9e_entry16_10e4(
    uint8_t *ram, size_t ram_size, pr_u9e_10e4_cpu *cpu);

#endif
