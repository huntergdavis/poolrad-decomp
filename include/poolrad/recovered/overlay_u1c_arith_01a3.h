#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_ARITH_01A3_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_ARITH_01A3_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x01A3..+0x0233 (file 0x0EE9..0x0F79, exactly 145
 * bytes, `retf` last byte, ten bytes of locals, no arguments, no result): the one handler shared by
 * ECL opcodes 0x04 ADD, 0x05 SUBTRACT, 0x06 DIVIDE and 0x07 MULTIPLY (reference names). The next routine
 * (+0x0234) is RANDOM. Evidence: docs/re/U1C_ARITH_01A3.md.
 *
 * In the original's order:
 *   loader(3) (0x2B:0x2A)                                              (+0x01A9..+0x01AC)
 *   for i = 1, 2: v[i] := the full 16-bit slot value(i) (0x2B:0x25)    (+0x01B1..+0x01D4)
 *   dest := word([0x7088], [0x7048]) via 0x2B:0x4D (slot 3)            (+0x01D6..+0x01E3)
 *   by the latch [0x6F7F]:                                             (+0x01E6..+0x0222)
 *       0x04: r := v1 + v2                      (16-bit, wraps)
 *       0x05: r := v2 - v1                      (16-bit, wraps; the SECOND operand minus the first)
 *       0x06: r := v1 div v2                    (unsigned 16-bit divide, dx = 0; v2 = 0 faults)
 *       0x07: r := low 16 bits of v1 * v2       (unsigned `mul`; the high word is dropped)
 *       other: r is the unwritten stack word [bp-9]
 *   store word 0x2B:0x52 (dest, r)                                      (+0x0225..+0x022B)
 *   return
 *
 * All arithmetic is unsigned 16-bit. A divisor of 0 raises the processor divide error (INT 0), whose
 * vector the runtime sets (START.EXE install, `mov ax,0x2500` with handler cs:0x00DF: `mov ax,0xC8` and
 * the runtime halt path at cs:0x00E2): runtime error 200. The model reports that as DIVIDE_ERROR with no
 * store and nothing else modelled of the halt. The quotient of a word by a nonzero word always fits, so
 * there is no quotient overflow fault. For a latch outside 4..7 the stored word is stack residue, supplied
 * by the host (`stack_residue`) and flagged with status RESIDUE; the dispatcher only reaches this handler
 * with 4..7.
 *
 * Classification: bounded model composed of the lane-A loader, slot value, operand word and store word.
 * The load-word helper resolves the 0x6B18, 0x6B9B and 0x6BA0 active-record branches when supplied;
 * other hook and runtime paths remain bounded. Not a strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_ARITH_OPERANDS 3u
#define PR_U1C_ARITH_ADD 0x04u
#define PR_U1C_ARITH_SUBTRACT 0x05u
#define PR_U1C_ARITH_DIVIDE 0x06u
#define PR_U1C_ARITH_MULTIPLY 0x07u
#define PR_U1C_ARITH_RUNTIME_ERROR_DIVIDE 200u   /* mov ax, 0xC8 */

typedef enum {
    PR_U1C_ARITH_OK = 0,
    PR_U1C_ARITH_ARGUMENT,       /* null state */
    PR_U1C_ARITH_CORE,           /* a lane-A core status ended the handler (report->core_status); on HOOK the handler has
                                    completed and a slot read or the store ran the engine's class-1 hook model */
    PR_U1C_ARITH_DIVIDE_ERROR,   /* latch 6 with v2 = 0: processor divide error, runtime error 200; no store */
    PR_U1C_ARITH_RESIDUE         /* latch not 4..7: the stored word is the host's stack_residue (store done) */
} pr_u1c_arith_status;

typedef struct {
    uint16_t v1, v2;                 /* slot values 1 and 2 (valid when read) */
    uint16_t result;                 /* the word passed to the store */
    uint16_t dest;                   /* word(slot 3) when reached */
    unsigned stored;                 /* the store word call was made */
    unsigned runtime_error;          /* 200 on DIVIDE_ERROR, else 0 */
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status[2];
    pr_rec_ecl_window_status store_status;
} pr_u1c_arith_report;

/* The handler on lane A's loader state; the opcode is state->latch. `report` may be NULL. */
pr_u1c_arith_status pr_u1c_op_arith(pr_rec_ecl_loader_state *state, uint16_t stack_residue,
                                    pr_u1c_arith_report *report);

#endif
