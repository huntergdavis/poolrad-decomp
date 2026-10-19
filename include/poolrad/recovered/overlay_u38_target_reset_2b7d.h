#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_RESET_2B7D_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_RESET_2B7D_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_2B7D_OK = 0,
    PR_U38_2B7D_ARGUMENT,
    PR_U38_2B7D_CALL_ABORT
} pr_u38_2b7d_status;

/* Complete U38 public entry 103. The selector callback supplies the
 * effects of the merged +101D helper's resident far-call dependency. */
pr_u38_2b7d_status pr_u38_target_reset_2b7d(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

/* Complete raw U38 [2B7D,2BC8), 75 bytes, RETF0Ah. Compose raw +101D and
 * resident +2E8B on shared CPU/RAM, preserving the callback API above.
 * If no selected target blocks via its +32 byte, clear DS:6818 when the
 * live active character's signed +2E byte is positive or signed +73 is <4.
 * Reload DS:5D92/5D94 at the original sites; caller locals may alias it.
 * Same exact 1 MiB A20-off flat-RAM profile as the raw selector: immutable
 * code, no MMIO/IRQ/fault/SMC, separate host CPU/diagnostic/RAM storage.
 * Entry IP2B7D, SP002E..FFFC; ten unused argument bytes; reached far-return
 * frames fit in SS:0000..FFFF. Effective offsets wrap at 16 bits; words/LES
 * read physically contiguous bytes. Live near-CALL guard, exact partial
 * refusal state and nested statuses. Invalid host arguments reject atomically.
 * See docs/re/U38_TARGET_RESET_2B7D_CPU.md. */
pr_target_status pr_u38_target_reset_2b7d_cpu(uint8_t *ram, size_t size,
    pr_target_cpu *cpu, pr_target_diagnostic *diagnostic);

#endif
