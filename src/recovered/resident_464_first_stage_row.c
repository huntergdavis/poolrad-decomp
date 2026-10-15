#include "poolrad/recovered/resident_464_first_stage_row.h"

#include <stdint.h>

pr_r464_first_stage_status pr_r464_first_stage_row(
    const pr_r464_dirty_state *state, uint8_t video_state,
    const uint8_t *object, size_t object_size,
    pr_r464_first_stage_report *report)
{
    const pr_r464_dirty_row *entry;
    pr_r464_first_stage_report next;
    size_t row, count, stage_at, from;
    unsigned plane;

    if (!state || !object || !report) return PR_R464_FIRST_STAGE_BAD_ARG;
    if (video_state != 1u || state->gc_set_reset != 0u ||
        state->gc_enable_set_reset != 0u || state->gc_rotate != 0u ||
        state->gc_mode != 0u || state->gc_bit_mask != 0xFFu)
        return PR_R464_FIRST_STAGE_OTHER_PATH;
    for (plane = 0; plane < 4u; ++plane)
        if (state->map_mask[plane] != (uint8_t)(1u << plane))
            return PR_R464_FIRST_STAGE_OTHER_PATH;

    for (row = 0; row < PR_R464_DIRTY_ROWS; ++row)
        if (state->row[row].dirty != 0u) break;
    if (row == PR_R464_DIRTY_ROWS) return PR_R464_FIRST_STAGE_OTHER_PATH;
    entry = &state->row[row];
    if (entry->last < entry->first || (entry->first & 3u) != 0u ||
        ((entry->last - entry->first) & 3u) != 3u)
        return PR_R464_FIRST_STAGE_OTHER_PATH;
    count = ((size_t)entry->last - entry->first) / 4u + 1u;
    stage_at = (size_t)entry->stage_base + entry->low + 1u;
    if (state->vram_size > 0x10000u - state->vram_origin ||
        stage_at < state->vram_origin ||
        stage_at + count > 0x10000u ||
        stage_at + count > (size_t)state->vram_origin + state->vram_size ||
        (size_t)entry->last + 0x17u > 0xFFFFu ||
        (size_t)entry->last + 0x17u >= object_size)
        return PR_R464_FIRST_STAGE_BOUNDS;
    for (plane = 0; plane < 4u; ++plane)
        if (!state->staging[plane]) return PR_R464_FIRST_STAGE_BAD_ARG;

    /* +1C09 selects one plane at a time. +1C37/+1C7E/+1CC6/+1D0F
     * write source bytes spaced four apart into consecutive A200 offsets. */
    from = stage_at - state->vram_origin;
    for (plane = 0; plane < 4u; ++plane) {
        size_t column;
        for (column = 0; column < count; ++column)
            state->staging[plane][from + column] =
                object[0x17u + entry->first + plane + 4u * column];
    }
    next.row = (unsigned)row;
    next.first = entry->first;
    next.last = entry->last;
    next.stage_offset = stage_at;
    next.bytes_per_plane = count;
    next.plane_writes = count * 4u;
    *report = next;
    return PR_R464_FIRST_STAGE_OK;
}
