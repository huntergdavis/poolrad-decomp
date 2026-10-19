#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_GATE_2AE6_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_GATE_2AE6_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_2AE6_OK = 0,
    PR_U38_2AE6_ARGUMENT,
    PR_U38_2AE6_CALL_ABORT
} pr_u38_2ae6_status;

/* Complete U38 public entry 100. select_target handles the unresolved
 * resident far call made by the provisional +101D dependency. */
pr_u38_2ae6_status pr_u38_target_gate_2ae6(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

/* Reuse the explicit measured SHR profiles introduced with +25BC. */
#include "poolrad/recovered/overlay_u38_target_mask81_25bc.h"
/* Complete raw U38 [2AE6,2B20), 58 bytes, RETF0Ah. Compose raw +101D and
 * resident +2E8B on shared CPU/RAM, preserving the callback API above.
 * A nonnull target with signed byte +32 > 0 halves unsigned DS:6818;
 * all other paths preserve the amount, including zero/negative target bytes.
 * Same exact 1 MiB A20-off flat-RAM profile as raw +101D: immutable mapped
 * code, no MMIO/IRQ/fault/SMC, separate host CPU/diagnostic/RAM storage.
 * Entry IP2AE6, SP002E..FFFC, ten unused argument bytes; reached far-return
 * frames fit in SS:0000..FFFF. Effective offsets wrap at 16 bits; words/LES
 * read physically contiguous bytes. Live near-CALL guard; exact partial
 * refusal state and nested statuses. Invalid host arguments reject atomically.
 * SHR AF is undefined: choose Unicorn(clear) or DOSBox386(set) explicitly;
 * no flag masking. See docs/re/U38_TARGET_GATE_2AE6_CPU.md. */
pr_target_status pr_u38_target_gate_2ae6_cpu(uint8_t *ram, size_t size,
    pr_target_cpu *cpu, pr_target_shift_profile shift_profile,
    pr_target_diagnostic *diagnostic);

#endif
