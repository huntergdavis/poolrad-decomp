#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_FIELD_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_FIELD_H

/* Portable C translation of DOS v1.3 GAME.OVR unit 0x3bd public entry 3
 * (+0x0350, file 0x2EC47, run-time stub 0x3D5:0x002F): a 2-bit field
 * lookup in the fourth 256-byte quarter of the 1,024-byte buffer at far
 * [0x6A5C] that entry 7 fills, gated by the nibble entry 2 returns for the
 * same cell and selector. Three pushed words, each read as a byte:
 * [bp+0xA] = x (pushed first), [bp+8] = y, [bp+6] = selector; result in
 * AL; `retf 6`. Four fixup-confirmed callers; three push the party x/y
 * bytes [0x6AAD]/[0x6AAE] and the facing byte [0x6AAF].
 *
 * Paths, in the original's order:
 *   range gate (local leaf +0x031F, not counted) and the map-id gate on
 *   [0x84DC]: out of range on map 0 or 10 returns 0 at once;
 *   wrap (> 15 -> 0, < 0 -> 15) on each coordinate;
 *   [bp-2] = 1 (the default result);
 *   `push cs; call +0x047B`: entry 2 (overlay_u3bd_nibble.c, merged) with
 *   the wrapped coordinates and the selector; AL == 0 -> result 1;
 *   otherwise selector 6 / 4 / 2 / 0 -> bits 7..6 / 5..4 / 3..2 / 1..0 of
 *   buffer[0x300 + 16*y + x]; any other selector -> result 1.
 * Hidden stack byte: on a selector outside {0, 2, 4, 6} the nested entry 2
 * returns its unwritten [bp-1], which is this routine's [bp-0xF]; no
 * instruction of this routine, the leaf or entry 2 writes it first. It is
 * an explicit input here (`entry2_frame_residue`) so the call is
 * reproduced exactly, and the result is 1 whether it is zero (the `ja`
 * at +0x03B9 falls through) or not (the selector chain matches nothing),
 * so this routine's output never depends on it.
 * Classification: full translation; no far calls; the two data-segment
 * reads are explicit state. Evidence: docs/re/U3BD_FIELD_ENTRY3.md.
 * "field" is a positional name; no game role is asserted. No game data is
 * embedded. */

#include <stdint.h>

#define PR_U3BD_FIELD_BUFFER_SIZE 0x400u      /* far [0x6A5C] holds 1,024 bytes (entry 7) */
#define PR_U3BD_FIELD_QUARTER_OFFSET 0x300u   /* es:[di + 0x300] at the four reads */
#define PR_U3BD_FIELD_GRID 16u                /* row stride (shl 4) and the 0..15 range */
#define PR_U3BD_FIELD_NO_WRAP_MAP_A 0u        /* +0x0366: [0x84DC] == 0 returns 0 */
#define PR_U3BD_FIELD_NO_WRAP_MAP_B 10u       /* +0x036D: [0x84DC] == 10 returns 0 */
#define PR_U3BD_FIELD_DEFAULT 1u              /* +0x03A3: [bp-2] = 1 */

typedef struct {
    uint8_t map_id;                /* byte [0x84DC] */
    const uint8_t *buffer;         /* host memory for far [0x6A5C]; bytes 0x000..0x1FF (through
                                      entry 2) and 0x300..0x3FF are read */
    uint8_t entry2_frame_residue;  /* this routine's [bp-0xF] = the nested entry 2's [bp-1] after
                                      its `sub sp,2`: stack residue from before this routine was
                                      entered; read by entry 2 on a selector outside {0,2,4,6}.
                                      Proven not to change this routine's result. */
} pr_u3bd_field_state;

typedef enum {
    PR_U3BD_FIELD_OK = 0,
    PR_U3BD_FIELD_ARGUMENT         /* null state or result (checked on entry), or a null buffer
                                      when a read is reached (inside entry 2 on a recognised
                                      selector, or at +0x03DB/+0x0408/+0x0435/+0x0462) */
} pr_u3bd_field_status;

typedef struct {
    unsigned in_range;             /* the leaf +0x031F returned 1: both bytes in 0..15 (signed) */
    unsigned returned_zero;        /* +0x0374: out of range on map 0 or 10, result 0, nothing else */
    unsigned wrapped_x, wrapped_y; /* +0x037B..+0x039F changed a coordinate */
    uint8_t x_used, y_used;        /* the bytes after the wrap, 0..15 */
    uint8_t nibble;                /* AL returned by entry 2 at +0x03B7 */
    unsigned nibble_from_residue;  /* entry 2 returned entry2_frame_residue (unmatched selector) */
    unsigned read;                 /* 1 when the quarter-3 byte was read */
    unsigned offset;               /* 0x300 + 16 * y_used + x_used when read */
    unsigned shift;                /* 6, 4, 2 or 0 when read */
    unsigned defaulted;            /* 1 when the result is the +0x03A3 default (nibble 0 or no match) */
} pr_u3bd_field_report;

/* The entry. `x` = [bp+0xA], `y` = [bp+8], `selector` = [bp+6], raw bytes
 * (x and y interpreted as signed, as the original compares and cbw do).
 * On PR_U3BD_FIELD_OK, *result is AL: 0 on the map gate, else 0..3 or 1. */
pr_u3bd_field_status pr_u3bd_entry3(const pr_u3bd_field_state *state, uint8_t x, uint8_t y,
                                    uint8_t selector, uint8_t *result,
                                    pr_u3bd_field_report *report);

#endif
