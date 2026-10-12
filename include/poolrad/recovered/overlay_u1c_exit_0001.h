#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_EXIT_0001_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_EXIT_0001_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x0001..+0x00F3 (file
 * 0x0D47..0x0E39, exactly 243 bytes, 91 instructions, `retf` last byte, no
 * arguments, no result; the next routine is +0x00F4, and the byte at +0x0000
 * before it is the empty Pascal string it copies through cs:0x0000): the
 * handler of ECL opcode 0x00 (EXIT, dispatcher case +0x2F2D). Evidence:
 * docs/re/U1C_EXIT_0001.md.
 *
 * In the original's order:
 *   [0x84E2] <> 0:                                                       (+0x0007..+0x0061)
 *       copy cs:0x0000 (the empty string); resident 0x3F1:0x02E8 with the byte words
 *       (1, 1, 0x0F, 0x0F, 0, 0x0F) and the string
 *       window-0 word 0xE6 (byte offset 0x1CC) <> 0: copy it again; 0x3F1:0x02E8 with
 *       (3, 3, 0x0D, 0x0D, 0, 0x0F)
 *   [0x4430] <> 0: [0x5D92]:[0x5D94] := [0x4436]:[0x4438]; [0x4430] := 0      (+0x0066..+0x007B)
 *   FillChar(DS:0x84C8, 2, 0): [0x84C8] := [0x84C9] := 0                      (+0x0080..+0x008C)
 *   [0x84E0] := 0; [0x84E2] := 0; [0x442E] := 1; [0x49ED] += 1 (the PC)        (+0x0091..+0x00A0)
 *   free every node of the 6-byte call list at far [0x6F70] (next link at +2/+4,
 *       FreeMem(node, 6) per node) until it is nil                              (+0x00A4..+0x00DF)
 *   [0x5E25] := 0x11; [0x5E24] := 1; [0x84DE] := 0                             (+0x00E1..+0x00EB)
 * The call list is represented by the loader state's call_stack array: the
 * model empties it and reports the node count; there is no heap parity (no
 * allocator, no FreeMem). The empty string constant is length 0 by the byte
 * before the routine. The resident box call is a fallible host hook that runs
 * only under [0x84E2] <> 0.
 *
 * Argument widths: the box call's six values and the (1, 1, ...) words are pushed as
 * `mov al, imm; push ax`, so the high byte of each pushed word is whatever AH held; the
 * hook takes bytes. Whether the callee reads only the low bytes is NOT established, so no
 * argument parity for the upper bytes is claimed.
 *
 * Classification: bounded model (path-complete over the 243 bytes with the
 * resident call as a hook and the heap list as the host array). Not a strict
 * translation. No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_EXIT_STATE_5E25 0x11u        /* mov byte [0x5E25], 0x11 */
#define PR_U1C_EXIT_STATE_5E24 1u           /* mov byte [0x5E24], 1 */
#define PR_U1C_EXIT_W0_WORD 0xE6u           /* window-0 byte offset 0x1CC */

typedef struct pr_u1c_exit_env pr_u1c_exit_env;

typedef struct {
    /* 0x3F1:0x02E8, resident, unresolved: six byte words in push order and the Pascal string
       (length 0). Returns 0 on success; nonzero stops the handler with PR_U1C_EXIT_CALLBACK. */
    int (*box_3f1_02e8)(void *ctx, pr_u1c_exit_env *env, const uint8_t args[6], const uint8_t *pascal_string);
} pr_u1c_exit_hooks;

/* DS bytes and words this handler uses that lane A's loader state does not carry
 * ([0x84C8], [0x84C9], [0x84E0], [0x84E2], [0x442E], the PC, the call list, [0x5E24], [0x5E25] are the
 * state's b84c8, b84c9, b84e0, b84e2, b442e, pc, call_stack/call_depth, b5e24, b5e25). */
struct pr_u1c_exit_env {
    uint8_t b4430;                   /* [0x4430] */
    uint16_t w4436, w4438;           /* [0x4436], [0x4438]: the saved active-character far pointer */
    uint16_t w5d92, w5d94;           /* [0x5D92], [0x5D94]: the active-character far pointer */
    uint8_t b84de;                   /* [0x84DE], cleared */
    pr_u1c_exit_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_EXIT_OK = 0,
    PR_U1C_EXIT_ARGUMENT,   /* null state/env, or call_depth above the array: nothing done */
    PR_U1C_EXIT_HOOK,       /* a box call is reached and its hook is NULL. If it is the FIRST box call, nothing has
                               been done. If it is the SECOND call (the first call removed the hook before it), the
                               first call's effects are retained (report->boxes == 1) and nothing after it is done */
    PR_U1C_EXIT_CALLBACK    /* the box hook returned nonzero (report->callback_status); effects before it retained */
} pr_u1c_exit_status;

typedef struct {
    unsigned boxes;                  /* box calls made (0, 1 or 2) */
    unsigned freed_nodes;            /* call-list nodes released */
    unsigned restored_character;     /* [0x4430] was nonzero */
    int callback_status;
    unsigned failed_box;             /* 1 or 2 when a box hook failed */
} pr_u1c_exit_report;

/* The handler. `report` may be NULL. */
pr_u1c_exit_status pr_u1c_op_exit(pr_rec_ecl_loader_state *state, pr_u1c_exit_env *env, pr_u1c_exit_report *report);

#endif
