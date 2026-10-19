#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_INDEX_GATE_2A87_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_INDEX_GATE_2A87_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_2A87_OK = 0,
    PR_U38_2A87_ARGUMENT,
    PR_U38_2A87_CALL_ABORT
} pr_u38_2a87_status;

/* Complete U38 public entry 99. select_target handles the unresolved
 * resident far call made by the provisional +101D dependency. */
pr_u38_2a87_status pr_u38_target_index_gate_2a87(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

/* Reuse the explicit measured SHR profiles introduced with +25BC. */
#include "poolrad/recovered/overlay_u38_target_mask81_25bc.h"
/* Complete raw U38 [2A87,2AE6), 95 bytes, RETF0Ah. Compose raw +101D and
 * resident +2E8B on shared CPU/RAM. For a nonnull target, halve DS:6818 iff
 * its DS:5589 indexed table byte is zero or has bit 0 set. Preserve the
 * original two index reads and the original flags, stack, and register effects.
 * Same exact 1 MiB A20-off flat-RAM profile as the raw selector: immutable
 * code, no MMIO/IRQ/fault/SMC, separate host CPU/diagnostic/RAM storage.
 * Entry IP2A87, SP002E..FFFC, ten unused argument bytes; reached far-return
 * frames fit in SS:0000..FFFF. Effective offsets wrap at 16 bits; words/LES
 * read physically contiguous bytes. Live near-CALL guard; partial refusal
 * state and nested status retained. Invalid host arguments reject atomically.
 * SHR AF is undefined: caller chooses Unicorn(clear) or DOSBox386(set), with
 * no flag masking. See docs/re/U38_TARGET_INDEX_GATE_2A87_CPU.md. */
pr_target_status pr_u38_target_index_gate_2a87_cpu(uint8_t *ram, size_t size,
    pr_target_cpu *cpu, pr_target_shift_profile shift_profile,
    pr_target_diagnostic *diagnostic);

#endif
