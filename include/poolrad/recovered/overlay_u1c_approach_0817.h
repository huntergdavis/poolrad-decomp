#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_APPROACH_0817_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_APPROACH_0817_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x0817..+0x0850 (file 0x155D..0x1596, 58 bytes of code
 * ending in `retf`, no locals, no operands, no result): the handler of ECL opcode 0x0D (APPROACH,
 * reference name). The ledger's 63-byte span adds five trailing bytes (+0x0851..+0x0855), an embedded
 * Pascal string constant never executed here; the next routine is +0x0856 (PICTURE). Evidence:
 * docs/re/U1C_APPROACH_0817.md.
 *
 * In the original's order:
 *   window-1 word w582 (es:[[0x49D6] + 0x582], script address 0x6DC1) <> 0 (`cmp word, 0; jbe`, unsigned):
 *                                                                      (+0x081A..+0x0824)
 *       w582 := w582 - 1  (16-bit)                                      (+0x0826..+0x082A)
 *       0x2B:0x48 (unit 0x25 entry 8, +0x054E) with the bytes [0x6F81], [0x6F82], low byte of the
 *           decremented w582 and the far pointer DS:0x84C8              (+0x082F..+0x0846)
 *   [0x49ED] := [0x49ED] + 1  (the PC skips the opcode byte)             (+0x084B)
 *   return
 *
 * w582 and the DS bytes [0x6F81], [0x6F82] are the state SETUP MONSTER (unit 0x1c +0x03B4, see
 * overlay_u1c_setup_monster_03b4.h) leaves: this handler shares its environment and its unresolved
 * 0x2B:0x48 hook. The hook is an explicit fallible host callback (missing reached hook: HOOK status;
 * nonzero return: CALLBACK status); the decrement stands in both cases and the PC is not advanced.
 * The branch is skipped when w582 = 0 and then only the PC advances.
 *
 * Classification: bounded model (the 0x2B:0x48 callee is unresolved). Not a strict translation. No
 * game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"
#include "poolrad/recovered/overlay_u1c_setup_monster_03b4.h"

typedef enum {
    PR_U1C_APPROACH_OK = 0,
    PR_U1C_APPROACH_ARGUMENT,   /* null state or env */
    PR_U1C_APPROACH_HOOK,       /* w582 <> 0 and the 0x2B:0x48 hook is NULL; the decrement stands, the PC is not advanced */
    PR_U1C_APPROACH_CALLBACK    /* the 0x2B:0x48 hook returned nonzero (report->callback_status); decrement stands, PC not advanced */
} pr_u1c_approach_status;

typedef struct {
    uint16_t before, after;          /* w582 before and after */
    unsigned called;                 /* the 0x2B:0x48 hook was called */
    int callback_status;
} pr_u1c_approach_report;

/* The handler on lane A's loader state, sharing SETUP MONSTER's environment (the DS bytes and the 0x2B:0x48 hook). `report` may be NULL. */
pr_u1c_approach_status pr_u1c_op_approach(pr_rec_ecl_loader_state *state, pr_u1c_setup_monster_env *env,
                                          pr_u1c_approach_report *report);

#endif
