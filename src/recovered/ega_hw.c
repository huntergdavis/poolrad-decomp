/* EGA planar-mode graphics pipeline; docs/re/EGA_HW_MODEL.md. */
#include "poolrad/recovered/ega_hw.h"

#include <string.h>

void pr_ega_reset(pr_ega *ega)
{
    memset(ega, 0, sizeof *ega);
    ega->seq[2] = 0x0Fu;
    ega->gc[7] = 0x0Fu;
    ega->gc[8] = 0xFFu;
}

static pr_ega_status out8(pr_ega *ega, uint16_t port, uint8_t value)
{
    switch (port) {
    case 0x3C4u:
        ega->seq_index = value;
        return PR_EGA_OK;
    case 0x3C5u:
        if (ega->seq_index >= sizeof ega->seq)
            return PR_EGA_IGNORED;
        ega->seq[ega->seq_index] = value;
        return PR_EGA_OK;
    case 0x3CEu:
        ega->gc_index = value;
        return PR_EGA_OK;
    case 0x3CFu:
        if (ega->gc_index >= sizeof ega->gc)
            return PR_EGA_IGNORED;
        ega->gc[ega->gc_index] = value;
        return PR_EGA_OK;
    default:
        return PR_EGA_IGNORED;
    }
}

pr_ega_status pr_ega_out(pr_ega *ega, uint16_t port, uint16_t value, unsigned size)
{
    pr_ega_status status = out8(ega, port, (uint8_t)value);
    if (size == 2u && status == PR_EGA_OK)
        status = out8(ega, (uint16_t)(port + 1u), (uint8_t)(value >> 8));
    return status;
}

uint8_t pr_ega_read(pr_ega *ega, uint16_t offset)
{
    unsigned p, bit;
    uint8_t result = 0;
    for (p = 0; p < 4u; ++p)
        ega->latch[p] = ega->plane[p][offset];
    if (!(ega->gc[5] & 0x08u))
        return ega->latch[ega->gc[4] & 3u];
    for (bit = 0; bit < 8u; ++bit) {
        unsigned match = 1u;
        for (p = 0; p < 4u; ++p)
            if ((ega->gc[7] >> p) & 1u)
                match &= (((unsigned)ega->latch[p] >> bit) & 1u) == ((ega->gc[2] >> p) & 1u);
        result = (uint8_t)(result | (match << bit));
    }
    return result;
}

pr_ega_status pr_ega_write(pr_ega *ega, uint16_t offset, uint8_t value)
{
    unsigned mode = ega->gc[5] & 3u, rotate = ega->gc[3] & 7u, function = (ega->gc[3] >> 3) & 3u, p;
    uint8_t rotated = (uint8_t)((value >> rotate) | (value << ((8u - rotate) & 7u)));
    if (mode == 3u)
        return PR_EGA_UNSUPPORTED;
    for (p = 0; p < 4u; ++p) {
        uint8_t data, mixed;
        if (!((ega->seq[2] >> p) & 1u))
            continue;
        if (mode == 1u) {
            ega->plane[p][offset] = ega->latch[p];
            continue;
        }
        if (mode == 2u)
            data = ((value >> p) & 1u) ? 0xFFu : 0x00u;
        else if ((ega->gc[1] >> p) & 1u)
            data = ((ega->gc[0] >> p) & 1u) ? 0xFFu : 0x00u;
        else
            data = rotated;
        switch (function) {
        case 1u: mixed = (uint8_t)(data & ega->latch[p]); break;
        case 2u: mixed = (uint8_t)(data | ega->latch[p]); break;
        case 3u: mixed = (uint8_t)(data ^ ega->latch[p]); break;
        default: mixed = data; break;
        }
        ega->plane[p][offset] = (uint8_t)((mixed & ega->gc[8]) | (ega->latch[p] & (uint8_t)~ega->gc[8]));
    }
    return PR_EGA_OK;
}
