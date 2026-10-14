#include "poolrad/recovered/resident_0709_rect_fill_00b7.h"

#include <string.h>

enum { EGA_PAGE_BYTES = 0x2000, EGA_SCANLINE_BYTES = 40 };

pr_res0709_rect_status pr_res0709_rect_fill_00b7(
    pr_res0709_rect_state *state, pr_res0709_rect_args args,
    pr_res0709_rect_report *report) {
    size_t plane, line, column;
    size_t page_base, first_line, last_line, width, last_byte;

    if (report == NULL) {
        return PR_RES0709_RECT_ARGUMENT;
    }
    *report = (pr_res0709_rect_report){0};
    if (state == NULL) {
        return PR_RES0709_RECT_ARGUMENT;
    }
    for (plane = 0; plane < 4; ++plane) {
        if (state->plane[plane] == NULL) {
            return PR_RES0709_RECT_ARGUMENT;
        }
    }
    if (args.left > args.right || args.right >= 40u ||
        args.top > args.bottom || args.bottom >= 25u ||
        args.page >= 8u) {
        return PR_RES0709_RECT_BOUNDARY;
    }
    page_base = (size_t)args.page * EGA_PAGE_BYTES;
    first_line = (size_t)args.top * 8u;
    last_line = (size_t)args.bottom * 8u + 7u;
    width = (size_t)args.right - args.left + 1u;
    last_byte = page_base + last_line * EGA_SCANLINE_BYTES + args.right;
    if (last_byte >= state->plane_bytes) {
        return PR_RES0709_RECT_BOUNDARY;
    }

    /* +00F7: select the requested drawing page via +00A0. */
    report->selected_segment = (uint16_t)(0xA000u + ((uint16_t)args.page << 9));
    state->draw_segment = report->selected_segment;

    /* +00FF: +0000 resets GC 0..5, 7, 8 and sequencer map mask. */
    for (plane = 0; plane <= 5; ++plane) {
        state->gc[plane] = 0;
    }
    state->gc[7] = 0x0Fu;
    state->gc[8] = 0xFFu;
    state->seq_index = 2u;
    state->seq_map_mask = 0x0Fu;

    /* +0103 and +0115..+014F: write mode 2, inclusive cells and scanlines.
     * SYSTEM 16A8 fills each EGA byte; GC bitmask FF and map mask 0F make
     * each color bit an all-one or all-zero plane byte. */
    state->gc_index = 5u;
    state->gc[5] = 2u;
    for (line = first_line; line <= last_line; ++line) {
        size_t at = page_base + line * EGA_SCANLINE_BYTES + args.left;
        for (plane = 0; plane < 4; ++plane) {
            uint8_t value = (args.color & (uint8_t)(1u << plane)) ? 0xFFu : 0u;
            for (column = 0; column < width; ++column) {
                report->changed_plane_bytes += state->plane[plane][at + column] != value;
            }
            memset(state->plane[plane] + at, value, width);
            report->plane_byte_stores += width;
        }
    }

    /* +0151..+016A: leave write mode 0, preserve DS:4A1A, and restore the
     * drawing segment through +00A0. */
    state->gc[5] = 0u;
    report->restored_segment =
        (uint16_t)(0xA000u + ((uint16_t)state->active_page << 9));
    state->draw_segment = report->restored_segment;
    return PR_RES0709_RECT_OK;
}
