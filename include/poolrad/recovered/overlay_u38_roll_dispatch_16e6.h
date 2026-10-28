#ifndef POOLRAD_RECOVERED_OVERLAY_U38_ROLL_DISPATCH_16E6_H
#define POOLRAD_RECOVERED_OVERLAY_U38_ROLL_DISPATCH_16E6_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38_16E6_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_16e6_cpu;

typedef enum {
    PR_U38_16E6_ENTER = 0,
    PR_U38_16E6_AFTER_ROLL,
    PR_U38_16E6_AFTER_DISPATCH
} pr_u38_16e6_phase;

typedef enum {
    PR_U38_16E6_RETURNED = 0,
    PR_U38_16E6_ARGUMENT,      /* rejected without changing CPU or RAM */
    PR_U38_16E6_ROLL_STOP,     /* entered relocated far target :0048 */
    PR_U38_16E6_DISPATCH_STOP  /* entered U38:15EB; untranslated callee */
} pr_u38_16e6_status;

/* Bounded raw-CPU translation of U38 entry 57, [16E6,170B), 37 bytes.
 * This is NOT a composed function: no callee runs here. ENTER stops at the
 * loaded roll target with its far frame intact. AFTER_ROLL requires an
 * externally supplied returned state at wrapper_cs:16FA, then stops at
 * wrapper_cs:15EB. AFTER_DISPATCH requires an externally supplied returned
 * state at wrapper_cs:1707, then executes POP BP / RETF 0Ah. Calling any
 * phase while still at either callee is rejected unchanged. No return ABI
 * or callee effects are fabricated, and phase selection is never automatic.
 *
 * RAM is exactly 1 MiB; physical addresses wrap there, effective offsets
 * at 16 bits; a word's second byte is physically contiguous. Original
 * instruction bytes must be immutable except the relocated far-call segment
 * word at CS:16F8, read from RAM when reached. Stack/data must not overwrite
 * code. CPU and RAM storage must not overlap. Caller controls wrapper_cs
 * and explicit phase; the expected CS:IP is checked before any mutation.
 * Incoming flags should use real-mode CPU-normalized reserved bits.
 */
pr_u38_16e6_status pr_u38_roll_dispatch_16e6(
    uint8_t *ram, size_t ram_size, pr_u38_16e6_cpu *cpu,
    uint16_t wrapper_cs, pr_u38_16e6_phase phase);

#endif
