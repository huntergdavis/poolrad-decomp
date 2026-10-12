/* Bounded C model of GAME.OVR unit 0x68 +0x0818; see
 * docs/re/U68_TILE_STEP_0818.md. */
#include "poolrad/recovered/overlay_u68_tile_step_0818.h"

#include <string.h>

static int sbyte(uint8_t b)
{
    return b < 0x80u ? (int)b : (int)b - 256;
}

int pr_u68_step0818_tile_blocked(uint8_t t)
{
    /* +0x08BB..+0x08F5: the compare chain that jumps to +0x0976 */
    return t == 0x4Eu || (t >= 0x7Eu && t <= 0x80u) || (t >= 0x82u && t <= 0x84u) ||
           (t >= 0xA8u && t <= 0xACu) || (t >= 0xAEu && t <= 0xB3u) || t == 0xBCu ||
           (t >= 0xDDu && t <= 0xDEu) || (t >= 0xE1u && t <= 0xE9u) || t == 0xF5u;
}

int pr_u68_step0818_tile_special(uint8_t t)
{
    /* +0x08F9..+0x0903: 0x66..0x6C to +0x0905, 0x72 falls to +0x0905, the rest to +0x0951 */
    return (t >= 0x66u && t <= 0x6Cu) || t == 0x72u;
}

static uint16_t word_at(const uint8_t *p, uint16_t off)
{
    return (uint16_t)(p[off] | (p[off + 1u] << 8));
}

pr_u68_step0818_status pr_u68_step0818(pr_u68_step0818_state *st, pr_u68_step0818_report *report)
{
    pr_u68_step0818_report local;
    pr_u68_step0818_report *r = report ? report : &local;
    uint8_t x, y, col, tile, sel_off;
    int xs, ys;
    uint16_t rel, addr, sel_addr;
    memset(r, 0, sizeof *r);
    if (!st || !st->window0 || st->window0_size < PR_U68_STEP_WINDOW0_MIN_SIZE || !st->direction_table ||
        st->direction_table_size < (size_t)st->facing + 10u || !st->ds)
        return PR_U68_STEP_ARGUMENT;
    if (st->message && st->message[0] > PR_U68_STEP_MESSAGE_MAX)
        return PR_U68_STEP_ARGUMENT;

    /* +0x081E..+0x0861: the 32-bit add keeps only AL */
    x = (uint8_t)(word_at(st->window0, PR_U68_STEP_X_WORD) + (uint16_t)sbyte(st->direction_table[st->facing]));
    y = (uint8_t)(word_at(st->window0, PR_U68_STEP_Y_WORD) +
                  (uint16_t)sbyte(st->direction_table[(size_t)st->facing + 9u]));
    r->x_raw = x;
    r->y_raw = y;
    /* +0x0864..+0x0888: signed byte compares, low then high */
    xs = sbyte(x);
    if (xs < 0) xs = 0;
    if (xs > PR_U68_STEP_X_MAX) xs = PR_U68_STEP_X_MAX;
    ys = sbyte(y);
    if (ys < 0) ys = 0;
    if (ys > PR_U68_STEP_Y_MAX) ys = PR_U68_STEP_Y_MAX;
    x = (uint8_t)xs;
    y = (uint8_t)ys;
    r->x = x;
    r->y = y;

    /* +0x088C..+0x08B7: DS[0x3C76 + selector]; column byte; DS[0x3648 + (y * 0x2C + sext(column)) mod 65536] */
    sel_addr = (uint16_t)(PR_U68_STEP_COLUMN_TABLE_DS + st->selector);
    if ((size_t)sel_addr >= st->ds_size)
        return PR_U68_STEP_ARGUMENT;
    sel_off = st->ds[sel_addr];
    col = (uint8_t)(x + sel_off);
    rel = (uint16_t)((uint16_t)y * PR_U68_STEP_ROW_STRIDE + (uint16_t)sbyte(col));
    addr = (uint16_t)(rel + PR_U68_STEP_TILE_BASE_DS);
    if ((size_t)addr >= st->ds_size)
        return PR_U68_STEP_ARGUMENT;
    tile = st->ds[addr];
    r->column = col;
    r->tile = tile;
    r->tile_address = addr;

    if (pr_u68_step0818_tile_blocked(tile)) {                 /* +0x08F7: jmp +0x0976 */
        r->blocked = 1;
        return PR_U68_STEP_OK;
    }
    if (pr_u68_step0818_tile_special(tile)) {                 /* +0x0905 */
        r->special = 1;
        if (word_at(st->window0, PR_U68_STEP_FLAG_WORD) == 0u) {
            r->flag_word_zero = 1;                           /* +0x0911..+0x0928: message, no move */
            if (!st->message_hook)
                return PR_U68_STEP_HOOK;
            st->message_hook(st->ctx, st->message, PR_U68_STEP_MESSAGE_COLOR);
            return PR_U68_STEP_OK;
        }
    }
    /* +0x092A..+0x094A / +0x0951..+0x0971: both write the clamped bytes as words, then the clock call */
    if (!st->clock_hook)
        return PR_U68_STEP_HOOK;
    st->window0[PR_U68_STEP_X_WORD] = x;
    st->window0[PR_U68_STEP_X_WORD + 1u] = 0;                /* cbw of a byte in 0..15 */
    st->window0[PR_U68_STEP_Y_WORD] = y;
    st->window0[PR_U68_STEP_Y_WORD + 1u] = 0;                /* cbw of a byte in 0..0x23 */
    r->moved = 1;
    st->clock_hook(st->ctx, PR_U68_STEP_CLOCK_N, PR_U68_STEP_CLOCK_IDX);
    return PR_U68_STEP_OK;
}
