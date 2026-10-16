#include "poolrad/replay/u19_row14_expected_rgb.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t font[1416u];
static uint8_t source[PR_U19_ROW14_CELLS];
static uint8_t crop[PR_U19_ROW14_CROP_RGB_BYTES];
static uint8_t host[PR_U19_ROW14_HOST_RGB_BYTES];

static void put_crop(void) {
    size_t y;
    for (y = 0u; y < PR_U19_ROW14_CROP_HEIGHT; ++y) {
        memcpy(host + (((224u + y) * PR_U19_ROW14_HOST_WIDTH + 16u) * 3u),
               crop + y * PR_U19_ROW14_CROP_WIDTH * 3u,
               PR_U19_ROW14_CROP_WIDTH * 3u);
    }
}

int main(void) {
    pr_u19_row14_rgb_difference diff = {9u, 9u, 9u, 9u};
    size_t needed = 0u, bad;
    memset(source, 0, sizeof source);
    source[0] = (uint8_t)'a';
    font[8u] = 0x80u; /* Uppercase A maps to glyph index 1. */
    memset(crop, 0xCC, sizeof crop);

    CHECK(pr_u19_row14_expected_rgb24(source, sizeof source,
          font, sizeof font, crop, sizeof crop, &needed) ==
          PR_U19_ROW14_RGB_OK);
    CHECK(needed == sizeof crop);
    CHECK(crop[0] == 85u && crop[1] == 255u && crop[2] == 85u);
    CHECK(memcmp(crop, crop + 3u, 3u) == 0);
    CHECK(crop[6u] == 0u && crop[7u] == 0u && crop[8u] == 0u);
    CHECK(memcmp(crop, crop + PR_U19_ROW14_CROP_WIDTH * 3u, 3u) == 0);
    CHECK(crop[2u * PR_U19_ROW14_CROP_WIDTH * 3u] == 0u);

    put_crop();
    CHECK(pr_u19_row14_compare_host_rgb24(crop, sizeof crop,
          host, sizeof host, &diff) == PR_U19_ROW14_RGB_OK);
    CHECK(diff.different_host_pixels == 0u &&
          diff.different_source_pixels == 0u &&
          diff.different_cells == 0u &&
          diff.nonuniform_source_blocks == 0u);

    bad = ((224u * PR_U19_ROW14_HOST_WIDTH + 17u) * 3u);
    host[bad] = 0u;
    CHECK(pr_u19_row14_compare_host_rgb24(crop, sizeof crop,
          host, sizeof host, &diff) == PR_U19_ROW14_RGB_OK);
    CHECK(diff.different_host_pixels == 1u &&
          diff.different_source_pixels == 1u &&
          diff.different_cells == 1u &&
          diff.nonuniform_source_blocks == 1u);

    host[bad + 1u] = 0u;
    host[bad + 2u] = 0u;
    bad = ((224u * PR_U19_ROW14_HOST_WIDTH + 16u) * 3u);
    memset(host + bad, 0, 6u);
    bad += PR_U19_ROW14_HOST_WIDTH * 3u;
    memset(host + bad, 0, 6u);
    CHECK(pr_u19_row14_compare_host_rgb24(crop, sizeof crop,
          host, sizeof host, &diff) == PR_U19_ROW14_RGB_OK);
    CHECK(diff.different_host_pixels == 4u &&
          diff.different_source_pixels == 1u &&
          diff.different_cells == 1u &&
          diff.nonuniform_source_blocks == 0u);

    memset(crop, 0xCC, sizeof crop);
    CHECK(pr_u19_row14_expected_rgb24(source, sizeof source - 1u,
          font, sizeof font, crop, sizeof crop, &needed) ==
          PR_U19_ROW14_RGB_SOURCE_SIZE);
    CHECK(needed == 0u && crop[0] == 0xCCu);
    CHECK(pr_u19_row14_expected_rgb24(source, sizeof source,
          font, sizeof font - 1u, crop, sizeof crop, &needed) ==
          PR_U19_ROW14_RGB_FONT_SIZE);
    CHECK(needed == 0u && crop[0] == 0xCCu);
    CHECK(pr_u19_row14_expected_rgb24(source, sizeof source,
          font, sizeof font, crop, sizeof crop - 1u, &needed) ==
          PR_U19_ROW14_RGB_OUTPUT_SIZE);
    CHECK(needed == sizeof crop && crop[0] == 0xCCu);
    CHECK(pr_u19_row14_expected_rgb24(source, sizeof source,
          font, sizeof font, NULL, 0u, &needed) ==
          PR_U19_ROW14_RGB_OUTPUT_SIZE);
    CHECK(needed == sizeof crop);
    CHECK(pr_u19_row14_expected_rgb24(source, sizeof source,
          font, sizeof font, crop, sizeof crop, NULL) ==
          PR_U19_ROW14_RGB_ARGUMENT);

    diff.different_host_pixels = 9u;
    CHECK(pr_u19_row14_compare_host_rgb24(crop, sizeof crop - 1u,
          host, sizeof host, &diff) == PR_U19_ROW14_RGB_OUTPUT_SIZE);
    CHECK(diff.different_host_pixels == 9u);
    CHECK(pr_u19_row14_compare_host_rgb24(crop, sizeof crop,
          host, sizeof host - 1u, &diff) == PR_U19_ROW14_RGB_HOST_SIZE);
    CHECK(diff.different_host_pixels == 9u);
    CHECK(pr_u19_row14_compare_host_rgb24(crop, sizeof crop,
          host, sizeof host, NULL) == PR_U19_ROW14_RGB_ARGUMENT);
    return 0;
}
