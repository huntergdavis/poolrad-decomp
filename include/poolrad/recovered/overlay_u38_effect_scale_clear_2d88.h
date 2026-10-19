#ifndef POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_SCALE_CLEAR_2D88_H
#define POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_SCALE_CLEAR_2D88_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_2D88_OK = 0,
    PR_U38_2D88_ARGUMENT,
    PR_U38_2D88_CALL_ABORT
} pr_u38_2d88_status;

/* GAME.OVR U38 public entry 107, +2D88..+2DE4 (92 bytes). The near call to
 * +101D is composed; its resident BA:2E8B far call uses select_target. */
pr_u38_2d88_status pr_u38_effect_scale_clear_2d88(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

/* Reuse the explicit measured SHR profiles introduced with +25BC. */
#include "poolrad/recovered/overlay_u38_target_mask81_25bc.h"
/* Complete raw U38 [2D88,2DE4), 92 bytes, RETF0Ah. Compose raw +101D and
 * resident +2E8B on shared CPU/RAM, preserving the callback API above.
 * A null target clears DS:6818; nonzero target +32 preserves it; otherwise
 * target +31 == B1h halves it and other values clear it. Preserve repeated
 * byte-32 comparisons and their returned FLAGS, including source-order aliases.
 * Same exact 1 MiB A20-off flat-RAM profile as raw +101D: immutable mapped
 * code, no MMIO/IRQ/fault/SMC, separate host CPU/diagnostic/RAM storage.
 * Entry IP2D88, SP002E..FFFC, ten unused argument bytes; reached far-return
 * frames fit in SS:0000..FFFF. Effective offsets wrap at 16 bits; words/LES
 * read physically contiguous bytes. Live near-CALL guard; exact partial
 * refusal state and nested statuses. Invalid host arguments reject atomically.
 * SHR AF is undefined: choose Unicorn(clear) or DOSBox386(set) explicitly;
 * no flag masking. See docs/re/U38_EFFECT_SCALE_CLEAR_2D88_CPU.md. */
pr_target_status pr_u38_effect_scale_clear_2d88_cpu(uint8_t *ram, size_t size,
    pr_target_cpu *cpu, pr_target_shift_profile shift_profile,
    pr_target_diagnostic *diagnostic);

#endif
