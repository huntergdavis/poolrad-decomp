#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_INPUT_STRING_0994_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_INPUT_STRING_0994_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_0994_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_0994_cpu;

typedef enum {
    PR_U1C_0994_LOADER = 0, /* 2B:002A, RETF 2 */
    PR_U1C_0994_WORD,       /* 2B:004D, RETF 4 */
    PR_U1C_0994_INPUT,      /* 709:09E3, RETF 8 */
    PR_U1C_0994_COPY,       /* AF8:0B25, RETF 10; one or two sites */
    PR_U1C_0994_STORE       /* 2B:005C, RETF 6 */
} pr_u1c_0994_call;

/* IP identifies the lcall. Arguments are at SS:SP before the far call.
 * The callback supplies all callee RAM/CPU effects; this wrapper performs
 * the independently pinned Pascal argument cleanup on successful return. */
typedef int (*pr_u1c_0994_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u1c_0994_cpu *cpu,
                                    pr_u1c_0994_call call);

typedef enum {
    PR_U1C_0994_OK = 0,
    PR_U1C_0994_ARGUMENT,
    PR_U1C_0994_CALL_ABORT
} pr_u1c_0994_status;

pr_u1c_0994_status pr_u1c_input_string_0994(
    uint8_t *ram, size_t ram_size, pr_u1c_0994_cpu *cpu,
    pr_u1c_0994_external external, void *context);

#endif
