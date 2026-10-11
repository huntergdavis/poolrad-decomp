#ifndef POOLRAD_RECOVERED_OVERLAY_UNIT38_HELPER_H
#define POOLRAD_RECOVERED_OVERLAY_UNIT38_HELPER_H

/* Portable C for the DOS v1.3 GAME.OVR unit 0x38 private helper at +0x0000
 * and the three public dispatch entries that only call it: 93 (+0x29CB),
 * 94 (+0x29D9) and 96 (+0x2A11), table items 109, 110 and 112 (issue #76).
 *
 * Call ABI of the helper, proved from all 20 near-call sites in the unit:
 * the caller sets AL to a byte, pushes AX, pushes CS, and near-calls
 * +0x0000, so the helper sees a far-call frame ([bp+2] IP, [bp+4] CS,
 * [bp+6] argument word) and returns with `retf 2`, which pops IP, CS and
 * the argument. Only the low byte of the argument word is read (AH is
 * whatever the caller left in it). The helper takes no far pointers and
 * reads no record. Status and path flags are shared with the reviewed
 * overlay_unit38_effects.h. Evidence: docs/re/OVERLAY_UNIT38_HELPER_0000.md.
 * Names are addresses, not meanings. No game data is embedded here. */

#include "poolrad/recovered/overlay_unit38_effects.h"

typedef struct {
    uint8_t b6817;      /* written only by unit 0x9e entry 20 (from an argument) and this helper */
    uint8_t b6818;
    uint8_t b6819;      /* read by entries 94 (bit 1) and 96 (bit 0); never written here */
} pr_u38h_state;

/* Private helper +0x0000 (file 0x0EC5B), 31 bytes, 11 instructions, three
 * paths: if key == 0, or key == [0x6817], then [0x6818] = 0 and [0x6817] = 0
 * (PR_U38_PATH_EFFECT); otherwise nothing changes. `key` is the low byte of
 * the pushed argument word. */
pr_u38_status pr_u38_helper_0000(pr_u38h_state *state, uint8_t key, unsigned *paths);

/* Entry 93, +0x29CB (file 0x11626), 14 bytes, 8 instructions, straight
 * line: calls the helper with key 0x34. paths: PR_U38_PATH_EFFECT when the
 * helper cleared the two bytes (i.e. [0x6817] was 0x34). */
pr_u38_status pr_u38_entry93(pr_u38h_state *state, unsigned *paths);

/* Entries 94 (+0x29D9, file 0x11634) and 96 (+0x2A11, file 0x1166C), 23
 * bytes, 12 instructions, two paths each: if bit 1 (entry 94) or bit 0
 * (entry 96) of [0x6819] is set, call the helper with key 0, which always
 * clears [0x6818] and [0x6817]. paths: PR_U38_PATH_SECOND when the call was
 * made, plus PR_U38_PATH_EFFECT (always set together, since key 0 clears). */
pr_u38_status pr_u38_entry94(pr_u38h_state *state, unsigned *paths);
pr_u38_status pr_u38_entry96(pr_u38h_state *state, unsigned *paths);

#endif
