#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_SPRITE_OFF_2AA2_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_SPRITE_OFF_2AA2_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_2AA2_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_2aa2_cpu;

/* View refresh at overlay stub 03CA:0025. The callback runs at the far-call
 * instruction and may supply the callee's RAM/CPU effects. It returns 1 to
 * resume at +2AB5; the callee takes no stack arguments. */
typedef int (*pr_u1c_2aa2_view)(void *context, uint8_t *ram,
                                 size_t ram_size, pr_u1c_2aa2_cpu *cpu);

typedef enum {
    PR_U1C_2AA2_OK = 0,
    PR_U1C_2AA2_ARGUMENT,
    PR_U1C_2AA2_CALL_ABORT
} pr_u1c_2aa2_status;

pr_u1c_2aa2_status pr_u1c_sprite_off_2aa2(
    uint8_t *ram, size_t ram_size, pr_u1c_2aa2_cpu *cpu,
    pr_u1c_2aa2_view view, void *context);

#endif
