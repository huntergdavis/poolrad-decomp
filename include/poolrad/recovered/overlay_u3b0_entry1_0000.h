#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY1_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY1_0000_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0000_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0000_cpu;

typedef enum {
    PR_U3B0_0000_OK = 0,
    PR_U3B0_0000_ARGUMENT
} pr_u3b0_0000_status;

/* Execute GAME.OVR U3B0 public entry 1, +0000..+0066.
 * SS:SP holds far return IP/CS, head far pointer, then a count word.
 * DX:AX is the selected far pointer; RETF 6 removes the arguments. */
pr_u3b0_0000_status pr_u3b0_entry1_0000(
    uint8_t *ram, size_t ram_size, pr_u3b0_0000_cpu *cpu);

#endif
