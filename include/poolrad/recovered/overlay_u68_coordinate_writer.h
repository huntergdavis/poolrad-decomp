#ifndef POOLRAD_RECOVERED_OVERLAY_U68_COORDINATE_WRITER_H
#define POOLRAD_RECOVERED_OVERLAY_U68_COORDINATE_WRITER_H

/* Portable C model of DOS v1.3 GAME.OVR unit 0x68 internal routine
 * +0x073B..+0x07F3 (file 0x16E06, 185 bytes of code, 66 instructions, `retf`,
 * no arguments, no result). The 36 bytes +0x07F4..+0x0817 up to the next
 * prologue are a Pascal string constant that the routine at +0x0818 reaches
 * with `push cs`; they are data, not part of this routine (the 221-byte
 * ledger bound covers both). Near-called from entry 1 (+0x0D6D) at +0x102E
 * when its local movement flag [bp-1] is nonzero. Evidence:
 * docs/re/U68_COORDINATE_WRITER_073B.md.
 *
 * What the bytes do, in order:
 *   resident 0x8AE:0x00DC with the word [0x2628]                    (+0x073E..+0x0742)
 *   [0x6AAD] += byte [0x2736 + facing]   (8-bit add, wraps mod 256)  (+0x0747..+0x0752)
 *   [0x6AAE] += byte [0x273F + facing]                               (+0x0756..+0x0761)
 *   x: signed byte < 0 -> 15; then signed byte > 15 -> 0            (+0x0765..+0x0778)
 *   y: the same on [0x6AAE]                                          (+0x077D..+0x0790)
 *   [0x6AB0] = unit 0x3bd entry 2 (x, y, facing)  via 0x3D5:0x2A     (+0x0795..+0x07A6)
 *   FillChar(DS:0x6DDE, 3, 1)  via 0xAF8:0x16A8 (argument shape)     (+0x07A9..+0x07B5)
 *   [0x6AB1] = unit 0x3bd entry 4 (x, y)          via 0x3D5:0x34     (+0x07BA..+0x07C7)
 *   window-1 word [0x49D6]+0x594, bit 0 set -> unit 0x7d entry 1 (1, 2)
 *                                       clear -> unit 0x7d entry 1 (1, 1)   (+0x07CA..+0x07ED)
 * Unlike the Up preflight (overlay_u68_up_preflight.h), which never stores
 * the stepped position and only clamps, this routine stores it, and it
 * wraps (not clamps) the result: 16..127 -> 0, 128..255 -> 15.
 * The direction tables are host state: the live DS bytes are not
 * established (see the evidence note). The writers of [0x6AAF] keep it in
 * {0, 2, 4, 6}; other facings are modelled, not assumed unreachable.
 * Two far callees stay behind host hooks because they are not translated
 * here: resident 0x8AE:0x00DC (unresolved) and unit 0x7d entry 1 (stub
 * 0x8C:0x25). Entries 2 and 4 are the merged translations. The runtime fill
 * is modelled as writing three 1 bytes (inferred from argument shape).
 * Classification: path-complete model of the 185 code bytes with the two
 * unmerged far calls as explicit hooks. No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u3bd_cell.h"
#include "poolrad/recovered/overlay_u3bd_nibble.h"

#define PR_U68_STEP_WINDOW1_BIT_OFFSET 0x594u   /* es:[di + 0x594], word, tested with `and ax, 1` */
#define PR_U68_STEP_WINDOW1_MIN_SIZE 0x596u     /* the word read touches bytes 0x594 and 0x595 */
#define PR_U68_STEP_DX_TABLE_DS 0x2736u         /* `mov al, [di + 0x2736]`, di = facing */
#define PR_U68_STEP_DY_TABLE_DS 0x273Fu         /* `mov al, [di + 0x273F]`, di = facing */
#define PR_U68_STEP_DY_SKEW 9u                  /* 0x273F - 0x2736 */
#define PR_U68_STEP_GRID_MAX 15                 /* +0x0771/+0x0789: cmp byte, 0xF */
#define PR_U68_STEP_FILL_DS 0x6DDEu             /* +0x07A9: mov di, 0x6DDE */
#define PR_U68_STEP_FILL_COUNT 3u
#define PR_U68_STEP_FILL_VALUE 1u

typedef void (*pr_u68_step_resident_hook)(void *ctx, uint16_t word_2628);
typedef void (*pr_u68_step_u7d_hook)(void *ctx, uint16_t first, uint16_t second);

typedef struct {
    pr_u68_step_resident_hook resident_8ae_00dc;   /* lcall 0x8AE:0x00DC, one word argument; may be NULL */
    pr_u68_step_u7d_hook u7d_entry1;               /* lcall 0x8C:0x25 = unit 0x7d entry 1, two words; may be NULL */
    void *ctx;
} pr_u68_step_hooks;

typedef struct {
    uint8_t party_x;                 /* [0x6AAD], rewritten */
    uint8_t party_y;                 /* [0x6AAE], rewritten */
    uint8_t facing;                  /* [0x6AAF], zero-extended index 0..255 (read only) */
    uint8_t lookup_b0;               /* [0x6AB0], written with entry 2's AL */
    uint8_t lookup_b1;               /* [0x6AB1], written with entry 4's AL */
    uint8_t ds_6dde[PR_U68_STEP_FILL_COUNT];   /* DS:0x6DDE..0x6DE0, filled with 1 */
    uint16_t word_2628;              /* word [0x2628], pushed to the resident call */
    uint8_t *window1;                /* host memory for far [0x49D6]; only the word at 0x594 is read */
    size_t window1_size;
    const uint8_t *direction_table;  /* host bytes starting at DS:0x2736; read at [facing] and [facing + 9] */
    size_t direction_table_size;
    uint8_t map_id;                  /* byte [0x84DC] used by entries 2 and 4 */
    const uint8_t *buffer;           /* host memory for far [0x6A5C], 1,024 bytes */
    uint8_t entry2_frame_residue;    /* entry 2's unwritten [bp-1] (returned for a selector other than 0/2/4/6) */
    const pr_u68_step_hooks *hooks;  /* may be NULL */
} pr_u68_step_state;

typedef enum {
    PR_U68_STEP_OK = 0,
    PR_U68_STEP_ARGUMENT   /* null state; window1 shorter than 0x596; null buffer; or direction_table
                              shorter than facing + 10. All checked before any effect: on ARGUMENT
                              nothing is written and no hook runs. */
} pr_u68_step_status;

typedef enum {
    PR_U68_STEP_CALL_RESIDENT = 1,   /* +0x0742 */
    PR_U68_STEP_CALL_ENTRY2,         /* +0x07A1 */
    PR_U68_STEP_CALL_FILL,           /* +0x07B5 */
    PR_U68_STEP_CALL_ENTRY4,         /* +0x07C2 */
    PR_U68_STEP_CALL_U7D_ENTRY1      /* +0x07E0 or +0x07ED */
} pr_u68_step_call;

typedef struct {
    uint8_t x_in, y_in;              /* position bytes before the step */
    int dx, dy;                      /* sign-extended table bytes, for reporting (the add is 8-bit) */
    uint8_t x_sum, y_sum;            /* after the 8-bit adds, before the wrap */
    unsigned wrapped_x_low, wrapped_x_high;   /* signed byte < 0 -> 15; then signed byte > 15 -> 0 */
    unsigned wrapped_y_low, wrapped_y_high;
    uint8_t x_out, y_out;            /* the stored position */
    unsigned window_bit;             /* es:[di + 0x594] & 1 */
    uint16_t u7d_first, u7d_second;  /* the two pushed words (1, then 2 or 1) */
    unsigned call_count;
    pr_u68_step_call calls[5];       /* far calls in the order the routine makes them */
    pr_u3bd_nibble_report entry2;
    pr_u3bd_cell_report entry4;
} pr_u68_step_report;

/* Runs the routine on `state` in place. `report` may be NULL. */
pr_u68_step_status pr_u68_step_coordinate_writer(pr_u68_step_state *state, pr_u68_step_report *report);

#endif
