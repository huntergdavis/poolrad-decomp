#include "poolrad/replay/u19_row14_expected_rgb.h"

#include <string.h>

#define FONT_BYTES 1416u
#define HOST_X 16u
#define HOST_Y 224u

static uint8_t glyph_index(uint8_t ch) {
    if (ch >= (uint8_t)'a' && ch <= (uint8_t)'z')
        ch = (uint8_t)(ch - 0x20u);
    return (uint8_t)(ch % 64u);
}

pr_u19_row14_rgb_status pr_u19_row14_expected_rgb24(
    const uint8_t *source, size_t source_size,
    const uint8_t *font, size_t font_size,
    uint8_t *crop, size_t capacity, size_t *needed) {
    size_t y, x;
    if (needed == NULL) return PR_U19_ROW14_RGB_ARGUMENT;
    *needed = 0u;
    if (source == NULL || font == NULL) return PR_U19_ROW14_RGB_ARGUMENT;
    if (source_size != PR_U19_ROW14_CELLS)
        return PR_U19_ROW14_RGB_SOURCE_SIZE;
    if (font_size != FONT_BYTES) return PR_U19_ROW14_RGB_FONT_SIZE;
    *needed = PR_U19_ROW14_CROP_RGB_BYTES;
    if (crop == NULL || capacity < *needed)
        return PR_U19_ROW14_RGB_OUTPUT_SIZE;

    for (y = 0u; y < PR_U19_ROW14_CROP_HEIGHT; ++y) {
        for (x = 0u; x < PR_U19_ROW14_CROP_WIDTH; ++x) {
            size_t source_x = x / 2u;
            uint8_t ch = source[source_x / 8u];
            uint8_t glyph = glyph_index(ch);
            uint8_t bits = font[(size_t)glyph * 8u + y / 2u];
            uint8_t on = (uint8_t)((bits & (0x80u >> (source_x % 8u))) != 0u);
            size_t at = (y * PR_U19_ROW14_CROP_WIDTH + x) * 3u;
            crop[at] = on != 0u ? 85u : 0u;
            crop[at + 1u] = on != 0u ? 255u : 0u;
            crop[at + 2u] = on != 0u ? 85u : 0u;
        }
    }
    return PR_U19_ROW14_RGB_OK;
}

pr_u19_row14_rgb_status pr_u19_row14_compare_host_rgb24(
    const uint8_t *expected_crop, size_t crop_size,
    const uint8_t *host, size_t host_size,
    pr_u19_row14_rgb_difference *difference) {
    size_t cell, gy, gx;
    pr_u19_row14_rgb_difference pending = {0u, 0u, 0u, 0u};
    if (expected_crop == NULL || host == NULL || difference == NULL)
        return PR_U19_ROW14_RGB_ARGUMENT;
    if (crop_size != PR_U19_ROW14_CROP_RGB_BYTES)
        return PR_U19_ROW14_RGB_OUTPUT_SIZE;
    if (host_size != PR_U19_ROW14_HOST_RGB_BYTES)
        return PR_U19_ROW14_RGB_HOST_SIZE;

    for (cell = 0u; cell < PR_U19_ROW14_CELLS; ++cell) {
        int cell_differs = 0;
        for (gy = 0u; gy < 8u; ++gy) {
            for (gx = 0u; gx < 8u; ++gx) {
                const uint8_t *first = host +
                    (((HOST_Y + gy * 2u) * PR_U19_ROW14_HOST_WIDTH +
                      HOST_X + cell * 16u + gx * 2u) * 3u);
                size_t dy, dx;
                int source_differs = 0;
                int nonuniform = 0;
                for (dy = 0u; dy < 2u; ++dy) {
                    for (dx = 0u; dx < 2u; ++dx) {
                        const uint8_t *actual = first +
                            (dy * PR_U19_ROW14_HOST_WIDTH + dx) * 3u;
                        const uint8_t *expected = expected_crop +
                            (((gy * 2u + dy) * PR_U19_ROW14_CROP_WIDTH +
                              cell * 16u + gx * 2u + dx) * 3u);
                        if (memcmp(actual, expected, 3u) != 0) {
                            ++pending.different_host_pixels;
                            source_differs = 1;
                        }
                        if (memcmp(actual, first, 3u) != 0)
                            nonuniform = 1;
                    }
                }
                pending.different_source_pixels += (size_t)source_differs;
                pending.nonuniform_source_blocks += (size_t)nonuniform;
                if (source_differs) cell_differs = 1;
            }
        }
        pending.different_cells += (size_t)cell_differs;
    }
    *difference = pending;
    return PR_U19_ROW14_RGB_OK;
}
