#ifndef POOLRAD_REPLAY_U19_ROW14_EXPECTED_RGB_H
#define POOLRAD_REPLAY_U19_ROW14_EXPECTED_RGB_H

#include <stddef.h>
#include <stdint.h>

#define PR_U19_ROW14_CELLS 36u
#define PR_U19_ROW14_SOURCE_WIDTH (PR_U19_ROW14_CELLS * 8u)
#define PR_U19_ROW14_SOURCE_HEIGHT 8u
#define PR_U19_ROW14_CROP_WIDTH (PR_U19_ROW14_SOURCE_WIDTH * 2u)
#define PR_U19_ROW14_CROP_HEIGHT (PR_U19_ROW14_SOURCE_HEIGHT * 2u)
#define PR_U19_ROW14_CROP_RGB_BYTES \
    (PR_U19_ROW14_CROP_WIDTH * PR_U19_ROW14_CROP_HEIGHT * 3u)
#define PR_U19_ROW14_HOST_WIDTH 640u
#define PR_U19_ROW14_HOST_HEIGHT 400u
#define PR_U19_ROW14_HOST_RGB_BYTES \
    (PR_U19_ROW14_HOST_WIDTH * PR_U19_ROW14_HOST_HEIGHT * 3u)

typedef enum {
    PR_U19_ROW14_RGB_OK = 0,
    PR_U19_ROW14_RGB_ARGUMENT,
    PR_U19_ROW14_RGB_SOURCE_SIZE,
    PR_U19_ROW14_RGB_FONT_SIZE,
    PR_U19_ROW14_RGB_OUTPUT_SIZE,
    PR_U19_ROW14_RGB_HOST_SIZE
} pr_u19_row14_rgb_status;

typedef struct {
    size_t different_host_pixels;
    size_t different_source_pixels;
    size_t different_cells;
    size_t nonuniform_source_blocks;
} pr_u19_row14_rgb_difference;

/* Render the original caller's 36 source bytes using the recovered
 * uppercase-modulo-64 font lookup and EGA color 10 on black. The result is
 * the doubled row-14 crop at host x=16..591, y=224..239, row-major RGB24.
 * The caller supplies the source and decoded 1,416-byte font; neither is
 * embedded. On OUTPUT_SIZE, needed is set and crop remains unchanged. */
pr_u19_row14_rgb_status pr_u19_row14_expected_rgb24(
    const uint8_t *source, size_t source_size,
    const uint8_t *font, size_t font_size,
    uint8_t *crop, size_t capacity, size_t *needed);

/* Compare a generated crop with a caller-supplied canonical 640x400 RGB24
 * host frame. Any differing pixel in a 2x2 block marks one source pixel;
 * nonuniform_source_blocks counts blocks whose four host pixels differ among
 * themselves. No guest page, writer, or presentation timing is inferred. */
pr_u19_row14_rgb_status pr_u19_row14_compare_host_rgb24(
    const uint8_t *expected_crop, size_t crop_size,
    const uint8_t *host, size_t host_size,
    pr_u19_row14_rgb_difference *difference);

#endif
