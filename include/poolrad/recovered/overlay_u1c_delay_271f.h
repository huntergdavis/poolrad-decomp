#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_DELAY_271F_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_DELAY_271F_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x271F..+0x272C (file 0x3465..0x3472, exactly 14 bytes,
 * 6 instructions, `retf` last byte, no locals, no operands, no result): the handler of ECL opcode 0x3A
 * (DELAY, reference name), plus the two resident routines it reaches. The Pascal string constants that
 * trail it (to the next prologue at file 0x34B8) are never executed. Evidence: docs/re/U1C_DELAY_271F.md.
 *
 * In the original's order:
 *   [0x49ED] := [0x49ED] + 1  (the PC skips the opcode byte)             (+0x2722)
 *   lcall 0x709:0x0DBE (resident, START.EXE image 0x7E4E..0x7E62, 21 bytes, 10 instructions):   (+0x2726)
 *       arg := word ([0x49E2] * 225)   (`mul dx` with dx = 0xE1; at most 255 * 225 = 57,375, no overflow)
 *       lcall 0xA40:0x029C with the word arg (retf 2)
 *   return
 * 0xA40:0x029C (resident, image 0xA69C) is a calibrated busy-wait of `arg` units (Turbo Pascal Delay): for arg = 0
 * it returns at once; otherwise it repeats arg times a spin loop that loads CX := DS:[0x84FC], calls the
 * spin routine at image 0xA6BD and ends with `loope 0xA6BD`: the loop runs while the BIOS data byte 0x40:0x0000 is unchanged
 * and CX <> 0 after its decrement. CX is a 16-bit counter, so an initial CX of 0 wraps to 0xFFFF and may run 65,536
 * iterations; no iteration bound is asserted here. It reads memory and changes no game
 * state; its only observable effect is elapsed time, which depends on the calibration word and on that BIOS byte.
 * The model therefore passes the exact word to an explicit host timing hook and asserts nothing about the
 * duration. [0x49E2] is loaded from window-0 byte 0x1F8 by the world-save loader (unit 0x6f +0x190B).
 *
 * Classification: bounded model: the handler's 6 instructions and the resident argument routine are translated; the
 * timing busy-wait is the host boundary. Not a strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_DELAY_UNIT_MS 225u        /* mov dx, 0xE1 ; mul dx */

/* The resident argument routine 0x709:0x0DBE: the word it passes to 0xA40:0x029C for the byte [0x49E2]. */
uint16_t pr_res_0709_0dbe_arg(uint8_t b49e2);

typedef struct pr_u1c_delay_env pr_u1c_delay_env;

typedef struct {
    /* 0xA40:0x029C with the word `ms` pushed. Returns 0 normally; nonzero stops with PR_U1C_DELAY_CALLBACK. */
    int (*delay_a40_029c)(void *ctx, pr_u1c_delay_env *env, uint16_t ms);
} pr_u1c_delay_hooks;

struct pr_u1c_delay_env {
    uint8_t b49e2;                   /* DS byte [0x49E2] */
    pr_u1c_delay_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_DELAY_OK = 0,
    PR_U1C_DELAY_ARGUMENT,   /* null state or env */
    PR_U1C_DELAY_HOOK,       /* the timing hook is NULL; the PC has already advanced */
    PR_U1C_DELAY_CALLBACK    /* the timing hook returned nonzero (report->callback_status); the PC has already advanced */
} pr_u1c_delay_status;

typedef struct {
    uint16_t ms;                     /* the word passed to the timing routine */
    unsigned called;
    int callback_status;
} pr_u1c_delay_report;

/* The handler on lane A's loader state. `report` may be NULL. */
pr_u1c_delay_status pr_u1c_op_delay(pr_rec_ecl_loader_state *state, pr_u1c_delay_env *env, pr_u1c_delay_report *report);

#endif
