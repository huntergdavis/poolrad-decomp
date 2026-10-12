#ifndef POOLRAD_RECOVERED_OVERLAY_U68_TILE_STEP_0818_H
#define POOLRAD_RECOVERED_OVERLAY_U68_TILE_STEP_0818_H

/* Bounded C model of DOS v1.3 GAME.OVR unit 0x68 internal routine
 * +0x0818..+0x0979 (file 0x16EE3, exactly 354 bytes of code, 144
 * instructions, `retf`, no arguments, no result). The 15 bytes
 * +0x097A..+0x0988 up to the next prologue (entry 2, +0x0989) are string
 * constants that entry 2 reaches with `cs:` offsets (not this routine); the
 * 369-byte ledger bound is 354 + 15. Near-called only by entry 1 at +0x1066
 * (selector [0x49FA] in 2..4 with window-1 word 0x592 below 0xFF).
 * Evidence: docs/re/U68_TILE_STEP_0818.md.
 *
 * Flow in the original's order:
 *   x := low byte of (window-0 word 0x186 + sext(DS[0x2736 + facing]))        (+0x081E..+0x083E)
 *   y := low byte of (window-0 word 0x188 + sext(DS[0x273F + facing]))        (+0x0841..+0x0861)
 *   x := clamp(x as signed byte, 0, 0x0F);  y := clamp(y as signed byte, 0, 0x23)   (+0x0864..+0x0888)
 *   col := low byte of (DS[0x3C76 + selector] + x);  tile := DS[0x3648 + 0x2C * y + sext(col)]
 *                                                                             (+0x088C..+0x08B7)
 *   tile in {0x4E, 0x7E..0x80, 0x82..0x84, 0xA8..0xAC, 0xAE..0xB3, 0xBC,
 *            0xDD..0xDE, 0xE1..0xE9, 0xF5}  ->  return, nothing else         (+0x08BB..+0x08F7)
 *   tile in {0x66..0x6C, 0x72} (a "special" tile)                            (+0x08F9..+0x0903):
 *       window-0 word 0x344 == 0 -> copy the Pascal string cs:0x07F4 to a
 *           local and call resident 0x709:0x0D68 with it and 0x0E; return    (+0x0905..+0x094F)
 *       otherwise fall to the move below                                     (+0x092A)
 *   any other tile -> the move (+0x0951..+0x0971): window-0 words 0x186 :=
 *       x and 0x188 := y (16-bit, from the clamped bytes), then unit 0x7d
 *       entry 1 (0x8C:0x25) with (0x0C, 0x03)
 * The 16-bit address arithmetic is reproduced: y * 0x2C + sext(col) wraps
 * modulo 65536, and so does the +0x3648 displacement. The tile table and the
 * selector offset table are DS bytes supplied as a host image.
 *
 * Classification: bounded model of the 354 bytes. NOT strict full C: the
 * resident message call 0x709:0x0D68 and unit 0x7d entry 1 (merged as
 * overlay_u7d_entry1.h, wired in by the host) are hooks, the string
 * constant is host data, and the DS image and window 0 are explicit host
 * state. No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#define PR_U68_0818_X_WORD 0x186u         /* window-0 words read and (on a move) written */
#define PR_U68_0818_Y_WORD 0x188u
#define PR_U68_0818_FLAG_WORD 0x344u      /* cmp word es:[di + 0x344], 0 */
#define PR_U68_0818_WINDOW0_MIN_SIZE 0x346u
#define PR_U68_0818_X_MAX 0x0F
#define PR_U68_0818_Y_MAX 0x23
#define PR_U68_0818_ROW_STRIDE 0x2Cu      /* mov dx, 0x2C; mul dx */
#define PR_U68_0818_TILE_BASE_DS 0x3648u  /* [di + 0x3648] */
#define PR_U68_0818_COLUMN_TABLE_DS 0x3C76u   /* [di + 0x3C76], di = selector */
#define PR_U68_0818_DX_TABLE_DS 0x2736u
#define PR_U68_0818_DY_TABLE_DS 0x273Fu
#define PR_U68_0818_MESSAGE_MAX 35u       /* the 36-byte local string slot [bp-0x27] */
#define PR_U68_0818_MESSAGE_COLOR 0x0Eu   /* mov al, 0xE before 0x709:0x0D68 */
#define PR_U68_0818_CLOCK_N 0x0Cu         /* pushed to 0x8C:0x25 first */
#define PR_U68_0818_CLOCK_IDX 0x03u       /* then 3 */

/* 0x709:0x0D68 (resident, unresolved): the local string copy and the byte 0x0E. */
typedef void (*pr_u68_0818_message_hook)(void *ctx, const uint8_t *pascal_string, uint8_t color);
/* 0x8C:0x25 = unit 0x7d entry 1, called with (n, idx) = (0x0C, 0x03). */
typedef void (*pr_u68_0818_clock_hook)(void *ctx, uint8_t n, uint8_t idx);

typedef struct {
    uint8_t facing;                   /* [0x6AAF] */
    uint8_t selector;                 /* [0x49FA] */
    uint8_t *window0;                 /* host memory for far [0x49D2]; words 0x186, 0x188, 0x344 */
    size_t window0_size;
    const uint8_t *direction_table;   /* DS bytes from 0x2736 (dy at +9); facing + 10 bytes needed */
    size_t direction_table_size;
    const uint8_t *ds;                /* host DS image from offset 0; reads at 0x3648.. and 0x3C76 + selector */
    size_t ds_size;
    const uint8_t *message;           /* the Pascal string cs:0x07F4 (host data). Read only on the reached
                                         special-tile, word 0x344 == 0 branch, where it must be non-NULL
                                         with at most 35 characters; ignored on every other path. */
    pr_u68_0818_message_hook message_hook;   /* may be NULL (reached -> PR_U68_0818_HOOK) */
    pr_u68_0818_clock_hook clock_hook;       /* may be NULL (reached -> PR_U68_0818_HOOK) */
    void *ctx;
} pr_u68_0818_state;

typedef enum {
    PR_U68_0818_OK = 0,
    PR_U68_0818_ARGUMENT,   /* null state, window 0 shorter than 0x346, short direction table, or a DS read
                               outside the image. Nothing written, no hook run. */
    PR_U68_0818_MESSAGE,    /* the message branch was reached (special tile, word 0x344 == 0) and `message`
                               is NULL or longer than 35 characters. Nothing written, no hook run. */
    PR_U68_0818_HOOK        /* the reached hook (message or clock) is NULL. Checked before any write. */
} pr_u68_0818_status;

typedef struct {
    uint8_t x_raw, y_raw;     /* the low bytes after the table steps */
    uint8_t x, y;             /* after the clamps */
    uint8_t column;           /* low byte of DS[0x3C76 + selector] + x */
    uint8_t tile;
    uint16_t tile_address;    /* DS offset read */
    unsigned blocked;         /* tile in the return set */
    unsigned special;         /* tile in {0x66..0x6C, 0x72} */
    unsigned flag_word_zero;  /* special and word 0x344 == 0: the message path */
    unsigned moved;           /* words 0x186/0x188 written and the clock hook called */
} pr_u68_0818_report;

/* 1 when `tile` is in the blocked set (+0x08BB..+0x08F7), 0 otherwise. */
int pr_u68_0818_tile_blocked(uint8_t tile);
/* 1 when `tile` is in {0x66..0x6C, 0x72}. */
int pr_u68_0818_tile_special(uint8_t tile);

/* The routine. `report` may be NULL. */
pr_u68_0818_status pr_u68_0818_run(pr_u68_0818_state *state, pr_u68_0818_report *report);

#endif
