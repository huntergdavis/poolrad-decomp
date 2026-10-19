#ifndef POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_CLEAR_264B_H
#define POOLRAD_RECOVERED_OVERLAY_U38_EFFECT_CLEAR_264B_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_264B_OK = 0,
    PR_U38_264B_ARGUMENT,
    PR_U38_264B_CALL_ABORT
} pr_u38_264b_status;

/* GAME.OVR U38 public entry 80, +264B..+268A (63 bytes). The near call to
 * +101D is composed; its resident BA:2E8B far call uses select_target. */
pr_u38_264b_status pr_u38_effect_clear_264b(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

/* Raw CPU composition API; the callback API above remains unchanged.
 * Complete U38 [264B,268A), 63 bytes, RETF0Ah. Compose raw +101D and
 * RES_00BA_2E8B using live shared CPU/RAM. Clear DS:6818 for a null target,
 * or a target with byte +32 equal to zero and byte +31 unequal to B1h.
 * Uses the +101D flat-RAM profile: exact 1 MiB, A20 off, immutable code,
 * no MMIO/IRQ/fault/SMC; CPU and diagnostic storage separate from RAM.
 * Entry IP264B, SP002E..FFFC, ten unused argument bytes; reached far-return
 * frames must fit within SS:0000..FFFF. Effective offsets wrap at 16 bits;
 * word/LES operands are physically contiguous. Live near-CALL opcode and
 * target guard; nested status and exact partial state retained on refusal.
 * Invalid host arguments reject atomically. See U38_EFFECT_CLEAR_264B_CPU.md. */
pr_target_status pr_u38_effect_clear_264b_cpu(uint8_t *ram, size_t size,
    pr_target_cpu *cpu, pr_target_diagnostic *diagnostic);

#endif
