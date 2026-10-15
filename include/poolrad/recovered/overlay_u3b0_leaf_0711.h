#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_LEAF_0711_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_LEAF_0711_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0711_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0711_cpu;

typedef enum {
    PR_U3B0_0711_OK = 0,
    PR_U3B0_0711_ARGUMENT
} pr_u3b0_0711_status;

/* Execute complete GAME.OVR U3B0 +0711..+07A3 from its near-call frame. */
pr_u3b0_0711_status pr_u3b0_leaf_0711(
    uint8_t *ram, size_t ram_size, pr_u3b0_0711_cpu *cpu);

#endif
