#include "poolrad/recovered/resident_464_row56_transfer.h"

#include <string.h>

pr_r464_row56_result pr_r464_row56_transfer(
    pr_r464_row56_state *state, const uint8_t *object, size_t object_size,
    uint16_t *output_offset) {
    pr_r464_row56_state next;
    unsigned plane, column, selected;
    unsigned offset;
    size_t last_byte;

    if (!state || !object || !output_offset) return PR_R464_ROW56_BAD_ARG;
    /* These are host bounds for the reached 11-byte path, not invented
     * branches in the original 16-bit routine. */
    if (state->dirty != 1 || state->last < state->first ||
        state->last - state->first != 43u || (state->first & 3u) != 0 ||
        state->row_base != 0x0A00u || state->low != 2u ||
        state->page_base != 0x0140u || state->gc_set_reset != 0 ||
        state->gc_enable_set_reset != 0 || state->gc_rotate != 0 ||
        state->gc_mode != 0 || state->gc_bit_mask != 0xFFu)
        return PR_R464_ROW56_UNSUPPORTED;
    for (plane = 0; plane < 4; ++plane)
        if ((state->map_mask[plane] & 0xF0u) != 0)
            return PR_R464_ROW56_UNSUPPORTED;
    last_byte = (size_t)0x17u + state->last;
    if (last_byte > 0xFFFFu) return PR_R464_ROW56_UNSUPPORTED;
    if (object_size <= last_byte) return PR_R464_ROW56_SHORT_OBJECT;

    offset = (unsigned)state->row_base + state->low + 1u;
    if (offset + PR_R464_ROW56_BYTES > 0x10000u)
        return PR_R464_ROW56_UNSUPPORTED;
    next = *state;

    /* Four CPU-byte loops, one source lane each. With pre-flush EGA GC
     * mode 0, bit mask FF, rotate 0 and set/reset disabled, each CPU byte
     * replaces only the staging planes enabled by its Sequencer map mask. */
    for (plane = 0; plane < 4; ++plane) {
        for (column = 0; column < PR_R464_ROW56_BYTES; ++column) {
            uint8_t value = object[0x17u + state->first + plane + 4u * column];
            for (selected = 0; selected < 4; ++selected)
                if (state->map_mask[plane] & (1u << selected))
                    next.staging.plane[selected][column] = value;
        }
    }

    /* The original sets Sequencer map mask 0F and GC write mode 1, then
     * runtime Move executes forward REP MOVSB (equal source/dest offsets).
     * Each A200 source read fills all four EGA latches; the A000 write
     * consumes those latches, independent of the CPU read-map byte. */
    for (column = 0; column < PR_R464_ROW56_BYTES; ++column) {
        uint8_t latch[4];
        for (plane = 0; plane < 4; ++plane)
            latch[plane] = next.staging.plane[plane][column];
        for (plane = 0; plane < 4; ++plane)
            next.page.plane[plane][column] = latch[plane];
    }

    /* RES_0464:1E25..1E53 clears the row dirty flag and extents after
     * all dirty-row transfers complete. */
    next.dirty = 0;
    next.first = 0xFFFFu;
    next.last = 0;
    *state = next;
    *output_offset = (uint16_t)offset;
    return PR_R464_ROW56_OK;
}
