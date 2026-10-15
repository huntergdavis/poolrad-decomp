#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_cpu;

#endif
