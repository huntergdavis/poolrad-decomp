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

#include "poolrad/recovered/resident_0709_clear_0dd3_cpu.h"
typedef enum {
    PR_U1C_2B14_CPU_OK=0, PR_U1C_2B14_CPU_ARGUMENT,
    PR_U1C_2B14_CPU_CODE, PR_U1C_2B14_CPU_CALLEE,
    PR_U1C_2B14_CPU_TRANSFER
} pr_u1c_2b14_cpu_status;
typedef struct {
    int clear_status;
    pr_r0709_clear_cpu_diagnostic clear;
} pr_u1c_2b14_cpu_diagnostic;
/* Raw [2B14,2B33), RETF0. Increments live DS:49ED, pushes left1/top1/
 * right15/bottom15 with AH retained, calls raw 0709:0DD3, then stores1 at
 * live DS:84DE only after the expected callee return.
 * Exactly1MiB A20off bus with EGA; IP2B14; SP[004C,FFFC]. DS/SS entire
 * segment windows avoid EGA. Immutable loaded code/bindings outside EGA;
 * writes avoid code, reached far-return frames do not wrap at SS:FFFF.
 * CPU/bus/EGA/diagnostic are distinct host objects outside RAM.
 * Other aliases stay live. Inherits the raw clear/rectangle/bus contract.
 * Guard CALL2B27 opcode/offset0DD3 before pushing its return frame;
 * live segment operand selects loaded resident. No substituted functions.
 * max_loopbacks and rect_loopbacks pass through without normalization.
 * CALLEE retains nested partial state/status. TRANSFER retains unexpected
 * successful return state and does not perform the DS:84DE store.
 * ARGUMENT preserves CPU/RAM/EGA and resets diagnostics to -1.
 * No arbitrary-PC resume, interrupts, faults or hardware claim beyond EGA.
 */
pr_u1c_2b14_cpu_status pr_u1c_clear_box_2b14_cpu(pr_bus *bus,size_t ram_size,
    pr_u1c_2b14_cpu *cpu,size_t max_loopbacks,size_t rect_loopbacks,
    pr_u1c_2b14_cpu_diagnostic *diagnostic);

#endif
