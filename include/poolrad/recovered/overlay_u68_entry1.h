#ifndef POOLRAD_RECOVERED_OVERLAY_U68_ENTRY1_H
#define POOLRAD_RECOVERED_OVERLAY_U68_ENTRY1_H

/* Portable C model of DOS v1.3 GAME.OVR unit 0x68 public entry 1
 * (+0x0D6D..+0x10A6, file 0x1F458.., run-time stub of unit 0x68), exactly
 * 826 bytes, 350 instructions, 55 basic blocks, 43 jumps all internal, no
 * loops, `retf`, no arguments, no result. The gameplay loop (unit 0x1c
 * +0x3473..) calls it after the script run; the next prologue is the 5-byte
 * leaf +0x10A7. Evidence: docs/re/U68_ENTRY1.md.
 *
 * Flow in the original's order:
 *   [bp-1] := 0                                                         (+0x0D74)
 *   [0x49FA] <> 1 -> (2..4: window-1 word 0x592 < 0xFF -> +0x0818 and
 *       resident 0xBA:0x290B, else word 0x592 := 0) -> tail             (+0x0D7F, +0x1050..+0x1078)
 *   word 0x592 >= 0xFF -> word 0x592 := 0 -> tail                       (+0x0D86..+0x0D8F, +0x1043)
 *   r := unit 0x3bd entry 3 (x, y, facing) = merged pr_u3bd_entry3      (+0x0D92..+0x0D9E)
 *   r = 1 -> [bp-1] := 1 -> commit
 *   r = 2 -> message block A, r = 3 -> message block B, other -> commit  (+0x0DAE..+0x0DB2, +0x0EED..+0x0EF1)
 *   message block (A: +0x0DB5..+0x0EEA, B: +0x0EF4..+0x1026):
 *       s := ''; [0x6DDE] <> 0 -> s := lit0 (cs:0xD4C, kept to 40 chars)
 *       [0x6DDF] <> 0 and near +0x0236(6) <> 0 -> s := s + lit1 (cs:0xD51)
 *       [0x6DE0] <> 0 and (A: near +0x05EE(0x1F) <> 0 | B: near +0x0236(5) <> 0) -> s := s + lit2 (cs:0xD57)
 *       s := s + lit3 (cs:0xD5E); every store keeps at most 40 characters
 *       s = lit3 -> near +0x0000 (x, y, facing), [bp-1] stays 0
 *       else key := input(prompt = lit4 (cs:0xD64), s, 0xD, 0xA, 0xF, 0, 0)  (unit 0x3b0 entry via 0x3C5:0x2A)
 *            key 'B' -> [bp-1] := near +0x0294;  'K' -> near +0x0648;
 *            'P' -> block A: [bp-1] := near +0x04DF; block B: [0x6DDF] := 0 and no call (+0x1015)
 *   commit (+0x1027..+0x1042): [bp-1] <> 0 -> near +0x073B, else resident
 *       0x8AE:0x00DC with word [0x2622]; then resident 0xBA:0x290B
 *   tail (+0x107B..+0x10A6): 0x3D0:0x2F on DS:0x6ABE, 0x464:0x2595 on DS:0x6B01
 *       and DS:0x6B06, [0x6B00] := 0xFF, [0x6B05] := 0xFF
 * The runtime string routines 0xAF8:0x0B0B/0xB98/0xB25/0xC10 (load, append
 * with a 255 cap, truncating store, compare) were read from START.EXE and
 * are implemented here. The five string literals are host data (not game
 * text in this tree).
 *
 * Classification: bounded model of the 826 bytes. NOT strict full C: the
 * near callees +0x0000/+0x0236/+0x0294/+0x04DF/+0x05EE/+0x0648/+0x073B/+0x0818,
 * the key input, 0x3D0:0x2F, the two 0x464 calls, 0x8AE:0xDC and the two
 * 0x290B calls are host hooks (several are merged elsewhere and are wired in
 * by the host); entry 3 is the merged translation. No game-function credit
 * is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u3bd_field.h"

#define PR_U68_E1_WINDOW1_WORD 0x592u      /* es:[di + 0x592] */
#define PR_U68_E1_WINDOW1_MIN_SIZE 0x594u
#define PR_U68_E1_WORD_LIMIT 0xFFu         /* cmp word, 0xFF; jb / jae */
#define PR_U68_E1_MSG_MAX 0x28u            /* mov ax, 0x28 before every 0xB25 */
#define PR_U68_E1_PROMPT_MAX 8u            /* [bp-0x35] .. [bp-0x2d]: a 9-byte Pascal string slot */
#define PR_U68_E1_LIT_COUNT 5u             /* cs:0xD4C, 0xD51, 0xD57, 0xD5E, 0xD64 */
#define PR_U68_E1_KEY_B 0x42u
#define PR_U68_E1_KEY_P 0x50u
#define PR_U68_E1_KEY_K 0x4Bu
#define PR_U68_E1_TAIL_BUFFER_DS 0x6ABEu
#define PR_U68_E1_TAIL_REC1_DS 0x6B01u
#define PR_U68_E1_TAIL_REC2_DS 0x6B06u

typedef struct pr_u68_e1_state pr_u68_e1_state;

/* Hooks for the unresolved or separately merged callees. NULL = nothing happens (0 for the ones that
 * return a value). `st` is the live state so a host adapter can run another merged model on it. */
typedef struct {
    void (*near_0000)(void *ctx, pr_u68_e1_state *st, uint8_t x, uint8_t y, uint8_t facing);
    uint8_t (*near_0236)(void *ctx, pr_u68_e1_state *st, uint8_t arg);              /* AL */
    uint8_t (*near_0294)(void *ctx, pr_u68_e1_state *st);                           /* AL */
    uint8_t (*near_04df)(void *ctx, pr_u68_e1_state *st);                           /* AL */
    int (*near_05ee)(void *ctx, pr_u68_e1_state *st, uint8_t arg);                  /* DX:AX nonzero */
    uint8_t (*near_0648)(void *ctx, pr_u68_e1_state *st);                           /* AL */
    void (*near_073b)(void *ctx, pr_u68_e1_state *st);
    void (*near_0818)(void *ctx, pr_u68_e1_state *st);
    uint8_t (*key_input)(void *ctx, pr_u68_e1_state *st, const uint8_t *prompt, const uint8_t *message,
                         uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e);   /* 0x3C5:0x2A, AL */
    void (*far_3d0_2f)(void *ctx, pr_u68_e1_state *st, uint16_t ds_offset);
    void (*far_464_2595)(void *ctx, pr_u68_e1_state *st, uint16_t ds_offset);
    void (*resident_8ae_00dc)(void *ctx, pr_u68_e1_state *st, uint16_t word);
    void (*resident_ba_290b)(void *ctx, pr_u68_e1_state *st);
} pr_u68_e1_hooks;

struct pr_u68_e1_state {
    uint8_t selector;                 /* [0x49FA] */
    uint8_t *window1;                 /* host memory for far [0x49D6]; word 0x592 read and cleared */
    size_t window1_size;
    uint8_t party_x, party_y, facing; /* [0x6AAD], [0x6AAE], [0x6AAF] (the merged writer changes x, y) */
    uint8_t ds_6dde[3];               /* [0x6DDE], [0x6DDF], [0x6DE0] */
    uint16_t word_2622;               /* [0x2622] */
    uint8_t ds_6b00, ds_6b05;         /* written 0xFF at the tail */
    const uint8_t *lit[PR_U68_E1_LIT_COUNT];   /* Pascal strings (length byte first) for cs:0xD4C.. ; NULL = empty */
    pr_u3bd_field_state field;        /* merged entry 3's inputs */
    const pr_u68_e1_hooks *hooks;     /* may be NULL */
    void *ctx;
};

typedef enum {
    PR_U68_E1_OK = 0,
    PR_U68_E1_ARGUMENT   /* null state; window1 shorter than 0x594; a prompt literal longer than 8; or the
                            merged entry 3 reporting ARGUMENT (null buffer). Nothing is written. */
} pr_u68_e1_status;

typedef enum {
    PR_U68_E1_CALL_ENTRY3 = 1,
    PR_U68_E1_CALL_0000, PR_U68_E1_CALL_0236, PR_U68_E1_CALL_0294, PR_U68_E1_CALL_04DF,
    PR_U68_E1_CALL_05EE, PR_U68_E1_CALL_0648, PR_U68_E1_CALL_073B, PR_U68_E1_CALL_0818,
    PR_U68_E1_CALL_KEY, PR_U68_E1_CALL_3D0, PR_U68_E1_CALL_464, PR_U68_E1_CALL_8AE,
    PR_U68_E1_CALL_290B
} pr_u68_e1_call;

typedef struct {
    unsigned selector_branch;     /* 1 main, 2 = [0x49FA] in 2..4, 0 otherwise */
    unsigned window_cleared;      /* word 0x592 was >= 0xFF and was cleared, or cleared at +0x1070 */
    uint8_t entry3_result;        /* AL of entry 3 (main path only) */
    unsigned block;               /* 0 none, 1 = result 2 block (A), 2 = result 3 block (B) */
    uint8_t message[0x29];        /* the Pascal message s at the compare (length byte first) */
    unsigned message_equals_lit3; /* the 0xC10 compare was equal */
    uint8_t key;                  /* key returned (0 when no prompt) */
    unsigned prompted;
    unsigned cleared_6ddf;        /* block B, key 'P': [0x6DDF] := 0 */
    uint8_t move_flag;            /* [bp-1] at the commit */
    unsigned committed_move;      /* +0x073B was called */
    unsigned calls_n;
    pr_u68_e1_call calls[48];     /* the calls in order (capped; calls_n counts all) */
} pr_u68_e1_report;

/* The entry. `report` may be NULL. */
pr_u68_e1_status pr_u68_entry1(pr_u68_e1_state *state, pr_u68_e1_report *report);

#endif
