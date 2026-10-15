#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY6_1140_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY6_1140_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_1140_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_1140_cpu;

/* Called with IP at +114A or +11A8 and 0x002E at SS:SP. The callback
 * supplies all allocator RAM/CPU effects, including returned DX:AX.
 * This wrapper performs the pinned RETF 2 cleanup after success. */
typedef int (*pr_u3b0_1140_alloc)(void *context, uint8_t *ram,
                                  size_t ram_size, pr_u3b0_1140_cpu *cpu);

typedef enum {
    PR_U3B0_1140_OK = 0,
    PR_U3B0_1140_ARGUMENT,
    PR_U3B0_1140_ALLOC_ABORT
} pr_u3b0_1140_status;

pr_u3b0_1140_status pr_u3b0_entry6_1140(
    uint8_t *ram, size_t ram_size, pr_u3b0_1140_cpu *cpu,
    pr_u3b0_1140_alloc alloc, void *context);

#endif
