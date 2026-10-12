/* Portable C parity for GAME.OVR unit 0x68 internal +0x05AB.
 * Exact instruction and caller evidence: docs/re/U68_RECORD_SCAN_05AB.md. */
#include "poolrad/recovered/overlay_u68_record_scan.h"

pr_u68_record_scan_status pr_u68_record_scan(const uint8_t *segment,
                                             size_t segment_size,
                                             uint16_t record_offset,
                                             uint16_t value_word,
                                             uint16_t *ax_out)
{
    unsigned index;
    const uint8_t value = (uint8_t)value_word; /* +0x05C9: [bp+0xA] is one byte */

    if (!segment || segment_size < PR_U68_RECORD_SEGMENT_SIZE || !ax_out)
        return PR_U68_RECORD_SCAN_ARGUMENT;

    /* +0x05B1..+0x05D1: unsigned index 0..20, first equal byte wins. */
    for (index = 0; index < PR_U68_RECORD_SCAN_COUNT; ++index) {
        const uint16_t offset = (uint16_t)(record_offset + PR_U68_RECORD_SCAN_START + index);
        if (segment[offset] == value) {
            *ax_out = (uint16_t)index; /* +0x05D9..+0x05E5; AH was cleared at +0x05BE */
            return PR_U68_RECORD_SCAN_OK;
        }
    }

    *ax_out = PR_U68_RECORD_SCAN_MISS; /* +0x05E1..+0x05E5: AX=0x00FF */
    return PR_U68_RECORD_SCAN_OK;
}
