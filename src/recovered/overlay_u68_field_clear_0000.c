/* Portable C translation of GAME.OVR unit 0x68 +0x0000; see
 * docs/re/U68_FIELD_CLEAR_0000.md. */
#include "poolrad/recovered/overlay_u68_field_clear_0000.h"

#include <string.h>

static uint16_t pr_u68_0000_cbw(uint8_t b)
{
    return (uint16_t)(b < 0x80u ? b : 0xFF00u | b);
}

pr_u68_0000_status pr_u68_0000(pr_u68_0000_state *st, uint8_t x, uint8_t y, uint8_t selector,
                               pr_u68_0000_report *report)
{
    pr_u68_0000_report local;
    pr_u68_0000_report *r = report ? report : &local;
    uint8_t mask;
    uint16_t di, addr;
    memset(r, 0, sizeof *r);
    if (!st || !st->segment || st->segment_size > 0x10000u)
        return PR_U68_0000_ARGUMENT;
    /* +0x0006..+0x00C9: the compare chain 6, 4, 2, 0 on the selector byte */
    switch (selector) {
    case 6: mask = 0x3Fu; break;          /* +0x0025: and al, 0x3F */
    case 4: mask = 0xCFu; break;          /* +0x0066: and al, 0xCF */
    case 2: mask = 0xF3u; break;          /* +0x00A6: and al, 0xF3 */
    case 0: mask = 0xFCu; break;          /* +0x00E6: and al, 0xFC */
    default: return PR_U68_0000_OK;       /* +0x00C9: jne +0x0105 */
    }
    /* +0x000A..+0x0020: dx = cbw(x); ax = cbw(y) << 4 (16-bit); di = [0x6A5C] + ax + dx; read es:[di + 0x300] */
    di = (uint16_t)(st->offset + (uint16_t)(pr_u68_0000_cbw(y) << PR_U68_0000_ROW_SHIFT) + pr_u68_0000_cbw(x));
    addr = (uint16_t)(di + PR_U68_0000_QUARTER);
    if ((size_t)addr >= st->segment_size)
        return PR_U68_0000_ARGUMENT;
    r->touched = 1;
    r->address = addr;
    r->before = st->segment[addr];
    /* +0x0025..+0x003F: bl = byte & mask; the address is recomputed identically; es:[di + 0x300] = bl */
    st->segment[addr] = (uint8_t)(r->before & mask);
    r->after = st->segment[addr];
    return PR_U68_0000_OK;
}
