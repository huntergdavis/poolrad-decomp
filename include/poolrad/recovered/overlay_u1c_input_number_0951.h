#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_INPUT_NUMBER_0951_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_INPUT_NUMBER_0951_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_0951_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_0951_cpu;

typedef enum {
    PR_U1C_0951_LOADER = 0, /* 2B:002A, RETF 2 */
    PR_U1C_0951_WORD,       /* 2B:004D, RETF 4 */
    PR_U1C_0951_INPUT,      /* 709:0B66, RETF 6 */
    PR_U1C_0951_STORE       /* 2B:0052, RETF 4 */
} pr_u1c_0951_call;

/* Call-site hook with IP at the lcall and arguments on SS:SP. The hook
 * supplies all callee RAM/CPU effects; this wrapper performs the confirmed
 * Pascal argument cleanup after successful return. */
typedef int (*pr_u1c_0951_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u1c_0951_cpu *cpu,
                                    pr_u1c_0951_call call);

typedef enum {
    PR_U1C_0951_OK = 0,
    PR_U1C_0951_ARGUMENT,
    PR_U1C_0951_CALL_ABORT
} pr_u1c_0951_status;

pr_u1c_0951_status pr_u1c_input_number_0951(
    uint8_t *ram, size_t ram_size, pr_u1c_0951_cpu *cpu,
    pr_u1c_0951_external external, void *context);

#endif
