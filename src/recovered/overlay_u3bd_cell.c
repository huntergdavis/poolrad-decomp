/* Portable C translation of GAME.OVR unit 0x3bd entry 4 (+0x057A) and the
 * local leaf it near-calls (+0x031F); see docs/re/U3BD_CELL_ENTRY4.md.
 * Every instruction path is reproduced in the original's order; the two
 * data-segment reads are explicit state. */
#include "poolrad/recovered/overlay_u3bd_cell.h"

#include <string.h>

/* Byte argument as the original's signed compares and cbw see it. */
static int pr_u3bd_signed_byte(uint8_t b)
{
    return b < 0x80u ? (int)b : (int)b - 256;
}

/* +0x031F..+0x034D: local leaf, 49 bytes, 18 instructions, `retf 4`, reached by
 * `push cs; call` from entries 2, 3, 4 and 5. [bp+8] = a, [bp+6] = b, both signed
 * bytes. +0x0325 cmp a,15 jg; +0x032B cmp a,0 jl; +0x0331 cmp b,15 jg;
 * +0x0337 cmp b,0 jge -> +0x0343 result 1; otherwise +0x033D result 0. */
static uint8_t pr_u3bd_in_grid(int a, int b)
{
    if (a > 15 || a < 0 || b > 15 || b < 0) return 0;
    return 1;
}

pr_u3bd_cell_status pr_u3bd_entry4(const pr_u3bd_cell_state *state, uint8_t x, uint8_t y,
                                   uint8_t *result, pr_u3bd_cell_report *report)
{
    pr_u3bd_cell_report local;
    pr_u3bd_cell_report *r = report ? report : &local;
    int sx, sy;
    memset(r, 0, sizeof *r);
    if (!state || !result) return PR_U3BD_CELL_ARGUMENT;
    sx = pr_u3bd_signed_byte(x);
    sy = pr_u3bd_signed_byte(y);

    /* +0x0580..+0x058E: push x byte, push y byte, push cs, call +0x031F; or al,al; jne +0x05A4 */
    r->in_range = pr_u3bd_in_grid(sx, sy);
    if (!r->in_range) {
        /* +0x0590..+0x059C: [0x84DC] == 0 -> +0x059E; [0x84DC] != 10 -> +0x05A4; == 10 -> +0x059E */
        if (state->map_id == PR_U3BD_CELL_NO_WRAP_MAP_A || state->map_id == PR_U3BD_CELL_NO_WRAP_MAP_B) {
            /* +0x059E..+0x05A2: [bp-1] = 0; jmp +0x05EA (AL = 0, retf 4) */
            *result = 0;
            r->returned_zero = 1;
            return PR_U3BD_CELL_OK;
        }
    }
    /* +0x05A4..+0x05C8: in the original's order, each a signed byte compare on [bp+8]/[bp+6]:
     * x > 15 -> x = 0; x < 0 -> x = 15; y > 15 -> y = 0; y < 0 -> y = 15. */
    if (sx > 15) { sx = 0;  r->wrapped_x = 1; }
    if (sx < 0)  { sx = 15; r->wrapped_x = 1; }
    if (sy > 15) { sy = 0;  r->wrapped_y = 1; }
    if (sy < 0)  { sy = 15; r->wrapped_y = 1; }
    r->x_used = (uint8_t)sx;
    r->y_used = (uint8_t)sy;
    /* +0x05CC..+0x05E7: dx = cbw(x); ax = cbw(y) shl 4; les di,[0x6A5C]; di += ax + dx;
     * al = es:[di + 0x200]; [bp-1] = al. Both values are 0..15 here, so the offset is
     * 0x200..0x2FF, inside the 1,024-byte buffer. */
    r->offset = PR_U3BD_CELL_QUARTER_OFFSET + PR_U3BD_CELL_GRID * (unsigned)sy + (unsigned)sx;
    if (!state->buffer) return PR_U3BD_CELL_ARGUMENT;
    /* +0x05EA..+0x05F0: AL = [bp-1]; epilogue; retf 4 */
    *result = state->buffer[r->offset];
    return PR_U3BD_CELL_OK;
}
