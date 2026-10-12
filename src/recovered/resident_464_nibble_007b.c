#include "poolrad/recovered/resident_464_nibble_007b.h"

#include <string.h>

pr_r464_nibble_status pr_r464_nibble_007b(const uint8_t *source,
                                           size_t source_size,
                                           uint8_t selector,
                                           uint8_t *result) {
    uint8_t local[8];
    uint8_t value;

    if (!source || !result) return PR_R464_NIBBLE_ARGUMENT;
    if (source_size < sizeof local || selector >= 16u)
        return PR_R464_NIBBLE_BOUNDS;

    /* +0x0081..+0x0094: runtime Move copies eight source bytes into
     * the local stack frame. The four decoded callers pass index 0..15. */
    memcpy(local, source, sizeof local);
    value = local[selector >> 1];
    /* +0x0094..+0x00C6: odd selector chooses the low nibble;
     * even selector chooses the high nibble shifted down. */
    *result = (uint8_t)((selector & 1u) ? (value & 0x0Fu) : (value >> 4));
    return PR_R464_NIBBLE_OK;
}
