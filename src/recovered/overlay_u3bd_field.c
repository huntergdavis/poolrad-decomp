/* Portable C translation of GAME.OVR unit 0x3bd entry 3 (+0x0350); see
 * docs/re/U3BD_FIELD_ENTRY3.md. Every instruction path is reproduced in
 * the original's order; the nested call to entry 2 goes through the merged
 * translation in overlay_u3bd_nibble.c. */
#include "poolrad/recovered/overlay_u3bd_field.h"
#include "poolrad/recovered/overlay_u3bd_nibble.h"

#include <string.h>

/* Byte argument as the original's signed compares and cbw see it. */
static int pr_u3bd_field_signed_byte(uint8_t b)
{
    return b < 0x80u ? (int)b : (int)b - 256;
}

/* +0x031F..+0x034D: the unit's local leaf (49 bytes, 18 instructions, `retf 4`),
 * reached from +0x035E by `push cs; call`. Same translation as entries 2 and 4;
 * static so no merged file changes and the leaf is not counted again. */
static uint8_t pr_u3bd_field_in_grid(int a, int b)
{
    if (a > 15 || a < 0 || b > 15 || b < 0) return 0;
    return 1;
}

pr_u3bd_field_status pr_u3bd_entry3(const pr_u3bd_field_state *state, uint8_t x, uint8_t y,
                                    uint8_t selector, uint8_t *result,
                                    pr_u3bd_field_report *report)
{
    pr_u3bd_field_report local;
    pr_u3bd_field_report *r = report ? report : &local;
    pr_u3bd_nibble_state inner;
    pr_u3bd_nibble_report inner_report;
    pr_u3bd_nibble_status inner_status;
    uint8_t nibble = 0;
    uint8_t value;
    int sx, sy;
    unsigned mask;
    memset(r, 0, sizeof *r);
    if (!state || !result) return PR_U3BD_FIELD_ARGUMENT;
    sx = pr_u3bd_field_signed_byte(x);
    sy = pr_u3bd_field_signed_byte(y);

    /* +0x0356..+0x0364: push x byte, push y byte, push cs, call +0x031F; or al,al; jne +0x037B */
    r->in_range = pr_u3bd_field_in_grid(sx, sy);
    if (!r->in_range) {
        /* +0x0366..+0x0372: [0x84DC] == 0 -> +0x0374; [0x84DC] != 10 -> +0x037B; == 10 -> +0x0374 */
        if (state->map_id == PR_U3BD_FIELD_NO_WRAP_MAP_A || state->map_id == PR_U3BD_FIELD_NO_WRAP_MAP_B) {
            /* +0x0374..+0x0378: [bp-1] = 0; jmp +0x0472 (AL = 0, retf 6) */
            *result = 0;
            r->returned_zero = 1;
            return PR_U3BD_FIELD_OK;
        }
    }
    /* +0x037B..+0x039F: signed byte compares on [bp+0xA]/[bp+8] in this order:
     * x > 15 -> x = 0; x < 0 -> x = 15; y > 15 -> y = 0; y < 0 -> y = 15. */
    if (sx > 15) { sx = 0;  r->wrapped_x = 1; }
    if (sx < 0)  { sx = 15; r->wrapped_x = 1; }
    if (sy > 15) { sy = 0;  r->wrapped_y = 1; }
    if (sy < 0)  { sy = 15; r->wrapped_y = 1; }
    r->x_used = (uint8_t)sx;
    r->y_used = (uint8_t)sy;
    /* +0x03A3: [bp-2] = 1 */
    value = PR_U3BD_FIELD_DEFAULT;
    /* +0x03A7..+0x03B4: push x, push y, push selector, push cs, call +0x047B (entry 2). The
     * wrapped coordinates are in range, so entry 2 takes its read path for selectors 0, 2,
     * 4, 6 and its unwritten-[bp-1] path otherwise; that byte is this frame's [bp-0xF]. */
    inner.map_id = state->map_id;
    inner.buffer = state->buffer;
    inner.frame_residue = state->entry2_frame_residue;
    inner_status = pr_u3bd_entry2(&inner, (uint8_t)sx, (uint8_t)sy, selector, &nibble, &inner_report);
    if (inner_status == PR_U3BD_NIBBLE_ARGUMENT) return PR_U3BD_FIELD_ARGUMENT;   /* null buffer at entry 2's read */
    r->nibble = nibble;
    r->nibble_from_residue = (inner_status == PR_U3BD_NIBBLE_UNSPECIFIED);
    /* +0x03B7..+0x03BB: or al,al; ja +0x03BE (AL != 0); else jmp +0x046C */
    if (nibble != 0) {
        /* +0x03BE..+0x044A: al = selector; cmp 6 / 4 / 2 / 0 in that order; no match -> +0x046C */
        switch (selector) {
        case 6:  mask = 0xC0u; r->shift = 6; break;   /* +0x03C5..+0x03EC: es:[di+0x300] and 0xC0, shr 6 */
        case 4:  mask = 0x30u; r->shift = 4; break;   /* +0x03F2..+0x0419: and 0x30, shr 4 */
        case 2:  mask = 0x0Cu; r->shift = 2; break;   /* +0x041F..+0x0446: and 0x0C, shr 2 */
        case 0:  mask = 0x03u; r->shift = 0; break;   /* +0x044C..+0x0469: and 0x03 */
        default: mask = 0; break;
        }
        if (mask) {
            /* dx = cbw(x); ax = cbw(y) shl 4; les di,[0x6A5C]; di += ax + dx; al = es:[di+0x300] */
            r->offset = PR_U3BD_FIELD_QUARTER_OFFSET + PR_U3BD_FIELD_GRID * (unsigned)sy + (unsigned)sx;
            if (!state->buffer) return PR_U3BD_FIELD_ARGUMENT;
            r->read = 1;
            value = (uint8_t)((state->buffer[r->offset] & mask) >> r->shift);   /* [bp-2] = al */
        }
    }
    /* +0x046C..+0x0478: [bp-1] = [bp-2]; AL = [bp-1]; epilogue; retf 6 */
    r->defaulted = !r->read;
    *result = value;
    return PR_U3BD_FIELD_OK;
}
