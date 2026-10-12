#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_PARTY_STRENGTH_12C6_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_PARTY_STRENGTH_12C6_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x12C6..+0x13D6 (file
 * 0x200C..0x211C, exactly 273 bytes, `retf` last byte, twelve bytes of locals,
 * no arguments, no result): the handler of ECL opcode 0x1D (PARTYSTRENGTH).
 * The next routine (+0x13D7) is separate. Evidence: docs/re/U1C_PARTY_STRENGTH_12C6.md.
 * "Party strength" is the reference name only; no game role is asserted beyond the
 * documented fields.
 *
 * In the original's order:
 *   loader(1) (0x2B:0x2A)                                              (+0x12CC..+0x12CF)
 *   sum := 0 (a byte)                                                  (+0x12D4)
 *   node := far [0x5D96]:[0x5D98]                                      (+0x12D8..+0x12E2)
 *   while node (both words) <> 0:                                      (+0x12E5..+0x12ED)
 *       a := byte node+0x11B; b := node+0x111; c := node+0x110;
 *       d := node+0x09B; e := node+0x096                               (+0x12F0..+0x1324)
 *       b := b - 0x3C if b > 0x3C else 0                               (+0x1327..+0x133A)
 *       c := c - 0x27 if c > 0x27 else 0                               (+0x133E..+0x1351)
 *       w := a + 5*b + 5*c + 8*d + 4*e   (16-bit; at most 5,370, so no wrap)
 *                                                                      (+0x1355..+0x1394)
 *       sum := sum + low byte of (w div 10)   (cwd; div 10; byte add)   (+0x1396..+0x139C)
 *       node := far pointer at node+0x104 (offset) and node+0x106       (+0x139F..+0x13B2)
 *   dest := word(slot 1) via 0x2B:0x4D                                 (+0x13B5..+0x13C2)
 *   store word 0x2B:0x52 (dest, zero-extended sum)                     (+0x13C5..+0x13CE)
 *   return
 *
 * Each field read is es:[di + k] with di the node's far offset: the effective offset
 * wraps modulo 65536. The party list lives in game memory the model does not carry:
 * the nodes are explicit host state (the same pr_u68_0236_node view type as the
 * unit 0x68 party-byte routine: host bytes of the node's segment from offset 0,
 * the far offset, the far next link). The list has no cycle check in the original; the
 * model's cap is a host guard. The sum is a byte, so it wraps modulo 256 after
 * enough nodes. No game data is embedded.
 *
 * Classification: bounded model (path-complete over the 273 bytes; composed of the
 * lane-A loader, operand word and store word, with the party list as host state).
 * Not a strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"
#include "poolrad/recovered/overlay_u68_party_byte_0236.h"

#define PR_U1C_PSTR_A 0x11Bu                 /* es:[di + 0x11B] */
#define PR_U1C_PSTR_B 0x111u
#define PR_U1C_PSTR_C 0x110u
#define PR_U1C_PSTR_D 0x09Bu
#define PR_U1C_PSTR_E 0x096u
#define PR_U1C_PSTR_B_BIAS 0x3Cu             /* cmp byte [bp-5], 0x3C */
#define PR_U1C_PSTR_C_BIAS 0x27u             /* cmp byte [bp-2], 0x27 */
#define PR_U1C_PSTR_DIVISOR 10u              /* mov cx, 0x0A ; div cx */
#define PR_U1C_PSTR_LIST_CAP 65536u          /* host guard: the original has no cycle check */

typedef enum {
    PR_U1C_PSTR_OK = 0,
    PR_U1C_PSTR_ARGUMENT,   /* null state */
    PR_U1C_PSTR_LIST,       /* host guard: a visited node has a null or too small segment for a field or the four
                               link bytes, or the list exceeds the cap. Loader state retained, nothing stored. */
    PR_U1C_PSTR_CORE        /* a lane-A core status ended the handler (report->core_status); on HOOK the store
                               has completed and the engine's store hook model ran */
} pr_u1c_pstr_status;

typedef struct {
    unsigned visited;                /* nodes processed */
    uint8_t sum;                     /* the byte accumulator at the end */
    uint16_t dest;                   /* word(slot 1) when reached */
    unsigned stored;                 /* the store word call was made */
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status store_status;
} pr_u1c_pstr_report;

/* The handler on lane A's loader state; `head` = far [0x5D96]:[0x5D98] (NULL = both words zero).
 * `report` may be NULL. */
pr_u1c_pstr_status pr_u1c_op_party_strength(pr_rec_ecl_loader_state *state, const pr_u68_0236_node *head,
                                            pr_u1c_pstr_report *report);

#endif
