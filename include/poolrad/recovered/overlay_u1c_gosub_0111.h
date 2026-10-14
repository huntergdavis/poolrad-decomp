#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_GOSUB_0111_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_GOSUB_0111_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_0111_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_0111_cpu;

typedef enum {
    PR_U1C_0111_LOADER = 0, /* 2B:002A, RETF 2 */
    PR_U1C_0111_PUSH_FRAME  /* 2B:0075, RETF 2 */
} pr_u1c_0111_call;

/* Call-site hook with the argument on SS:SP and IP at the lcall. The hook
 * supplies all callee RAM/CPU effects; the model performs RETF 2 cleanup. */
typedef int (*pr_u1c_0111_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u1c_0111_cpu *cpu,
                                    pr_u1c_0111_call call);

typedef enum {
    PR_U1C_0111_OK = 0,
    PR_U1C_0111_ARGUMENT,
    PR_U1C_0111_CALL_ABORT
} pr_u1c_0111_status;

pr_u1c_0111_status pr_u1c_gosub_0111(
    uint8_t *ram, size_t ram_size, pr_u1c_0111_cpu *cpu,
    pr_u1c_0111_external external, void *context);

#endif
