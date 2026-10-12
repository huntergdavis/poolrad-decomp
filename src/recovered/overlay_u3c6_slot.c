/* Portable C translation of GAME.OVR unit 0x3c6 entries 12..15; see
 * docs/re/U9E_0160.md. */
#include "poolrad/recovered/overlay_u3c6_slot.h"

uint8_t pr_u3c6_entry15(const pr_u3c6_state *s, const void *far_ptr)
{
    uint8_t i = 0, found = 0, result = 0;      /* [bp-3], [bp-2], [bp-1]: +0x0C65..+0x0C6D */
    do {
        i++;                                   /* +0x0C71: inc byte [bp-3] (wraps 255 -> 0) */
        if (s->slot_ptr[i] == far_ptr)         /* +0x0C7F..+0x0C8F: both words equal */
            found = 1;
    } while (!found && i != s->bytes[PR_U3C6_COUNT_DS - PR_U3C6_BYTES_DS]);   /* +0x0C95..+0x0CA2 */
    if (found)                                 /* +0x0CA4..+0x0CAD */
        result = i;
    return result;
}

uint8_t pr_u3c6_entry12(const pr_u3c6_state *s, const void *p)
{
    return s->bytes[4u * pr_u3c6_entry15(s, p) + 0u];   /* DS:0x5F27 + 4 * slot */
}

uint8_t pr_u3c6_entry13(const pr_u3c6_state *s, const void *p)
{
    return s->bytes[4u * pr_u3c6_entry15(s, p) + 1u];   /* DS:0x5F28 + 4 * slot */
}

uint8_t pr_u3c6_entry14(const pr_u3c6_state *s, const void *p)
{
    return s->bytes[4u * pr_u3c6_entry15(s, p) + 3u];   /* DS:0x5F2A + 4 * slot */
}
