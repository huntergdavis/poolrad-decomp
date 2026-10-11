#ifndef POOLRAD_RECOVERED_OVERLAY_UNIT38_EFFECTS_NEXT_H
#define POOLRAD_RECOVERED_OVERLAY_UNIT38_EFFECTS_NEXT_H

/* Portable C translations of four more bounded, call-free routines in DOS
 * v1.3 GAME.OVR overlay unit 0x38 (file offset 60,507; stub segment 0x41):
 * public entries 6, 75, 82 and 97, dispatch-table items 6, 90, 98 and 113
 * (issue #67). The argument frame is the one established for PR #63 and
 * re-checked here: far pointer A at [bp+0xC] is a 285-byte character
 * record, far pointer B at [bp+8] and the byte at [bp+6] are not read by
 * any routine in this file. Far-pointer and status types are shared with
 * the reviewed overlay_unit38_effects.h; the state struct is separate
 * because these routines touch two additional engine bytes ([0x681C] and
 * [0x682A]). Evidence: docs/re/OVERLAY_UNIT38_EFFECT_ENTRIES_NEXT.md.
 * Names are addresses, not meanings. No game data is embedded here. */

#include "poolrad/recovered/overlay_unit38_effects.h"

#define PR_U38_RECORD_BYTE_14 0x14u     /* read by entry 75 */
#define PR_U38_RECORD_BYTE_32 0x32u     /* read by entry 82 */
#define PR_U38_RECORD_BYTE_11B 0x11Bu   /* updated by entry 82 */

typedef struct {
    uint8_t b6816;
    uint8_t b6818;
    uint8_t b6819;
    uint8_t b681c;                      /* written only by unit 0x9e entry 9 from its argument */
    uint8_t b6822;
    uint8_t b682a;                      /* written only by unit 0x9e entry 7 from its argument */
    pr_u38_far_ptr scratch_6826;        /* far-pointer temporary [0x6826]/[0x6828] */
} pr_u38n_state;

/* Entry 6, +0x01C3 (file 0x0EE1E), 101 bytes, 34 instructions, four
 * classification paths joining one tail. P = far pointer at
 * record[0x108..0x10B]; Q = far pointer at P[0xA..0xD]; Q is stored into
 * the scratch temporary before anything else. v = Q[0x9F]: v == 0x0A gives
 * 1; v == 0x09 or 0x0C gives 2; v == 0x04 gives 3; anything else 0. Then
 * [0x6822] += class, [0x6818] += class (modulo 256) and [0x6819] = 9,
 * unconditionally. `resolve` must yield 14 bytes for P and 0xA0 for Q; if Q
 * fails the scratch words are already written and PR_U38_ARGUMENT returns.
 * `class_out` receives the 0..3 value. */
pr_u38_status pr_u38_entry6(pr_u38n_state *state, const uint8_t *record,
                            pr_u38_resolve_fn resolve, void *context, unsigned *class_out);

/* Entry 75, +0x24D9 (file 0x11134), 111 bytes, 46 instructions. If [0x682A]
 * != 0 nothing happens (paths = 0). Otherwise v = record[0x14] selects a
 * bracket: 4..6 -> 1, 7..10 -> 2, 11..13 -> 3, 14..17 -> 4, 18..20 -> 5
 * (unsigned compares). For v < 4 or v > 20 the original reads its local
 * variable [bp-1] WITHOUT having written it: the value added is whatever
 * byte the stack held there, which this translation takes from
 * `stack_residue` and flags with PR_U38_PATH_SECOND. [0x6816] += bracket
 * (16-bit add of zero-extended bytes, low byte stored, i.e. modulo 256).
 * paths: PR_U38_PATH_EFFECT when the add ran; `bracket_out` receives the
 * value added (1..5, or the residue). */
pr_u38_status pr_u38_entry75(pr_u38n_state *state, const uint8_t *record,
                             uint8_t stack_residue, unsigned *paths, unsigned *bracket_out);

/* Entry 82, +0x2700 (file 0x1135B), 48 bytes, 15 instructions, two paths,
 * record only: record[0x11B] += 3 (modulo 256); then if record[0x11B] >
 * record[0x32] (unsigned) record[0x11B] = record[0x32] (PR_U38_PATH_EFFECT).
 * No engine state is touched. */
pr_u38_status pr_u38_entry82(uint8_t *record, unsigned *paths);

/* Entry 97, +0x2A28 (file 0x11683), 69 bytes, 27 instructions, a bounded
 * loop. If bit 0 of [0x6819] is clear, or [0x681C] == 0, nothing happens.
 * Otherwise the body runs exactly [0x681C] times: [0x6818] -= 1 (modulo
 * 256), then if [0x6818] < [0x681C] (unsigned) [0x6818] = [0x681C].
 * `iterations_out` receives the number of body executions (0..255). */
pr_u38_status pr_u38_entry97(pr_u38n_state *state, unsigned *iterations_out);

#endif
