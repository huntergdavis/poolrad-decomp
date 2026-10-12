#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_PARTY_SURPRISE_1629_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_PARTY_SURPRISE_1629_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x1629..+0x16C8 (file 0x236F..0x240E, exactly 160 bytes,
 * `retf` last byte, twelve bytes of locals, no arguments, no result): the handler of ECL opcode 0x22
 * (PARTY SURPRISE, reference name). The next routine (+0x16C9) is separate. Evidence:
 * docs/re/U1C_PARTY_SURPRISE_1629.md.
 *
 * In the original's order:
 *   loader(2) (0x2B:0x2A)                                              (+0x162F..+0x1632)
 *   a := b := c := 0 (bytes); node := far [0x5D96]:[0x5D98]            (+0x1637..+0x164C)
 *   while node (both words) <> 0:                                      (+0x1650..+0x1687)
 *       if byte node+0x2F = 4 or byte node+0x2F = 0x0A: a := 1, c := 1 (c is never read again)
 *       node := far pointer at node+0x104 (offset) and node+0x106 (segment)
 *   d1 := word([0x7086], [0x7046]) via 0x2B:0x4D (slot 1)              (+0x1689..+0x1696)
 *   d2 := word([0x7087], [0x7047]) via 0x2B:0x4D (slot 2)              (+0x1699..+0x16A6)
 *   store word 0x2B:0x52 (d1, zero-extended a)                         (+0x16A9..+0x16B2)
 *   store word 0x2B:0x52 (d2, zero-extended b)                         (+0x16B7..+0x16C0)
 *   return
 *
 * The byte b is initialized to 0 and never written, so the second store always stores 0. Both
 * destinations are operand WORDS built from the value bytes of slots 1 and 2 (whatever their tags).
 * The party list is explicit host state, the same node view type as the unit 0x68 party-byte routine
 * (host bytes of the record's segment, the far offset, the next link); every field read is
 * es:[di + k] with a 16-bit offset wrap. A null or too small view, or a list past the cap, is a host
 * guard (the original has no cycle check) that stops the handler before any store.
 *
 * Classification: bounded model composed of the lane-A loader, operand word and store word, with the
 * party list as host state. No new hooks. Not a strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"
#include "poolrad/recovered/overlay_u68_party_byte_0236.h"

#define PR_U1C_PSURP_OPERANDS 2u
#define PR_U1C_PSURP_FIELD 0x2Fu           /* es:[di + 0x2F] */
#define PR_U1C_PSURP_MATCH_A 4u            /* cmp byte, 4 */
#define PR_U1C_PSURP_MATCH_B 0x0Au         /* cmp byte, 0x0A */
#define PR_U1C_PSURP_LIST_CAP 65536u       /* host guard */

typedef enum {
    PR_U1C_PSURP_OK = 0,
    PR_U1C_PSURP_ARGUMENT,   /* null state */
    PR_U1C_PSURP_LIST,       /* host guard: a visited node has a null or too small segment for the field or the four link
                                bytes, or the list exceeds the cap. Loader state retained, nothing stored. */
    PR_U1C_PSURP_CORE        /* a lane-A core status ended the handler (report->core_status); on HOOK both stores have
                                completed and the class-1 store hook model reported */
} pr_u1c_psurp_status;

typedef struct {
    unsigned visited;                /* nodes processed */
    uint8_t flag;                    /* the first result: 1 when some node's +0x2F is 4 or 0x0A */
    uint16_t dest1, dest2;           /* word(slot 1), word(slot 2) when reached */
    unsigned stored1, stored2;       /* store calls made */
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status store1_status, store2_status;
} pr_u1c_psurp_report;

/* The handler on lane A's loader state; `head` = far [0x5D96]:[0x5D98] (NULL = both words zero). `report` may be NULL. */
pr_u1c_psurp_status pr_u1c_op_party_surprise(pr_rec_ecl_loader_state *state, const pr_u68_0236_node *head,
                                             pr_u1c_psurp_report *report);

#endif
