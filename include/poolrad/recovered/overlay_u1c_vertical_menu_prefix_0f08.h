#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_VERTICAL_MENU_PREFIX_0F08_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_VERTICAL_MENU_PREFIX_0F08_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_0F08_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_0f08_cpu;

typedef enum {
    PR_U1C_0F08_LOADER = 0, /* 2B:002A, RETF 2; two sites */
    PR_U1C_0F08_WORD,       /* 2B:004D, RETF 4 */
    PR_U1C_0F08_COPY,       /* AF8:0B25, RETF 10 */
    PR_U1C_0F08_VALUE       /* 2B:0025, RETF 2 */
} pr_u1c_0f08_call;

/* IP identifies the lcall; its arguments are at SS:SP. Callback supplies
 * all callee RAM/CPU effects; wrapper performs pinned Pascal cleanup. */
typedef int (*pr_u1c_0f08_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u1c_0f08_cpu *cpu,
                                    pr_u1c_0f08_call call);

typedef enum {
    PR_U1C_0F08_PREFIX_DONE = 0, /* at +0F73, before 3C5:003E */
    PR_U1C_0F08_ARGUMENT,
    PR_U1C_0F08_CALL_ABORT
} pr_u1c_0f08_status;

pr_u1c_0f08_status pr_u1c_vertical_menu_prefix_0f08(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f08_external external, void *context);

#endif
