#ifndef POOLRAD_RECOVERED_OVERLAY_U68_UP_PREFLIGHT_H
#define POOLRAD_RECOVERED_OVERLAY_U68_UP_PREFLIGHT_H

/* Portable C model of DOS v1.3 GAME.OVR unit 0x68 internal routine
 * +0x068D..+0x073A (file 0x16D58, 174 bytes, 55 instructions, `retf`, no
 * arguments, no result read by its caller): the Up preflight that entry 2
 * (+0x0989) reaches with `push cs; call +0x068D` at +0x0AB2 when its prompt
 * returned an extended key with scan code 0x48. Evidence:
 * docs/re/U68_UP_PREFLIGHT_068D.md.
 *
 * What the bytes do, in order:
 *   x = cbw([0x6AAD]); y = cbw([0x6AAE]); f = zero-extended [0x6AAF]   (+0x0693..+0x06A6)
 *   window-1 word [0x49D6]+0x5AA = 0                                      (+0x06A9..+0x06AF)
 *   AL = unit 0x3bd entry 3 (x byte, y byte, f byte) via stub 0x3D5:0x2F   (+0x06B4..+0x06C0)
 *   AL == 0 -> return with nothing else written                           (+0x06C5..+0x06C7)
 *   x += sign-extended DS byte [0x2736 + f]; y += DS byte [0x273F + f]    (+0x06C9..+0x06DC)
 *   x > 15 -> [0x6AAD] = 15, flag word = 1;  x < 0 -> [0x6AAD] = 0, flag = 1
 *   y > 15 -> [0x6AAE] = 15, flag word = 1;  y < 0 -> [0x6AAE] = 0, flag = 1
 *   (four independent signed compares on the candidate words, +0x06DF..+0x0730)
 * The candidate position is never stored when it stays inside 0..15: the
 * routine only clamps an out-of-range candidate to the edge and raises the
 * window-1 word 0x5AA. The nested entry 3 is the merged translation
 * (overlay_u3bd_field.c); its hidden frame byte is passed through and is
 * proven not to affect its result. The two DS direction tables are host
 * state (`direction_table` holds the bytes from DS:0x2736 onward; dy sits
 * 9 bytes after dx); their live contents are not established here.
 * Classification: path-complete model of the 174 bytes with the one far
 * callee merged. No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u3bd_field.h"

#define PR_U68_UP_FLAG_OFFSET 0x5AAu        /* window-1 word written at +0x06AF and the four clamps */
#define PR_U68_UP_WINDOW1_MIN_SIZE 0x5ACu   /* bytes of host window 1 the model touches */
#define PR_U68_UP_DX_TABLE_DS 0x2736u       /* `mov al, [di + 0x2736]`, di = facing */
#define PR_U68_UP_DY_TABLE_DS 0x273Fu       /* `mov al, [di + 0x273F]`, di = facing */
#define PR_U68_UP_DY_SKEW 9u                /* 0x273F - 0x2736 */
#define PR_U68_UP_GRID_MAX 15               /* +0x06DF / +0x070B: cmp word, 0xF */

typedef struct {
    uint8_t party_x;                 /* [0x6AAD], read signed, rewritten only on a clamp */
    uint8_t party_y;                 /* [0x6AAE], likewise */
    uint8_t facing;                  /* [0x6AAF], zero-extended index 0..255 */
    uint8_t *window1;                /* host memory for far [0x49D6]; only bytes 0x5AA..0x5AB are written */
    size_t window1_size;
    const uint8_t *direction_table;  /* host bytes starting at DS:0x2736; read at [facing] and [facing + 9] */
    size_t direction_table_size;
    pr_u3bd_field_state field;       /* the merged entry 3's inputs: map id, 1,024-byte [0x6A5C] buffer, residue */
} pr_u68_up_state;

typedef enum {
    PR_U68_UP_OK = 0,
    PR_U68_UP_ARGUMENT               /* null state; window1 shorter than 0x5AC (checked on entry, nothing
                                        written); entry 3 reporting ARGUMENT (null buffer at its read; the
                                        flag word is already 0 by then); or direction_table shorter than
                                        facing + 10 when the table is reached (flag 0, lookup done, no
                                        position write) */
} pr_u68_up_status;

typedef struct {
    int x_in, y_in;                  /* cbw of the two position bytes */
    unsigned facing;                 /* the zero-extended facing byte */
    uint8_t lookup;                  /* AL returned by entry 3 */
    unsigned skipped;                /* lookup == 0: returned at +0x06C7 */
    int dx, dy;                      /* sign-extended table bytes (valid when !skipped) */
    int x_candidate, y_candidate;    /* x_in + dx, y_in + dy */
    unsigned clamped_x_high, clamped_x_low, clamped_y_high, clamped_y_low;
    unsigned flag_set;               /* window-1 word 0x5AA == 1 at exit */
    pr_u3bd_field_report field;      /* entry 3's own report */
} pr_u68_up_report;

/* Runs the routine on `state` in place. `report` may be NULL. */
pr_u68_up_status pr_u68_up_preflight(pr_u68_up_state *state, pr_u68_up_report *report);

#endif
