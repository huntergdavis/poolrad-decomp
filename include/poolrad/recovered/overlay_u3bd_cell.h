#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_CELL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_CELL_H

/* Portable C translation of DOS v1.3 GAME.OVR unit 0x3bd public entry 4
 * (+0x057A, file 0x2EE71, run-time stub 0x3D5:0x0034): the 16 x 16 cell
 * lookup in the third 256-byte quarter of the 1,024-byte buffer at far
 * [0x6A5C], the buffer entry 7 fills (U3BD_WALLSET_ENTRY7.md). Two pushed
 * words, each read as a signed byte: [bp+8] = x (pushed first), [bp+6] = y;
 * result in AL; `retf 4`. All five fixup-confirmed callers push the party
 * x/y bytes [0x6AAD]/[0x6AAE] (GAMEPLAY_PATH.md) and store AL in [0x6AB1].
 *
 * Classification: full translation (FUNCTION_LEDGER.md vocabulary). The
 * routine makes no far calls; its one near call (`push cs; call +0x031F`)
 * reaches the unit's local leaf range test, translated here as a static
 * helper. Its two data-segment reads, byte [0x84DC] (the map id) and far
 * pointer [0x6A5C], are explicit state. Evidence: docs/re/U3BD_CELL_ENTRY4.md.
 * "cell" names the byte at (x, y); no game role beyond the documented
 * party-position and map-id globals is asserted. No game data is embedded. */

#include <stdint.h>

#define PR_U3BD_CELL_BUFFER_SIZE 0x400u      /* far [0x6A5C] holds 1,024 bytes (entry 7) */
#define PR_U3BD_CELL_QUARTER_OFFSET 0x200u   /* +0x05E2: es:[di + 0x200] */
#define PR_U3BD_CELL_GRID 16u                /* row stride (shl 4) and the 0..15 range */
#define PR_U3BD_CELL_NO_WRAP_MAP_A 0u        /* +0x0590: [0x84DC] == 0 returns 0 */
#define PR_U3BD_CELL_NO_WRAP_MAP_B 10u       /* +0x0597: [0x84DC] == 10 returns 0 */

typedef struct {
    uint8_t map_id;              /* byte [0x84DC] */
    const uint8_t *buffer;       /* host memory for far [0x6A5C]; only bytes 0x200..0x2FF are read */
} pr_u3bd_cell_state;

typedef enum {
    PR_U3BD_CELL_OK = 0,
    PR_U3BD_CELL_ARGUMENT        /* null state or result (checked on entry), or a null buffer
                                    when the read at +0x05DA is reached (the original first
                                    touches [0x6A5C] there; the zero path never does) */
} pr_u3bd_cell_status;

typedef struct {
    unsigned in_range;           /* the leaf +0x031F returned 1: both bytes in 0..15 (signed) */
    unsigned returned_zero;      /* +0x059E: out of range on map 0 or 10, result 0, no read */
    unsigned wrapped_x;          /* +0x05A4..+0x05B4 changed x (> 15 -> 0, < 0 -> 15) */
    unsigned wrapped_y;          /* +0x05B8..+0x05C8 changed y */
    uint8_t x_used, y_used;      /* the bytes after the wrap, 0..15 */
    unsigned offset;             /* buffer offset read: 0x200 + 16 * y_used + x_used */
} pr_u3bd_cell_report;

/* The entry. `x` = [bp+8], `y` = [bp+6], raw bytes interpreted as signed
 * (the original compares with jg/jl/jle/jge and extends with cbw). On
 * PR_U3BD_CELL_OK, *result is AL. */
pr_u3bd_cell_status pr_u3bd_entry4(const pr_u3bd_cell_state *state, uint8_t x, uint8_t y,
                                   uint8_t *result, pr_u3bd_cell_report *report);

#endif
