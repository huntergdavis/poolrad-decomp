#ifndef POOLRAD_RECOVERED_OVERLAY_U38_LEAF_2002_H
#define POOLRAD_RECOVERED_OVERLAY_U38_LEAF_2002_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38_2002_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_2002_cpu;

typedef enum {
    PR_U38_2002_OK = 0,
    PR_U38_2002_ARGUMENT
} pr_u38_2002_status;

/* Execute original GAME.OVR U38 +2002..+20DE from its far-return frame. */
pr_u38_2002_status pr_u38_leaf_2002(
    uint8_t *ram, size_t ram_size, pr_u38_2002_cpu *cpu);

#endif
