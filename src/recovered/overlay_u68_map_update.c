/* Full code-path translation of GAME.OVR unit 0x68 +0x0109.
 * Evidence: docs/re/U68_MAP_UPDATE_0109.md. */
#include "poolrad/recovered/overlay_u68_map_update.h"

#define PR_U68_MAP_SEGMENT_SIZE 65536u

pr_u68_map_update_status pr_u68_map_update(uint8_t x, uint8_t y,
                                           uint8_t selector,
                                           uint16_t map_base_offset,
                                           uint8_t *map_segment,
                                           size_t map_segment_size)
{
    uint8_t keep, set;
    uint16_t at;

    /* +0x010C..+0x0124: signed byte comparisons against 0 and 15.
     * High bytes of the three pushed words are never read. */
    if (x > 15u || y > 15u)
        return PR_U68_MAP_UPDATE_OK;

    /* +0x0127..+0x01F4: only these four selectors reach LES or ES. */
    switch (selector) {
    case 6u: keep = 0x3Fu; set = 0x40u; break; /* +0x012A..+0x016A */
    case 4u: keep = 0xCFu; set = 0x10u; break; /* +0x016D..+0x01AD */
    case 2u: keep = 0xF3u; set = 0x04u; break; /* +0x01B0..+0x01F0 */
    case 0u: keep = 0xFCu; set = 0x01u; break; /* +0x01F2..+0x0232 */
    default: return PR_U68_MAP_UPDATE_OK;
    }

    /* DOS loads ES:DI from DS:[0x6A5C], adds 16*y and x to DI, then
     * reads/writes ES:[DI+0x300]. Each effective offset wraps at 16 bits. */
    if (!map_segment || map_segment_size < PR_U68_MAP_SEGMENT_SIZE)
        return PR_U68_MAP_UPDATE_ARGUMENT;
    at = (uint16_t)(map_base_offset + 0x300u + 16u * y + x);
    map_segment[at] = (uint8_t)((map_segment[at] & keep) | set);
    return PR_U68_MAP_UPDATE_OK;
}
