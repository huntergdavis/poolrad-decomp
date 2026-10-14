#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_GOTO_00F4_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_GOTO_00F4_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_00F4_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_00f4_cpu;

typedef enum {
    PR_U1C_00F4_LOADER = 0, /* 2B:002A, RETF 2 */
    PR_U1C_00F4_WORD      /* 2B:004D, RETF 4 */
} pr_u1c_00f4_call;

/* Call-site hook: IP is the lcall and arguments are on SS:SP. The hook
 * supplies callee effects; this wrapper performs the confirmed cleanup. */
typedef int (*pr_u1c_00f4_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u1c_00f4_cpu *cpu,
                                    pr_u1c_00f4_call call);

typedef enum {
    PR_U1C_00F4_OK = 0,
    PR_U1C_00F4_ARGUMENT,
    PR_U1C_00F4_CALL_ABORT
} pr_u1c_00f4_status;

pr_u1c_00f4_status pr_u1c_goto_00f4(
    uint8_t *ram, size_t ram_size, pr_u1c_00f4_cpu *cpu,
    pr_u1c_00f4_external external, void *context);

#endif
