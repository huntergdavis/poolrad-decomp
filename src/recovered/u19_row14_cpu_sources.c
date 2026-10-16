#include "poolrad/recovered/u19_row14_cpu_sources.h"
#include "poolrad/recovered/slot_a_glyph_plane_byte.h"

pr_font201_status pr_u19_row14_cpu_sources(
    const pr_font201 *font, uint8_t ch, int upcase, uint8_t column,
    uint8_t color, uint16_t active_segment, uint16_t draw_segment,
    pr_u19_row14_cpu_event *events, size_t capacity, size_t *count) {
    size_t glyph, row, plane, n = 0u;

    if (count != NULL) {
        *count = 0u;
    }
    if (font == NULL || font->data == NULL || events == NULL || count == NULL ||
        capacity < PR_U19_ROW14_CPU_EVENTS) {
        return PR_FONT201_ARGUMENT;
    }
    if (font->size != PR_FONT201_BLOCK_SIZE) {
        return PR_FONT201_SIZE;
    }
    if (column >= PR_FONT201_COLUMNS) {
        return PR_FONT201_BOUNDS;
    }
    glyph = pr_font201_glyph_index(ch, upcase);
    if (glyph >= PR_FONT201_GLYPH_COUNT) {
        return PR_FONT201_GLYPH;
    }

    for (row = 0u; row < PR_FONT201_GLYPH_BYTES; ++row) {
        uint16_t offset = (uint16_t)(14u * 8u * 40u + column + row * 40u);
        for (plane = 0u; plane < 4u; ++plane) {
            uint8_t mask = (uint8_t)(1u << plane);
            uint8_t glyph_source = (uint8_t)((color & mask) != 0u);
            uint8_t source = 0u;
            pr_font201_status status = pr_slot_a_glyph_plane_byte(
                font, ch, upcase, (uint8_t)row, color, (unsigned)plane, &source);
            unsigned page;
            if (status != PR_FONT201_OK) {
                return status; /* unreachable after the complete input validation */
            }
            for (page = 0u; page < 2u; ++page) {
                pr_u19_row14_cpu_event *event = &events[n++];
                event->store_offset = (uint16_t)(glyph_source != 0u ?
                    (page == 0u ? 0x0412u : 0x0438u) :
                    (page == 0u ? 0x0445u : 0x0451u));
                event->segment = page == 0u ? active_segment : draw_segment;
                event->offset = offset;
                event->glyph_row = (uint8_t)row;
                event->plane = (uint8_t)plane;
                event->page = (uint8_t)page;
                event->map_mask = mask;
                event->source_byte = source;
                event->glyph_source = glyph_source;
            }
        }
    }
    *count = n;
    return PR_FONT201_OK;
}

pr_font201_status pr_u19_row14_fixed_sources(
    const pr_font201 *font, const uint8_t *text, size_t length,
    uint16_t active_segment, uint16_t draw_segment,
    pr_u19_row14_cpu_event *events, size_t capacity, size_t *count) {
    size_t cell;
    if (count != NULL) {
        *count = 0u;
    }
    if (font == NULL || font->data == NULL || text == NULL || events == NULL ||
        count == NULL || length != PR_U19_ROW14_FIXED_CELLS ||
        capacity < PR_U19_ROW14_FIXED_EVENTS) {
        return PR_FONT201_ARGUMENT;
    }
    if (font->size != PR_FONT201_BLOCK_SIZE) {
        return PR_FONT201_SIZE;
    }
    for (cell = 0u; cell < PR_U19_ROW14_FIXED_CELLS; ++cell) {
        size_t emitted = 0u;
        pr_font201_status status = pr_u19_row14_cpu_sources(
            font, text[cell], 1, (uint8_t)(cell + 1u), 10u,
            active_segment, draw_segment,
            events + cell * PR_U19_ROW14_CPU_EVENTS,
            capacity - cell * PR_U19_ROW14_CPU_EVENTS, &emitted);
        if (status != PR_FONT201_OK || emitted != PR_U19_ROW14_CPU_EVENTS) {
            return status == PR_FONT201_OK ? PR_FONT201_ARGUMENT : status;
        }
    }
    *count = PR_U19_ROW14_FIXED_EVENTS;
    return PR_FONT201_OK;
}
