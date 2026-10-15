#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_LEAF_0067_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_LEAF_0067_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0067_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0067_cpu;

typedef enum {
    PR_U3B0_0067_OK = 0,
    PR_U3B0_0067_ARGUMENT
} pr_u3b0_0067_status;

/* Execute original GAME.OVR U3B0 +0067..+00AA from its far-return frame. */
pr_u3b0_0067_status pr_u3b0_leaf_0067(
    uint8_t *ram, size_t ram_size, pr_u3b0_0067_cpu *cpu);

#endif
