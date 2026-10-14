#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_COMPARE_AND_0AED_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_COMPARE_AND_0AED_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_0AED_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_0aed_cpu;

typedef enum {
    PR_U1C_0AED_FILL = 0,  /* AF8:16A8, RETF 8 */
    PR_U1C_0AED_LOADER,    /* 2B:002A, RETF 2 */
    PR_U1C_0AED_VALUE     /* 2B:0025, RETF 2; called four times */
} pr_u1c_0aed_call;

/* Called immediately before each far call, with IP at its call instruction
 * and arguments already on SS:SP. The hook supplies callee RAM/CPU effects;
 * this function performs the confirmed Pascal argument cleanup. */
typedef int (*pr_u1c_0aed_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u1c_0aed_cpu *cpu,
                                    pr_u1c_0aed_call call);

typedef enum {
    PR_U1C_0AED_OK = 0,
    PR_U1C_0AED_ARGUMENT,
    PR_U1C_0AED_CALL_ABORT,
    PR_U1C_0AED_LOOP_LIMIT
} pr_u1c_0aed_status;

pr_u1c_0aed_status pr_u1c_compare_and_0aed(
    uint8_t *ram, size_t ram_size, pr_u1c_0aed_cpu *cpu,
    pr_u1c_0aed_external external, void *context);

#endif
