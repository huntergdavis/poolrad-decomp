#ifndef POOLRAD_RECOVERED_OVERLAY_UNIT38_WALK_H
#define POOLRAD_RECOVERED_OVERLAY_UNIT38_WALK_H

/* Portable C for DOS v1.3 GAME.OVR unit 0x38 public entries 81 (+0x268A,
 * table item 97) and 110 (+0x2E37, table item 126), issue #81. Both are
 * call-free far Pascal procedures ending in `retf 0xA`, reached through the
 * dispatcher frame established for PR #63: far pointer A at [bp+0xC] is a
 * 285-byte character record; neither routine reads far pointer B or the
 * byte argument. Far-pointer, resolver and status types are shared with
 * the reviewed overlay_unit38_effects.h.
 * Evidence: docs/re/OVERLAY_UNIT38_WALK_ENTRIES.md. Names are addresses,
 * not meanings. No game data is embedded here. */

#include "poolrad/recovered/overlay_unit38_effects.h"

#define PR_U38_RECORD_SLOT_C8 0xC8u     /* far-pointer slot of the record behind slot 0x108 (entry 110) */
#define PR_U38_NODE_LINK 0x2Au          /* far pointer to the next node */
#define PR_U38_NODE_MIN_SIZE 0x35u      /* bytes a node must expose: link, 0x2F..0x31, 0x34 */

typedef struct {
    uint8_t b6816;                      /* entry 81 adds to it */
    uint8_t b681e;                      /* entry 110 tests it and may set it to 1 */
    uint8_t b682a;                      /* entry 81's gate (compared with 4 and 2) */
    pr_u38_far_ptr scratch_6826;        /* far-pointer temporary [0x6826]/[0x6828], written by entry 110 */
} pr_u38w_state;

/* Entry 81, +0x268A (file 0x112E5), 118 bytes, 48 instructions, 16 edges.
 * Gate: proceeds only if [0x682A] == 4 or [0x682A] == 2 (paths = 0
 * otherwise). The 98-byte body after the gate (+0x269E..+0x26FF) is
 * byte-identical to the reviewed entry 75's +0x24E6..+0x2547 (its relative
 * +13..+110): v = record[0x14] selects 4..6 -> 1, 7..10 -> 2, 11..13 -> 3,
 * 14..17 -> 4, 18..20 -> 5; for v < 4 or v > 20 the
 * original reads its local [bp-1] WITHOUT having written it, so the byte
 * added is inherited stack content, taken here from `stack_residue` and
 * flagged with PR_U38_PATH_SECOND. [0x6816] += bracket (16-bit add of
 * zero-extended bytes, low byte stored). paths: PR_U38_PATH_EFFECT when the
 * add ran; `bracket_out` receives the byte added. */
pr_u38_status pr_u38_entry81(pr_u38w_state *state, const uint8_t *record,
                             uint8_t stack_residue, unsigned *paths, unsigned *bracket_out);

/* How entry 110's outer loop ended. */
typedef enum {
    PR_U38_WALK_END = 0,      /* the current node pointer was 0:0 (tested before the flag) */
    PR_U38_WALK_FLAG,         /* [0x681E] was nonzero at the top of the loop (preset, or set by a match) */
    PR_U38_WALK_ARGUMENT,     /* null state/record/resolver, or a pointer the resolver refused */
    PR_U38_WALK_LIMIT         /* `node_limit` nodes visited without reaching an exit: the original
                                 would keep following links (a cyclic list never terminates there) */
} pr_u38_walk_status;

/* Entry 110, +0x2E37 (file 0x11A92), 163 bytes, 52 instructions, 9 edges
 * (two backward). P = far pointer at record[0x108..0x10B]; Q = far pointer
 * at P[0xA..0xD], stored into the scratch temporary; node = far pointer at
 * Q[0xC8..0xCB]. Outer loop: exit if node == 0:0; exit if [0x681E] != 0;
 * if node[0x34] != 0, run the inner loop for i = 1, 2, 3 exactly: set
 * [0x681E] = 1 when (node[0x2E+i] == 0x98 and node[0x31] == 0xFC) or
 * node[0x2E+i] == 0x76 (the inner loop always completes all three steps);
 * then node = far pointer at node[0x2A..0x2D] and repeat. `resolve` is asked
 * for 14 bytes of P, 0xCC bytes of Q and 0x35 bytes of each node, in access
 * order. `nodes_out` receives the number of nodes whose fields were read
 * (nodes that failed the null test are not counted); `matched_out` receives
 * 1 if this call set the flag. The original has no bound on the number of
 * nodes; `node_limit` (>= 1) is this translation's explicit guard. */
pr_u38_walk_status pr_u38_entry110(pr_u38w_state *state, const uint8_t *record,
                                   pr_u38_resolve_fn resolve, void *context,
                                   unsigned node_limit, unsigned *nodes_out, unsigned *matched_out);

#endif
