#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_ON_GOTO_18F8_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_ON_GOTO_18F8_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x18F8..+0x198C (file
 * 0x263E..0x26D2, exactly 149 bytes, `retf` last byte, six bytes of locals, no
 * arguments, no result): the one handler shared by ECL opcode 0x25 (ON GOTO)
 * and every other opcode that dispatches to it (0x26, ON GOSUB). Evidence:
 * docs/re/U1C_ON_GOTO_18F8.md.
 *
 * In the original's order:
 *   found := 0                                                         (+0x18FE)
 *   loader(2) (0x2B:0x2A)                                              (+0x1902..+0x1905)
 *   sel := low byte of slot value(1); n := low byte of slot value(2)   (+0x190A..+0x191D)
 *   [0x49ED] := [0x49ED] - 1   (undoes the loader's final PC increment) (+0x1920)
 *   loader(n)                                                          (+0x1924..+0x1928)
 *   for i := 0 while i < n and not found, i += 1:                      (+0x192D..+0x1987)
 *       sel = i:                                                       (+0x193F..+0x1945)
 *           latch [0x6F7F] = 0x25: PC := word(low[i+1], high[i+1]) via 0x2B:0x4D
 *           otherwise: push call frame (0x2B:0x75) for slot i+1
 *           found := 1
 *   return
 *
 * Both comparisons are byte comparisons (sel against i, i against n). Only the first
 * matching index is acted on; no match, or n = 0, leaves the PC just past the n
 * operands (the second loader's own increment). A slot whose tag is not 1, 2 or 3 supplies
 * the stale high byte left in the loader state. The slot value reads can run the engine's
 * load hook (class-1 addresses): as in the PICTURE model, the handler then completes and
 * reports HOOK. The count n must be below PR_REC_ECL_SLOT_COUNT (a model bound, the
 * loader reports COUNT, mapped to an ARGUMENT handler status, with the first loader's
 * state and the PC decrement already done).
 *
 * Classification: bounded model composed of the merged lane-A loader, slot value, operand
 * word (strict full-C entry 9) and bounded call-frame model (heap list as an array). No new
 * hooks. Not a strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_ON_GOTO_LATCH_GOTO 0x25u        /* cmp byte [0x6F7F], 0x25 */

typedef enum {
    PR_U1C_ON_GOTO_OK = 0,
    PR_U1C_ON_GOTO_ARGUMENT,   /* null state */
    PR_U1C_ON_GOTO_CORE        /* a lane-A core status ended the handler (report->core_status); on HOOK the handler
                                  has completed and a slot value read ran the engine's load hook */
} pr_u1c_on_goto_status;

typedef struct {
    uint8_t selector;                /* low byte of slot value(1) */
    uint8_t count;                   /* low byte of slot value(2) */
    unsigned selected;               /* an index i < count matched the selector */
    unsigned went_goto;              /* latch 0x25 arm taken (else the call-frame arm) */
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status[2];
} pr_u1c_on_goto_report;

/* The handler on lane A's loader state. `report` may be NULL. */
pr_u1c_on_goto_status pr_u1c_op_on_goto(pr_rec_ecl_loader_state *state, pr_u1c_on_goto_report *report);

#endif
