#include "poolrad/recovered/u19_row14_cpu_sources.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static int check_event(const pr_u19_row14_cpu_event *event,
                       uint16_t site, uint16_t segment, uint16_t offset,
                       uint8_t row, uint8_t plane, uint8_t page,
                       uint8_t source, uint8_t from_glyph) {
    CHECK(event->store_offset == site);
    CHECK(event->segment == segment && event->offset == offset);
    CHECK(event->glyph_row == row && event->plane == plane &&
          event->page == page);
    CHECK(event->map_mask == (uint8_t)(1u << plane));
    CHECK(event->source_byte == source && event->glyph_source == from_glyph);
    return 0;
}

int main(void) {
    uint8_t block[PR_FONT201_BLOCK_SIZE] = {0};
    pr_font201 font;
    pr_u19_row14_cpu_event events[PR_U19_ROW14_CPU_EVENTS];
    static pr_u19_row14_cpu_event row_events[PR_U19_ROW14_FIXED_EVENTS];
    uint8_t row_text[PR_U19_ROW14_FIXED_CELLS];
    size_t count = 99u, row, plane;

    block[20u * 8u] = 0xA5u;        /* synthetic 'T' after UpCase/mod-64 */
    block[20u * 8u + 7u] = 0x3Cu;
    CHECK(pr_font201_open(block, sizeof block, &font) == PR_FONT201_OK);
    CHECK(pr_u19_row14_cpu_sources(&font, (uint8_t)'T', 1, 15u, 10u,
           0xA000u, 0xA200u, events, PR_U19_ROW14_CPU_EVENTS, &count) ==
          PR_FONT201_OK);
    CHECK(count == 64u);
    CHECK(check_event(&events[0], 0x0445u, 0xA000u, 0x118Fu,
                      0u, 0u, 0u, 0u, 0u) == 0);
    CHECK(check_event(&events[1], 0x0451u, 0xA200u, 0x118Fu,
                      0u, 0u, 1u, 0u, 0u) == 0);
    CHECK(check_event(&events[2], 0x0412u, 0xA000u, 0x118Fu,
                      0u, 1u, 0u, 0xA5u, 1u) == 0);
    CHECK(check_event(&events[3], 0x0438u, 0xA200u, 0x118Fu,
                      0u, 1u, 1u, 0xA5u, 1u) == 0);
    CHECK(check_event(&events[4], 0x0445u, 0xA000u, 0x118Fu,
                      0u, 2u, 0u, 0u, 0u) == 0);
    CHECK(check_event(&events[6], 0x0412u, 0xA000u, 0x118Fu,
                      0u, 3u, 0u, 0xA5u, 1u) == 0);
    CHECK(check_event(&events[62], 0x0412u, 0xA000u, 0x12A7u,
                      7u, 3u, 0u, 0x3Cu, 1u) == 0);
    CHECK(check_event(&events[63], 0x0438u, 0xA200u, 0x12A7u,
                      7u, 3u, 1u, 0x3Cu, 1u) == 0);

    for (row = 0u; row < 8u; ++row) {
        for (plane = 0u; plane < 4u; ++plane) {
            size_t first = row * 8u + plane * 2u;
            uint16_t offset = (uint16_t)(0x118Fu + row * 40u);
            CHECK(events[first].offset == offset &&
                  events[first + 1u].offset == offset);
            CHECK(events[first].plane == plane &&
                  events[first + 1u].plane == plane);
            CHECK(events[first].page == 0u &&
                  events[first + 1u].page == 1u);
            CHECK(events[first].source_byte == events[first + 1u].source_byte);
        }
    }

    /* A zero glyph byte still takes the glyph-source path for a set color bit. */
    CHECK(events[10].source_byte == 0u &&
          events[10].glyph_source == 1u &&
          events[10].store_offset == 0x0412u);
    CHECK(pr_u19_row14_cpu_sources(&font, (uint8_t)'T', 1, 39u, 0xFAu,
           0xA000u, 0xA000u, events, PR_U19_ROW14_CPU_EVENTS, &count) ==
          PR_FONT201_OK);
    CHECK(count == 64u && events[0].offset == 0x11A7u &&
          events[63].offset == 0x12BFu);

    memset(row_text, 'T', sizeof row_text);
    CHECK(pr_u19_row14_fixed_sources(&font, row_text, sizeof row_text,
           0xA000u, 0xA200u, row_events,
           PR_U19_ROW14_FIXED_EVENTS, &count) == PR_FONT201_OK);
    CHECK(count == 2304u);
    CHECK(check_event(&row_events[0], 0x0445u, 0xA000u, 0x1181u,
                      0u, 0u, 0u, 0u, 0u) == 0);
    CHECK(check_event(&row_events[14u * 64u + 2u], 0x0412u, 0xA000u,
                      0x118Fu, 0u, 1u, 0u, 0xA5u, 1u) == 0);
    CHECK(check_event(&row_events[2303], 0x0438u, 0xA200u, 0x12BCu,
                      7u, 3u, 1u, 0x3Cu, 1u) == 0);
    memset(row_events, 0xA7, sizeof row_events);
    CHECK(pr_u19_row14_fixed_sources(&font, row_text, 35u,
           0xA000u, 0xA200u, row_events,
           PR_U19_ROW14_FIXED_EVENTS, &count) == PR_FONT201_ARGUMENT);
    CHECK(count == 0u && row_events[0].source_byte == 0xA7u);
    CHECK(pr_u19_row14_fixed_sources(&font, row_text, sizeof row_text,
           0xA000u, 0xA200u, row_events,
           PR_U19_ROW14_FIXED_EVENTS - 1u, &count) == PR_FONT201_ARGUMENT);
    CHECK(count == 0u && row_events[0].source_byte == 0xA7u);

    memset(events, 0xA7, sizeof events);
    count = 99u;
    CHECK(pr_u19_row14_cpu_sources(&font, (uint8_t)'T', 1, 15u, 10u,
           0xA000u, 0xA200u, events, 63u, &count) == PR_FONT201_ARGUMENT);
    CHECK(count == 0u && events[0].source_byte == 0xA7u);
    CHECK(pr_u19_row14_cpu_sources(&font, (uint8_t)'T', 1, 40u, 10u,
           0xA000u, 0xA200u, events, 64u, &count) == PR_FONT201_BOUNDS);
    CHECK(count == 0u && events[0].source_byte == 0xA7u);
    CHECK(pr_u19_row14_cpu_sources(&font, 0xFEu, 0, 15u, 10u,
           0xA000u, 0xA200u, events, 64u, &count) == PR_FONT201_GLYPH);
    CHECK(count == 0u && events[0].source_byte == 0xA7u);
    font.size = 1u;
    CHECK(pr_u19_row14_cpu_sources(&font, (uint8_t)'T', 1, 15u, 10u,
           0xA000u, 0xA200u, events, 64u, &count) == PR_FONT201_SIZE);
    CHECK(count == 0u && events[0].source_byte == 0xA7u);
    CHECK(pr_u19_row14_cpu_sources(NULL, (uint8_t)'T', 1, 15u, 10u,
           0xA000u, 0xA200u, events, 64u, &count) == PR_FONT201_ARGUMENT);
    CHECK(pr_u19_row14_cpu_sources(&font, (uint8_t)'T', 1, 15u, 10u,
           0xA000u, 0xA200u, NULL, 64u, &count) == PR_FONT201_ARGUMENT);
    return 0;
}
