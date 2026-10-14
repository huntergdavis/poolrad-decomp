#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_RETURN_0A9F_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_RETURN_0A9F_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_0A9F_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_0a9f_cpu;

/* Runtime AF8:0254 FreeMem. The callback sees (segment, offset, size=6)
 * at SS:SP and supplies heap/CPU effects; return 1 to continue. The model
 * performs its confirmed RETF 6 stack cleanup. */
typedef int (*pr_u1c_0a9f_free)(void *context, uint8_t *ram,
                                 size_t ram_size, pr_u1c_0a9f_cpu *cpu);

typedef enum {
    PR_U1C_0A9F_OK = 0,
    PR_U1C_0A9F_ARGUMENT,
    PR_U1C_0A9F_CALL_ABORT
} pr_u1c_0a9f_status;

pr_u1c_0a9f_status pr_u1c_return_0a9f(
    uint8_t *ram, size_t ram_size, pr_u1c_0a9f_cpu *cpu,
    pr_u1c_0a9f_free free_node, void *context);

#endif
