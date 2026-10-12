#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_AND_OR_0DEF_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_AND_OR_0DEF_H

/* Path-complete model of DOS v1.3 GAME.OVR unit 0x1c +0x0DEF..+0x0E5D (file
 * 0x1B35..0x1BA2, exactly 111 bytes, 43 instructions, `retf`, no arguments, no
 * trailing data: the next prologue is +0x0E5E): the shared handler of ECL
 * opcodes 0x2F (AND) and 0x30 (OR), dispatcher cases of +0x2F2D. Evidence:
 * docs/re/U1C_AND_OR_0DEF.md.
 *
 * In the original's order, every callee being unit 0x25 stubs already merged
 * by lane A:
 *   loader(3)                                   lcall 0x2B:0x2A   pr_rec_ecl_load_operands(state, 3)
 *   v1 = slot value(1)   -> [bp-6]              lcall 0x2B:0x25   pr_rec_ecl_slot_value
 *   v2 = slot value(2)   -> [bp-4]              lcall 0x2B:0x25
 *   dest = word(low[3], high[3]) -> [bp-2]      lcall 0x2B:0x4D   pr_rec_ecl_operand_word (function 4)
 *   r = low byte of (v1 and v2) when the latch [0x6F7F] = 0x2F, else low byte of (v1 or v2)
 *   compare words (0, r): the six flags        lcall 0x2B:0x70   pr_rec_ecl_compare_words
 *   store word (dest, r zero-extended)          lcall 0x2B:0x52   pr_rec_ecl_store_word
 * Only the low byte of the 16-bit result is kept (`mov byte [bp-7], al`), and
 * every latch other than 0x2F takes the OR path. The compare runs before the
 * store, so the flags are set even when the store ends in a hook or fails.
 *
 * Statuses follow lane A's SAVE model (pr_rec_ecl_op_save): a loader or slot
 * value that is BOUNDS, UNRESOLVED or ARGUMENT returns at once and neither the
 * compare nor the store is made (the engine would use a value this model does
 * not have); a class-1 window read reports HOOK after the store, so the host
 * must treat HOOK as "the engine also ran its load hook". Nothing is reported
 * as success that the cores could not resolve.
 *
 * Classification: path-complete composition of merged cores, like function 22.
 * Not a strict translation of the 111 bytes by itself: its result depends on
 * the cores' own bounded parts (class-1 hook +0x07BC, class-4 sources). No
 * game-function credit is claimed. */

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_AND_OR_LATCH_AND 0x2Fu      /* cmp byte [0x6F7F], 0x2F */
#define PR_U1C_AND_OR_OPERANDS 3u          /* mov al, 3; lcall 0x2B:0x2A */

typedef struct {
    uint16_t v1, v2;            /* slot values (valid when read) */
    uint16_t dest;              /* function 4 of slot 3 */
    uint8_t result;             /* low byte of the AND / OR */
    unsigned slot1_read, slot2_read;
    unsigned compared;          /* the compare was made */
    unsigned stored;            /* the store was made */
    pr_rec_ecl_window_status slot1_status, slot2_status;
} pr_u1c_and_or_report;

/* The handler. `store_status` (optional) receives the store core's status. `report` (optional) the details. */
pr_rec_ecl_handler_status pr_u1c_op_and_or(pr_rec_ecl_loader_state *state, pr_rec_ecl_window_status *store_status,
                                           pr_u1c_and_or_report *report);

#endif
