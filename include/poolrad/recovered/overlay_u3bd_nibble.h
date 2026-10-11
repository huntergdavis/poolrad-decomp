#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_NIBBLE_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_NIBBLE_H

/* Portable C translation of DOS v1.3 GAME.OVR unit 0x3bd public entry 2
 * (+0x047B, file 0x2ED72, run-time stub 0x3D5:0x002A): a nibble lookup in
 * the first two 256-byte quarters of the 1,024-byte buffer at far [0x6A5C]
 * that entry 7 fills (U3BD_WALLSET_ENTRY7.md). Three pushed words, each
 * read as a byte: [bp+0xA] = x (pushed first), [bp+8] = y, [bp+6] =
 * selector; result in AL; `retf 6`. Eight fixup-confirmed callers; the
 * direct ones push the party x/y bytes [0x6AAD]/[0x6AAE] and the facing
 * byte [0x6AAF] and store AL into [0x6AB0].
 *
 * The range gate (local leaf +0x031F, translated as a static helper and
 * not counted), the map-id gate on [0x84DC] and the 0..15 wrap are the
 * same instruction sequence as entry 4 (overlay_u3bd_cell.c). Then:
 *   selector 0: high nibble of buffer[16*y + x]
 *   selector 2: low  nibble of buffer[16*y + x]
 *   selector 4: high nibble of buffer[0x100 + 16*y + x]
 *   selector 6: low  nibble of buffer[0x100 + 16*y + x]
 *   any other selector: the result byte [bp-1] is never written, so the
 *   original returns a stale frame byte. Reported as UNSPECIFIED with
 *   *result untouched; every writer of [0x6AAF] in GAME.OVR keeps it in
 *   {0, 2, 4, 6}, see docs/re/U3BD_NIBBLE_ENTRY2.md.
 * Classification: full translation of every path; the stale-byte path's
 * value is not reproducible and is flagged. No far calls; the two
 * data-segment reads are explicit state. No game data is embedded. */

#include <stdint.h>

#define PR_U3BD_NIBBLE_BUFFER_SIZE 0x400u     /* far [0x6A5C] holds 1,024 bytes (entry 7) */
#define PR_U3BD_NIBBLE_QUARTER 0x100u         /* selectors 4 and 6 read es:[di + 0x100] */
#define PR_U3BD_NIBBLE_GRID 16u               /* row stride (shl 4) and the 0..15 range */
#define PR_U3BD_NIBBLE_NO_WRAP_MAP_A 0u       /* +0x0491: [0x84DC] == 0 returns 0 */
#define PR_U3BD_NIBBLE_NO_WRAP_MAP_B 10u      /* +0x0498: [0x84DC] == 10 returns 0 */

typedef struct {
    uint8_t map_id;              /* byte [0x84DC] */
    const uint8_t *buffer;       /* host memory for far [0x6A5C]; only bytes 0x000..0x1FF are read */
} pr_u3bd_nibble_state;

typedef enum {
    PR_U3BD_NIBBLE_OK = 0,
    PR_U3BD_NIBBLE_ARGUMENT,     /* null state or result (checked on entry), or a null buffer
                                    when one of the four reads is reached */
    PR_U3BD_NIBBLE_UNSPECIFIED   /* selector not 0, 2, 4 or 6 after the gates: the original
                                    returns an unwritten frame byte; *result is left untouched */
} pr_u3bd_nibble_status;

typedef struct {
    unsigned in_range;           /* the leaf +0x031F returned 1: both bytes in 0..15 (signed) */
    unsigned returned_zero;      /* +0x049F: out of range on map 0 or 10, result 0, no read */
    unsigned wrapped_x;          /* +0x04A6..+0x04B6 changed x (> 15 -> 0, < 0 -> 15) */
    unsigned wrapped_y;          /* +0x04BA..+0x04CA changed y */
    uint8_t x_used, y_used;      /* the bytes after the wrap, 0..15 */
    unsigned read;               /* 1 when one of the four reads happened */
    unsigned offset;             /* buffer offset read: quarter*0x100 + 16 * y_used + x_used */
    unsigned high_nibble;        /* 1 for selectors 0 and 4 (and 0xF0, shr 4), 0 for 2 and 6 (and 0x0F) */
} pr_u3bd_nibble_report;

/* The entry. `x` = [bp+0xA], `y` = [bp+8], `selector` = [bp+6], raw bytes
 * (x and y interpreted as signed, as the original compares and cbw do).
 * On PR_U3BD_NIBBLE_OK, *result is AL (0..15). */
pr_u3bd_nibble_status pr_u3bd_entry2(const pr_u3bd_nibble_state *state, uint8_t x, uint8_t y,
                                     uint8_t selector, uint8_t *result,
                                     pr_u3bd_nibble_report *report);

#endif
