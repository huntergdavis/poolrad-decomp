#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_CLOCK_2AC1_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_CLOCK_2AC1_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_2AC1_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_2ac1_cpu;

typedef enum {
    PR_U1C_2AC1_LOADER = 0, /* 2B:002A, one word; RETF 2 */
    PR_U1C_2AC1_VALUE,      /* 2B:0025, one word; RETF 2, AX result */
    PR_U1C_2AC1_CLOCK       /* 8C:0025, two words; RETF 4 */
} pr_u1c_2ac1_call;

/* Called with the original full-width words still on the emulated stack.
 * The adapter supplies each callee's RAM/CPU effects; return 1 to continue.
 * The model performs the confirmed Pascal callee stack cleanup. */
typedef int (*pr_u1c_2ac1_external)(void *context, uint8_t *ram,
                                     size_t ram_size, pr_u1c_2ac1_cpu *cpu,
                                     pr_u1c_2ac1_call call);

typedef enum {
    PR_U1C_2AC1_OK = 0,
    PR_U1C_2AC1_ARGUMENT,
    PR_U1C_2AC1_CALL_ABORT
} pr_u1c_2ac1_status;

pr_u1c_2ac1_status pr_u1c_clock_2ac1(
    uint8_t *ram, size_t ram_size, pr_u1c_2ac1_cpu *cpu,
    pr_u1c_2ac1_external external, void *context);

#endif
