#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_RANDOM_0234_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_RANDOM_0234_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x0234..+0x0291 (file
 * 0x0F7A..0x0FD7, exactly 94 bytes, 35 instructions, `retf` last byte, four
 * bytes of locals, no arguments, no result): the handler of ECL opcode 0x08
 * (RANDOM). The next routine (+0x0292) is SAVE. Evidence:
 * docs/re/U1C_RANDOM_0234.md. The name is the reference name only: the three
 * runtime calls are opaque and no generator behavior is asserted.
 *
 * In the original's order:
 *   loader(2) (0x2B:0x2A)                                              (+0x023A..+0x023D)
 *   v := low byte of slot value(1) (0x2B:0x25)                        (+0x0242..+0x024A)
 *   v := v + 1 if v < 0xFF, else v stays 0xFF (a byte compare, jae)    (+0x024D..+0x0253)
 *   dest := word([0x7087], [0x7047]) via 0x2B:0x4D (slot 2)            (+0x0256..+0x0263)
 *   push the word v (AH = 0); 0xAF8:0x11C0                             (+0x0266..+0x026C)
 *   xor dx, dx; 0xAF8:0x119A                                           (+0x0271..+0x0273)
 *   0xAF8:0x119E; r := AL                                              (+0x0278..+0x027D)
 *   0x2B:0x52 (dest, zero-extended r), the store word call              (+0x0280..+0x0289)
 *   return
 *
 * The three 0xAF8 runtime routines are explicit fallible hooks run in this order. Their
 * registers flow between them (AX/DX left by one are the next one's inputs; `xor dx, dx`
 * sets DX before the second); that register contract is not decoded here, so the model
 * passes only the stack word and the DX value it can see and takes AL from the last hook.
 * No PRNG semantics and no saved result are modelled: a host that wants a value supplies it.
 *
 * Classification: bounded model (path-complete over the 94 bytes; composed of the lane-A
 * loader, slot value, operand word and store word, with the runtime trio as hooks). Not a
 * strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_RANDOM_OPERANDS 2u

typedef struct pr_u1c_random_env pr_u1c_random_env;

typedef struct {
    /* 0xAF8:0x11C0: one word argument pushed (the incremented byte, AH = 0). Nonzero return stops the
       handler with PR_U1C_RANDOM_CALLBACK. */
    int (*rt_af8_11c0)(void *ctx, pr_u1c_random_env *env, uint16_t arg);
    /* 0xAF8:0x119A, called right after `xor dx, dx` (dx is passed as 0; AX is whatever the previous call
       left and is not modelled). */
    int (*rt_af8_119a)(void *ctx, pr_u1c_random_env *env, uint16_t dx);
    /* 0xAF8:0x119E; *al is the result byte the original reads from AL afterwards. */
    int (*rt_af8_119e)(void *ctx, pr_u1c_random_env *env, uint8_t *al);
} pr_u1c_random_hooks;

struct pr_u1c_random_env {
    pr_u1c_random_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_RANDOM_OK = 0,
    PR_U1C_RANDOM_ARGUMENT,   /* null state or env */
    PR_U1C_RANDOM_CORE,       /* a lane-A core status ended the handler (report->core_status), before any hook for
                                 the loader and slot value; after the store attempt for the store word. On HOOK the
                                 handler has completed. */
    PR_U1C_RANDOM_HOOK,       /* a reached hook is NULL; earlier hooks ran, nothing is stored */
    PR_U1C_RANDOM_CALLBACK    /* a reached hook returned nonzero (report->callback_status); nothing is stored */
} pr_u1c_random_status;

typedef enum {
    PR_U1C_RANDOM_CALL_11C0 = 1, PR_U1C_RANDOM_CALL_119A, PR_U1C_RANDOM_CALL_119E
} pr_u1c_random_call;

typedef struct {
    uint8_t slot_byte;               /* low byte of slot value(1), before the increment */
    uint8_t argument;                /* the incremented (saturating) byte pushed to 0xAF8:0x11C0 */
    uint8_t result;                  /* AL after 0xAF8:0x119E */
    uint16_t dest;                   /* word(slot 2) when reached */
    unsigned stored;                 /* the store word call was made */
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status;
    pr_rec_ecl_window_status store_status;
    int callback_status;
    pr_u1c_random_call failed_call;
    unsigned calls_n;
    pr_u1c_random_call calls[3];
} pr_u1c_random_report;

/* The handler on lane A's loader state. `report` may be NULL. */
pr_u1c_random_status pr_u1c_op_random(pr_rec_ecl_loader_state *state, pr_u1c_random_env *env,
                                      pr_u1c_random_report *report);

#endif
