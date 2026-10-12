#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_PICTURE_0856_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_PICTURE_0856_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x0856..+0x0950 (file
 * 0x159C..0x1696, exactly 251 bytes, 91 instructions, `retf`, no arguments, no
 * result, no trailing data: the next prologue is +0x0951): the handler of ECL
 * opcode 0x0E (PICTURE, one operand), dispatcher case +0x2FBD (its only near
 * caller). The five bytes +0x0851..+0x0855 before it are two Pascal string
 * constants it reads with cs: offsets (cs:0x0851 of length 0, cs:0x0852 of
 * length 3). Evidence: docs/re/U1C_PICTURE_0856.md.
 *
 * In the original's order:
 *   loader(1)                                   0x2B:0x2A   pr_rec_ecl_load_operands(state, 1)
 *   id := low byte of slot value(1)             0x2B:0x25   pr_rec_ecl_slot_value
 *   window-0 word 0xE6 (byte offset 0x1CC) = 0 and id <> 0xFF:
 *       copy cs:0x0851 to a local; resident 0x3F1:0x02E8 (3, 3, 0xD, 0xD, 0, 0xF, the local string)
 *   id <> 0xFF:                                                          (+0x08A7..+0x0911)
 *       [0x84C9] := 1; [0x84E0] := 1
 *       window-1 word 0x2E1 (byte offset 0x5C2) = 0xFF (word compare):
 *           copy cs:0x0852 to a local; unit 0x3b9 entry 2 (0x3D0:0x2A) with
 *           (that string, id, 0, DS offset 0x6ABE); then unit 0x3b9 entry 1
 *           (0x3D0:0x25) with (3, 3, 1, far pointer [0x6AC6]:[0x6AC4])
 *       otherwise: unit 0x25 entry 7 (0x2B:0x43) with (id, low byte of window-1 byte 0x5C2)
 *       return
 *   id = 0xFF:                                                           (+0x0913..+0x0950)
 *       [0x49F9] > 1, or [0x49FA] = 1: and ([0x84E0] <> 0 or [0x84E3] <> 0):
 *           unit 0x3b4 entry 1 (0x3CA:0x25); [0x84E0] := 0; [0x84E3] := 0; [0x84E1] := 1
 *       [0x84C8] := 0; [0x84C9] := 0
 *
 * Every callee except the two merged lane-A cores is a fallible host hook:
 * it returns 0 on success; nonzero stops the run with PR_U1C_PICTURE_CALLBACK,
 * keeping the effects done so far. A NULL reached hook returns
 * PR_U1C_PICTURE_HOOK before the callee's slot in the order (earlier effects
 * retained). Core failures (loader or slot value BOUNDS/UNRESOLVED/ARGUMENT)
 * stop before any effect; a class-1 slot read lets the handler finish and then
 * reports core_status HOOK, as lane A's SAVE model does.
 *
 * Argument widths: the original pushes most of these as `mov al, imm/[mem]; push ax`, so the
 * high byte of each pushed word is whatever AH held (not fixed here), and a few as
 * `mov ax, imm; push ax` (the 3, 3 of 0x3D0:0x25). The hooks take byte (or the
 * stated word) values; whether each callee reads only the low byte is NOT
 * established, so full argument parity for the upper bytes is not claimed.
 *
 * Classification: bounded model (path-complete over the 251 bytes with host
 * hooks for the resident box call, the picture routines of unit 0x3b9, unit
 * 0x25 entry 7 and the view refresh). Not a strict translation. It makes no
 * claim about which branch the live game takes at PC 0x997B. No game-function
 * credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_PICTURE_OPERANDS 1u
#define PR_U1C_PICTURE_NONE 0xFFu            /* cmp byte [bp-1], 0xFF */
#define PR_U1C_PICTURE_W0_WORD 0xE6u         /* window-0 byte offset 0x1CC */
#define PR_U1C_PICTURE_W1_WORD 0x2E1u        /* window-1 byte offset 0x5C2 */
#define PR_U1C_PICTURE_W1_MATCH 0xFFu        /* cmp word, 0xFF */
#define PR_U1C_PICTURE_STR0_MAX 0u           /* cs:0x0851 is copied into the 1-byte slot [bp-2] */
#define PR_U1C_PICTURE_STR1_MAX 3u           /* cs:0x0852 is copied into [bp-5..-2]; [bp-1] holds the id */
#define PR_U1C_PICTURE_BUFFER_DS 0x6ABEu     /* mov di, 0x6ABE; push ds */

typedef struct pr_u1c_picture_env pr_u1c_picture_env;

typedef struct {
    /* 0x3F1:0x02E8: resident; (3, 3, 0xD, 0xD, 0, 0xF) as the six byte words, then the string */
    int (*box_3f1_02e8)(void *ctx, pr_u1c_picture_env *env, const uint8_t args[6], const uint8_t *pascal_string);
    /* 0x3D0:0x2A: unit 0x3b9 entry 2 (string, id, 0, DS offset) */
    int (*pic_3d0_2a)(void *ctx, pr_u1c_picture_env *env, const uint8_t *pascal_string, uint8_t id, uint8_t zero,
                      uint16_t ds_offset);
    /* 0x3D0:0x25: unit 0x3b9 entry 1 (3, 3, 1, far pointer words [0x6AC4] then [0x6AC6]) */
    int (*pic_3d0_25)(void *ctx, pr_u1c_picture_env *env, uint8_t a, uint8_t b, uint8_t c, uint16_t w6ac4, uint16_t w6ac6);
    /* 0x2B:0x43: unit 0x25 entry 7 (id, low byte of window-1 word 0x2E1) */
    int (*unit25_43)(void *ctx, pr_u1c_picture_env *env, uint8_t id, uint8_t byte_5c2);
    /* 0x3CA:0x25: unit 0x3b4 entry 1 */
    int (*view_3ca_25)(void *ctx, pr_u1c_picture_env *env);
} pr_u1c_picture_hooks;

/* DS bytes and the two string constants this handler uses that lane A's loader state does not carry
 * ([0x84C8], [0x84C9], [0x84E0] are the state's b84c8, b84c9, b84e0). */
struct pr_u1c_picture_env {
    uint8_t b84e1, b84e3;
    uint8_t b49f9, b49fa;
    uint16_t w6ac4, w6ac6;
    const uint8_t *str_0851;          /* Pascal string cs:0x0851 (host data; must be non-NULL with length 0 where reached) */
    const uint8_t *str_0852;          /* Pascal string cs:0x0852 (host data; non-NULL, length <= 3 where reached) */
    pr_u1c_picture_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_PICTURE_OK = 0,
    PR_U1C_PICTURE_ARGUMENT,   /* null state or env */
    PR_U1C_PICTURE_CORE,       /* a lane-A core status stops the handler (report->core_status); on HOOK the handler
                                  has completed and the value read ran the engine's load hook */
    PR_U1C_PICTURE_STRING,     /* a reached string constant is NULL or longer than its local slot allows */
    PR_U1C_PICTURE_HOOK,       /* a reached hook is NULL */
    PR_U1C_PICTURE_CALLBACK    /* a reached hook returned nonzero (report->callback_status) */
} pr_u1c_picture_status;

typedef enum {
    PR_U1C_PIC_CALL_BOX = 1, PR_U1C_PIC_CALL_2A, PR_U1C_PIC_CALL_25, PR_U1C_PIC_CALL_UNIT25_43, PR_U1C_PIC_CALL_VIEW
} pr_u1c_picture_call;

typedef struct {
    uint8_t id;                      /* low byte of slot value 1 (valid when read) */
    unsigned slot_read;
    unsigned boxed;                  /* the resident box call was made */
    unsigned shown;                  /* id <> 0xFF path taken */
    unsigned cleared;                /* id = 0xFF path taken */
    unsigned view_refreshed;
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status;
    int callback_status;
    pr_u1c_picture_call failed_call;
    unsigned calls_n;
    pr_u1c_picture_call calls[8];
} pr_u1c_picture_report;

/* The handler on lane A's loader state. `report` may be NULL. */
pr_u1c_picture_status pr_u1c_op_picture(pr_rec_ecl_loader_state *state, pr_u1c_picture_env *env,
                                        pr_u1c_picture_report *report);

#endif
