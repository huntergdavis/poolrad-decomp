#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_SETUP_MONSTER_03B4_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_SETUP_MONSTER_03B4_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x03B4..+0x044A (file 0x10FA..0x1190, 151 bytes of
 * code ending in `retf`, two bytes of unused locals, no arguments, no result): the handler of ECL
 * opcode 0x0C (SETUP MONSTER, reference name). The ledger's 156-byte span adds five trailing bytes
 * (+0x044B..+0x044F), an embedded Pascal string constant never executed here; the next routine is
 * +0x0450. Evidence: docs/re/U1C_SETUP_MONSTER_03B4.md.
 *
 * In the original's order:
 *   loader(3) (0x2B:0x2A)                                              (+0x03BA..+0x03BD)
 *   [0x6F81] := AL of slot value(1)                                    (+0x03C2..+0x03CA)
 *   window-1 word w580 := the FULL AX of slot value(2)                 (+0x03CD..+0x03D9)
 *       (es:[[0x49D6] + 0x580], script address 0x6DC0: a direct window write, no store hook)
 *   [0x6F82] := AL of slot value(3)                                    (+0x03DE..+0x03E6)
 *   window-1 word w582 := zero-extended AL of 0x2B:0x3E (unit 0x25 entry 6, +0x0482) called with the
 *       three bytes [0x6AAD], [0x6AAE], [0x6AAF]                       (+0x03E9..+0x0400)
 *       (script address 0x6DC1, again a direct window write)
 *   if w580 < w582 (unsigned): w582 := w580                            (+0x0405..+0x042B)
 *   0x2B:0x48 (unit 0x25 entry 8, +0x054E) with the bytes [0x6F81], [0x6F82], low byte of w582 and the
 *       far pointer DS:0x84C8                                          (+0x042B..+0x0442)
 *   return
 *
 * The two callees are unresolved and modelled as explicit fallible host hooks (missing reached hook:
 * HOOK status; nonzero return: CALLBACK status; earlier effects, the two DS bytes and the two window
 * words, are retained). The window words are written directly (not through the store word) as the
 * original does, so no class-1 hook status applies to them. The bytes [0x6F81], [0x6F82] are explicit
 * environment bytes. The hook for 0x2B:0x48 receives the loader state because the far pointer
 * DS:0x84C8 addresses the state's b84c8/b84c9 bytes and an unknown extent of DS beyond; what the
 * callee does with it is not modelled.
 *
 * Classification: bounded model composed of the lane-A loader and slot value, with two hooks. Not a
 * strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_SETUP_MONSTER_OPERANDS 3u
#define PR_U1C_SETUP_MONSTER_W580 0x2C0u      /* window-1 word index of byte offset 0x580 (script address 0x6DC0) */
#define PR_U1C_SETUP_MONSTER_W582 0x2C1u      /* window-1 word index of byte offset 0x582 (script address 0x6DC1) */

typedef struct pr_u1c_setup_monster_env pr_u1c_setup_monster_env;

typedef struct {
    /* 0x2B:0x3E (unit 0x25 +0x0482): the three bytes [0x6AAD], [0x6AAE], [0x6AAF] pushed in that order; *al is the
       result byte. Nonzero return stops the handler with PR_U1C_SMON_CALLBACK. */
    int (*unit25_0482)(void *ctx, pr_u1c_setup_monster_env *env, uint8_t x, uint8_t y, uint8_t facing, uint8_t *al);
    /* 0x2B:0x48 (unit 0x25 +0x054E): ([0x6F81], [0x6F82], low byte of w582) and the far pointer DS:0x84C8. */
    int (*unit25_054e)(void *ctx, pr_u1c_setup_monster_env *env, pr_rec_ecl_loader_state *state,
                       uint8_t a, uint8_t b, uint8_t c);
} pr_u1c_setup_monster_hooks;

struct pr_u1c_setup_monster_env {
    uint8_t b6f81, b6f82;            /* DS bytes [0x6F81], [0x6F82] */
    pr_u1c_setup_monster_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_SMON_OK = 0,
    PR_U1C_SMON_ARGUMENT,   /* null state or env */
    PR_U1C_SMON_CORE,       /* a lane-A core status ended the handler (report->core_status); on HOOK the handler has
                               completed and a class-1 slot read ran the engine's load hook model */
    PR_U1C_SMON_HOOK,       /* a reached hook is NULL; earlier effects retained */
    PR_U1C_SMON_CALLBACK    /* a reached hook returned nonzero (report->callback_status); earlier effects retained */
} pr_u1c_smon_status;

typedef enum { PR_U1C_SMON_CALL_0482 = 1, PR_U1C_SMON_CALL_054E } pr_u1c_smon_call;

typedef struct {
    uint8_t s1, s3;                  /* low bytes of slot values 1 and 3 */
    uint16_t s2;                     /* the full slot value 2 */
    uint8_t cell;                    /* AL of 0x2B:0x3E */
    uint16_t w580, w582;             /* the final window words */
    unsigned lowered;                /* w582 := w580 was taken */
    unsigned calls_n;
    pr_u1c_smon_call calls[2];
    int callback_status;
    pr_u1c_smon_call failed_call;
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status[3];
} pr_u1c_smon_report;

/* The handler on lane A's loader state. `report` may be NULL. */
pr_u1c_smon_status pr_u1c_op_setup_monster(pr_rec_ecl_loader_state *state, pr_u1c_setup_monster_env *env,
                                           pr_u1c_smon_report *report);

#endif
