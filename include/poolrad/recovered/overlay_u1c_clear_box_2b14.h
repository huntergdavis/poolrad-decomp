#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_CLEAR_BOX_2B14_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_CLEAR_BOX_2B14_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_2B14_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_2b14_cpu;

/* Resident 0709:0DD3 display region, with four words on the emulated stack.
 * The callback supplies callee RAM/CPU effects and returns 1 to continue;
 * the model performs the confirmed RETF 8 cleanup. */
typedef int (*pr_u1c_2b14_display)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u1c_2b14_cpu *cpu);

typedef enum {
    PR_U1C_2B14_OK = 0,
    PR_U1C_2B14_ARGUMENT,
    PR_U1C_2B14_CALL_ABORT
} pr_u1c_2b14_status;

pr_u1c_2b14_status pr_u1c_clear_box_2b14(
    uint8_t *ram, size_t ram_size, pr_u1c_2b14_cpu *cpu,
    pr_u1c_2b14_display display, void *context);

#endif
