#include "poolrad/recovered/resident_string_0425.h"

#include "poolrad/recovered/resident_string_0356.h"

#include <string.h>

pr_rec_0802_0425_status pr_rec_res_0802_0425(
    const uint8_t *source, size_t source_size,
    const uint8_t *table, size_t table_size,
    uint8_t *output, size_t output_capacity)
{
    uint8_t work[256], next[256], live_table[10], needle[2];
    unsigned i, len;

    if (!source || !table || !output) return PR_REC_0802_0425_ARGUMENT;
    if (source_size == 0u || source_size - 1u < source[0] ||
        table_size < sizeof live_table)
        return PR_REC_0802_0425_BOUNDS;

    /* SYSTEM 0x0B25 copies the source to a 255-character local. The table
     * is a live DS input; copying it also permits host buffers to alias. */
    memcpy(work, source, (size_t)source[0] + 1u);
    memcpy(live_table, table, sizeof live_table);

    /* +0x0440..+0x0487: ten one-based table reads, each converted through
     * SYSTEM 0x0C3B and passed to the resident 0x0356 delete routine. */
    needle[0] = 1u;
    for (i = 0u; i < sizeof live_table; ++i) {
        needle[1] = live_table[i];
        if (pr_rec_res_0802_0356(work, (size_t)work[0] + 1u,
                                 needle, sizeof needle, next, sizeof next)
            != PR_REC_0802_0356_OK)
            return PR_REC_0802_0425_BOUNDS;
        memcpy(work, next, (size_t)next[0] + 1u);
    }

    /* +0x0489..+0x04F3: SYSTEM Copy(...,1,8), then ASCII UpCase on
     * every copied byte. +0x0504 stores with a 40-byte cap, which cannot
     * truncate this at-most-eight-byte prefix. */
    len = work[0] < 8u ? work[0] : 8u;
    if (output_capacity < (size_t)len + 1u)
        return PR_REC_0802_0425_BOUNDS;
    output[0] = (uint8_t)len;
    for (i = 0u; i < len; ++i) {
        uint8_t c = work[i + 1u];
        output[i + 1u] = (c >= 0x61u && c <= 0x7au)
            ? (uint8_t)(c - 0x20u) : c;
    }
    return PR_REC_0802_0425_OK;
}
