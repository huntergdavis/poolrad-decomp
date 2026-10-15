#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY9_ROLL_LATCH_0E2E_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY9_ROLL_LATCH_0E2E_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_0E2E_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_0e2e_cpu;

/* Called at CS:+0DE3 after PUSH CS / CALL NEAR. SS:SP holds return IP
 * 0E46, return CS, range WORD, then count WORD. The callback supplies
 * the complete callee effects and leaves SP on this frame for RETF 4.
 * The merged pr_u9e_entry8_roll supplies its AL and seed effects. */
typedef int (*pr_u9e_0e2e_roll)(void *context, uint8_t *ram,
                                 size_t ram_size, pr_u9e_0e2e_cpu *cpu);

typedef enum {
    PR_U9E_0E2E_OK = 0,
    PR_U9E_0E2E_ARGUMENT,
    PR_U9E_0E2E_CALL_ABORT
} pr_u9e_0e2e_status;

pr_u9e_0e2e_status pr_u9e_entry9_roll_latch_0e2e(
    uint8_t *ram, size_t ram_size, pr_u9e_0e2e_cpu *cpu,
    pr_u9e_0e2e_roll roll, void *context);

#endif
