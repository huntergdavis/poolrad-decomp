#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_SURPRISE_16C9_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_SURPRISE_16C9_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x16C9..+0x178E (file 0x240F..0x24D4, 198 bytes of code
 * ending in `retf`, ten bytes of locals, no arguments, no result): the handler of ECL opcode 0x23
 * (SURPRISE, reference name). The 12 bytes that follow (+0x178F..+0x179A, to the 210-byte ledger span)
 * are embedded Pascal string constants, never executed by this routine. The next routine is
 * +0x179B. Evidence: docs/re/U1C_SURPRISE_16C9.md.
 *
 * In the original's order:
 *   loader(4) (0x2B:0x2A)                                              (+0x16CF..+0x16D2)
 *   r := 0; for i = 1..4: s[i] := low BYTE of slot value(i) (0x2B:0x25)  (+0x16D7..+0x16FD)
 *   a := byte (s4 + 2 - s1); b := byte (s2 + 2 - s3)   (16-bit arithmetic, low byte kept)  (+0x16FF..+0x1722)
 *   x := AL of 0xB0:0x48 pushed (1, 6); y := AL of 0xB0:0x48 pushed (1, 6)   (two calls, in this order)
 *                                                                      (+0x1725..+0x173E)
 *   if not (x > a) [signed: x zero-extended against sign-extended a]:      (+0x1741..+0x1763)
 *       r := 1 if y > b else 3        (y zero-extended, b sign-extended, signed compare)
 *   if not (y > b): r := 2                                              (+0x1769..+0x1778)
 *   store word 0x2B:0x52 (0x6DCB, r)                                    (+0x177C..+0x1786)
 *   return
 *
 * Net result: r = 2 if y <= b (signed); else 1 if x <= a; else 0. The value 3 is always overwritten by the
 * second test and is never stored. The destination is the constant class-1 word 0x6DCB, not an operand.
 *
 * The two 0xB0:0x48 calls (unit 0x9E entry 8: count byte pushed first, range byte second) are explicit
 * fallible host hooks run in this order; no dice semantics are asserted. A NULL reached hook is a HOOK
 * status, a nonzero return a CALLBACK status, neither stores.
 *
 * Classification: bounded model composed of the lane-A loader, slot value and store word, with the roll
 * hook. Not a strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_SURPRISE_OPERANDS 4u
#define PR_U1C_SURPRISE_DEST 0x6DCBu       /* mov ax, 0x6DCB */
#define PR_U1C_SURPRISE_ROLL_COUNT 1u      /* first pushed byte */
#define PR_U1C_SURPRISE_ROLL_RANGE 6u      /* second pushed byte */

typedef struct pr_u1c_surprise_env pr_u1c_surprise_env;

typedef struct {
    /* 0xB0:0x48 with (count byte, range byte) in push order; *al is the result byte. Nonzero return stops the handler
       with PR_U1C_SURPRISE_CALLBACK. */
    int (*roll_b0_0048)(void *ctx, pr_u1c_surprise_env *env, uint8_t count, uint8_t range, uint8_t *al);
} pr_u1c_surprise_hooks;

struct pr_u1c_surprise_env {
    pr_u1c_surprise_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_SURPRISE_OK = 0,
    PR_U1C_SURPRISE_ARGUMENT,   /* null state or env */
    PR_U1C_SURPRISE_CORE,       /* a lane-A core status ended the handler (report->core_status); on HOOK the handler has
                                   completed and a class-1 slot read or the store ran the engine's hook model */
    PR_U1C_SURPRISE_HOOK,       /* a reached roll hook is NULL; nothing is stored */
    PR_U1C_SURPRISE_CALLBACK    /* a reached roll hook returned nonzero (report->callback_status); nothing is stored */
} pr_u1c_surprise_status;

typedef struct {
    uint8_t s[4];                    /* low bytes of slot values 1..4 */
    uint8_t a, b;                    /* (s4 + 2 - s1) and (s2 + 2 - s3), low bytes */
    uint8_t x, y;                    /* the two roll results */
    uint8_t result;                  /* the value stored to 0x6DCB: 0, 1 or 2 */
    unsigned rolls;                  /* roll hook calls made */
    unsigned stored;                 /* the store word call was made */
    int callback_status;
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status[4];
    pr_rec_ecl_window_status store_status;
} pr_u1c_surprise_report;

/* The handler on lane A's loader state. `report` may be NULL. */
pr_u1c_surprise_status pr_u1c_op_surprise(pr_rec_ecl_loader_state *state, pr_u1c_surprise_env *env,
                                          pr_u1c_surprise_report *report);

#endif
