/* Portable C translation of GAME.OVR unit 0x3bd entry 2 (+0x047B); see
 * docs/re/U3BD_NIBBLE_ENTRY2.md. Every instruction path is reproduced in
 * the original's order; the two data-segment reads are explicit state. */
#include "poolrad/recovered/overlay_u3bd_nibble.h"

#include <string.h>

/* Byte argument as the original's signed compares and cbw see it. */
static int pr_u3bd_nibble_signed_byte(uint8_t b)
{
    return b < 0x80u ? (int)b : (int)b - 256;
}

/* +0x031F..+0x034D: the unit's local leaf (49 bytes, 18 instructions, `retf 4`),
 * reached from +0x0489 by `push cs; call`. Same translation as in
 * overlay_u3bd_cell.c (entry 4); kept static here so no merged file changes and
 * the leaf is not counted twice. 1 iff 0 <= a <= 15 and 0 <= b <= 15, signed. */
static uint8_t pr_u3bd_nibble_in_grid(int a, int b)
{
    if (a > 15 || a < 0 || b > 15 || b < 0) return 0;
    return 1;
}

pr_u3bd_nibble_status pr_u3bd_entry2(const pr_u3bd_nibble_state *state, uint8_t x, uint8_t y,
                                     uint8_t selector, uint8_t *result,
                                     pr_u3bd_nibble_report *report)
{
    pr_u3bd_nibble_report local;
    pr_u3bd_nibble_report *r = report ? report : &local;
    int sx, sy;
    unsigned base;
    memset(r, 0, sizeof *r);
    if (!state || !result) return PR_U3BD_NIBBLE_ARGUMENT;
    sx = pr_u3bd_nibble_signed_byte(x);
    sy = pr_u3bd_nibble_signed_byte(y);

    /* +0x0481..+0x048F: push x byte, push y byte, push cs, call +0x031F; or al,al; jne +0x04A6 */
    r->in_range = pr_u3bd_nibble_in_grid(sx, sy);
    if (!r->in_range) {
        /* +0x0491..+0x049D: [0x84DC] == 0 -> +0x049F; [0x84DC] != 10 -> +0x04A6; == 10 -> +0x049F */
        if (state->map_id == PR_U3BD_NIBBLE_NO_WRAP_MAP_A || state->map_id == PR_U3BD_NIBBLE_NO_WRAP_MAP_B) {
            /* +0x049F..+0x04A3: [bp-1] = 0; jmp +0x0571 (AL = 0, retf 6) */
            *result = 0;
            r->returned_zero = 1;
            return PR_U3BD_NIBBLE_OK;
        }
    }
    /* +0x04A6..+0x04CA: signed byte compares on [bp+0xA]/[bp+8] in this order:
     * x > 15 -> x = 0; x < 0 -> x = 15; y > 15 -> y = 0; y < 0 -> y = 15. */
    if (sx > 15) { sx = 0;  r->wrapped_x = 1; }
    if (sx < 0)  { sx = 15; r->wrapped_x = 1; }
    if (sy > 15) { sy = 0;  r->wrapped_y = 1; }
    if (sy < 0)  { sy = 15; r->wrapped_y = 1; }
    r->x_used = (uint8_t)sx;
    r->y_used = (uint8_t)sy;
    /* +0x04CE..+0x04D3: al = [bp+6]; cmp al,0; jne +0x04FC; +0x04FC cmp al,2; jne +0x0520;
     * +0x0520 cmp al,4; jne +0x054D; +0x054D cmp al,6; jne +0x0571. Each case computes
     * dx = cbw(x); ax = cbw(y) shl 4; les di,[0x6A5C]; di += ax + dx and reads one byte. */
    switch (selector) {
    case 0:  base = 0;                      r->high_nibble = 1; break;   /* +0x04D5..+0x04FA: es:[di], and 0xF0, shr 4 */
    case 2:  base = 0;                      r->high_nibble = 0; break;   /* +0x0500..+0x051E: es:[di], and 0x0F */
    case 4:  base = PR_U3BD_NIBBLE_QUARTER; r->high_nibble = 1; break;   /* +0x0524..+0x054B: es:[di+0x100], and 0xF0, shr 4 */
    case 6:  base = PR_U3BD_NIBBLE_QUARTER; r->high_nibble = 0; break;   /* +0x0551..+0x056E: es:[di+0x100], and 0x0F */
    default:
        /* +0x054F jne +0x0571: no case matched; [bp-1] was never written (frame set up by
         * `sub sp,2` only), so AL is whatever byte that stack slot held. */
        return PR_U3BD_NIBBLE_UNSPECIFIED;
    }
    r->offset = base + PR_U3BD_NIBBLE_GRID * (unsigned)sy + (unsigned)sx;   /* 0x000..0x1FF */
    if (!state->buffer) return PR_U3BD_NIBBLE_ARGUMENT;
    r->read = 1;
    /* +0x0571..+0x0577: AL = [bp-1]; epilogue; retf 6 */
    *result = r->high_nibble ? (uint8_t)((state->buffer[r->offset] & 0xF0u) >> 4)
                             : (uint8_t)(state->buffer[r->offset] & 0x0Fu);
    return PR_U3BD_NIBBLE_OK;
}
