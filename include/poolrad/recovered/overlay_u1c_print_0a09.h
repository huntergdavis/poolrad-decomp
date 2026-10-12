#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_PRINT_0A09_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_PRINT_0A09_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x0A09..+0x0A9E (file
 * 0x174F..0x17E4, exactly 150 bytes, `retf` last byte, a
 * 256-byte stack local, no arguments, no result): the one handler shared by
 * ECL opcode 0x11 (PRINT) and opcode 0x12 (PRINTCLEAR). The two opcodes differ
 * only at +0x0A4B (`cmp byte [0x6F7F], 0x11`). Evidence:
 * docs/re/U1C_PRINT_0A09.md.
 *
 * In the original's order:
 *   loader(1) (0x2B:0x2A)                                              (+0x0A10..+0x0A13)
 *   [0x84E4] := 0; [0x4A00] := 1                                       (+0x0A18..+0x0A1D)
 *   [0x7006] (tag of slot 1) < 0x80:                                   (+0x0A22..+0x0A46)
 *       value = slot value(1) (0x2B:0x25);
 *       0xBA:0x1073 (far pointer to the 256-byte local, value);
 *       0xAF8:0x0B25 (far pointer DS:0x70C8 = string buffer 1, 0xFF)
 *   latch [0x6F7F] = 0x11 (PRINT):                                     (+0x0A4B..+0x0A6E)
 *       0x709:0x0768 with the six byte words (1, 0x11, 0x26, 0x16, 0x0A, 0) and the
 *       far pointer DS:0x70C8 (buffer 1: string_len[1], string_text[1])
 *   otherwise (PRINTCLEAR):                                            (+0x0A70..+0x0A91)
 *       [0x5E25] := 0x11; [0x5E24] := 1; the same call with sixth word 1
 *   [0x4A00] := 0; return                                              (+0x0A96..+0x0A9E)
 *
 * The three far callees are unresolved and modelled as fallible host hooks.
 * The model does not copy a converted number into buffer 1: hook B receives the
 * loader state and owns the write to string_len[1]/string_text[1] (the
 * original's argument shape is the Pascal string-assign convention, but the
 * callee is not decoded here). When the operand tag is >= 0x80, buffer 1 holds
 * the string operand the loader stored and the numeric calls are skipped.
 *
 * Argument widths: all six words are pushed as `mov al, imm; push ax`, so the high
 * byte of each pushed word is whatever AH held; hooks take bytes and no upper-byte
 * parity is claimed. [0x84E4] and [0x4A00] are explicit host bytes.
 *
 * Classification: bounded model (path-complete over the 150 bytes with the three
 * far callees as hooks). Not a strict translation. No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_PRINT_LATCH_PRINT 0x11u        /* cmp byte [0x6F7F], 0x11 */
#define PR_U1C_PRINT_STATE_5E25 0x11u         /* mov byte [0x5E25], 0x11 */
#define PR_U1C_PRINT_STATE_5E24 1u            /* mov byte [0x5E24], 1 */
#define PR_U1C_PRINT_BUFFER_DS 0x70C8u        /* mov di, 0x70C8: string buffer 1 */
#define PR_U1C_PRINT_STORE_MAX 0xFFu          /* mov ax, 0xFF before 0xAF8:0x0B25 */
#define PR_U1C_PRINT_LOCAL_SIZE 0x100u        /* sub sp, 0x100 */
#define PR_U1C_PRINT_OPERANDS 1u

typedef struct pr_u1c_print_env pr_u1c_print_env;

typedef struct {
    /* 0xBA:0x1073: (far pointer to the 256-byte local Pascal string, value word). Nonzero return stops
       the handler with PR_U1C_PRINT_CALLBACK. */
    int (*num_ba_1073)(void *ctx, pr_u1c_print_env *env, uint8_t local[PR_U1C_PRINT_LOCAL_SIZE], uint16_t value);
    /* 0xAF8:0x0B25: (far pointer DS:0x70C8, 0xFF) with the local string beneath. The hook owns any write to
       state->string_len[1] / string_text[1]. */
    int (*store_af8_0b25)(void *ctx, pr_u1c_print_env *env, pr_rec_ecl_loader_state *state, uint8_t max_len,
                          const uint8_t local[PR_U1C_PRINT_LOCAL_SIZE]);
    /* 0x709:0x0768: six byte words in push order and the far pointer DS:0x70C8 (string buffer 1), passed as
       the loader state's own string_len[1] and string_text[1] (no copy is made). */
    int (*print_709_0768)(void *ctx, pr_u1c_print_env *env, const uint8_t args[6], uint8_t length, const char *text);
} pr_u1c_print_hooks;

struct pr_u1c_print_env {
    uint8_t b84e4;                   /* [0x84E4] */
    uint8_t b4a00;                   /* [0x4A00] */
    pr_u1c_print_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_PRINT_OK = 0,
    PR_U1C_PRINT_ARGUMENT,   /* null state or env */
    PR_U1C_PRINT_CORE,       /* a lane-A core status stops the handler (report->core_status) before any hook; on HOOK the
                                handler has completed and the slot value read ran the engine's load hook */
    PR_U1C_PRINT_HOOK,       /* a reached hook is NULL; earlier effects retained */
    PR_U1C_PRINT_CALLBACK    /* a reached hook returned nonzero (report->callback_status); earlier effects retained */
} pr_u1c_print_status;

typedef enum {
    PR_U1C_PRINT_CALL_NUM = 1, PR_U1C_PRINT_CALL_STORE, PR_U1C_PRINT_CALL_PRINT
} pr_u1c_print_call;

typedef struct {
    unsigned numeric_path;           /* tag[1] < 0x80 */
    unsigned print_arm;              /* latch = 0x11 (PRINT); 0 = PRINTCLEAR arm */
    uint16_t value;                  /* slot value(1) (numeric path) */
    unsigned slot_read;
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status;
    int callback_status;
    pr_u1c_print_call failed_call;
    unsigned calls_n;
    pr_u1c_print_call calls[4];
} pr_u1c_print_report;

/* The handler on lane A's loader state. `report` may be NULL. */
pr_u1c_print_status pr_u1c_op_print(pr_rec_ecl_loader_state *state, pr_u1c_print_env *env,
                                    pr_u1c_print_report *report);

#endif
