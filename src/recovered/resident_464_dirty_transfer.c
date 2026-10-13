#include "poolrad/recovered/resident_464_dirty_transfer.h"

static size_t row_length(const pr_r464_dirty_row *row) {
    return ((size_t)row->last - row->first) / 4u + 1u;
}

static size_t stage_offset(const pr_r464_dirty_row *row) {
    return (size_t)row->stage_base + row->low + 1u;
}

static size_t page_offset(const pr_r464_dirty_state *state, size_t index) {
    return (size_t)state->page_base + 1u + index * 40u +
           state->row[index].low;
}

pr_r464_dirty_result pr_r464_dirty_transfer(
    pr_r464_dirty_state *state, const uint8_t *object, size_t object_size,
    pr_r464_dirty_report *report) {
    pr_r464_dirty_report next_report = {0};
    size_t row_index, column;
    unsigned lane, selected;

    if (!state || !object || !report) return PR_R464_DIRTY_BAD_ARG;
    if (state->vram_size > 0x10000u - state->vram_origin ||
        state->gc_set_reset != 0 || state->gc_enable_set_reset != 0 ||
        state->gc_rotate != 0 || state->gc_mode != 0 ||
        state->gc_bit_mask != 0xFFu)
        return PR_R464_DIRTY_UNSUPPORTED;
    for (lane = 0; lane < 4; ++lane) {
        if (!state->staging[lane] || !state->page[lane])
            return PR_R464_DIRTY_BAD_ARG;
        if (state->map_mask[lane] & 0xF0u)
            return PR_R464_DIRTY_UNSUPPORTED;
    }

    /* Preflight every reached source and destination before any writes.
     * The DOS routine itself has no host allocation-length checks. */
    for (row_index = 0; row_index < PR_R464_DIRTY_ROWS; ++row_index) {
        const pr_r464_dirty_row *row = &state->row[row_index];
        size_t count, stage_at, page_at, source_end;
        if (!row->dirty) continue;
        if (row->last < row->first ||
            (row->first & 3u) != 0 ||
            ((row->last - row->first) & 3u) != 3u)
            return PR_R464_DIRTY_UNSUPPORTED;
        count = row_length(row);
        stage_at = stage_offset(row);
        page_at = page_offset(state, row_index);
        source_end = (size_t)row->last + 0x17u;
        if (source_end > 0xFFFFu || stage_at + count > 0x10000u ||
            page_at + count > 0x10000u)
            return PR_R464_DIRTY_UNSUPPORTED;
        if (source_end >= object_size)
            return PR_R464_DIRTY_SHORT_OBJECT;
        if (stage_at < state->vram_origin ||
            page_at < state->vram_origin ||
            stage_at + count > (size_t)state->vram_origin + state->vram_size ||
            page_at + count > (size_t)state->vram_origin + state->vram_size)
            return PR_R464_DIRTY_SHORT_VRAM;
        next_report.dirty_rows++;
        next_report.transfer_bytes += count;
        next_report.page_plane_writes += count * 4u;
        for (lane = 0; lane < 4; ++lane)
            for (selected = 0; selected < 4; ++selected)
                if (state->map_mask[lane] & (1u << selected))
                    next_report.stage_plane_writes += count;
    }

    /* Phase one: four plane-selector loops within each dirty row. The
     * measured pre-flush GC state is mode 0, bit mask FF, no rotate or
     * enabled set/reset; each CPU byte replaces its selected staging
     * planes. */
    for (row_index = 0; row_index < PR_R464_DIRTY_ROWS; ++row_index) {
        const pr_r464_dirty_row *row = &state->row[row_index];
        size_t at;
        if (!row->dirty) continue;
        at = stage_offset(row) - state->vram_origin;
        for (lane = 0; lane < 4; ++lane)
            for (column = 0; column < row_length(row); ++column) {
                uint8_t value =
                    object[0x17u + row->first + lane + 4u * column];
                for (selected = 0; selected < 4; ++selected)
                    if (state->map_mask[lane] & (1u << selected))
                        state->staging[selected][at + column] = value;
            }
    }

    /* Phase two: map mask 0F and GC write mode 1. Every A200 source
     * read loads four latches; runtime Move's forward REP MOVSB writes
     * those four bytes to A000, regardless of the CPU read-map byte. */
    for (row_index = 0; row_index < PR_R464_DIRTY_ROWS; ++row_index) {
        const pr_r464_dirty_row *row = &state->row[row_index];
        size_t from, to;
        if (!row->dirty) continue;
        from = stage_offset(row) - state->vram_origin;
        to = page_offset(state, row_index) - state->vram_origin;
        for (column = 0; column < row_length(row); ++column) {
            uint8_t latch[4];
            for (lane = 0; lane < 4; ++lane)
                latch[lane] = state->staging[lane][from + column];
            for (lane = 0; lane < 4; ++lane)
                state->page[lane][to + column] = latch[lane];
        }
    }

    /* The three reached FillChar calls reset all rows after the copies. */
    for (row_index = 0; row_index < PR_R464_DIRTY_ROWS; ++row_index) {
        state->row[row_index].dirty = 0;
        state->row[row_index].first = 0xFFFFu;
        state->row[row_index].last = 0;
    }
    *report = next_report;
    return PR_R464_DIRTY_OK;
}
