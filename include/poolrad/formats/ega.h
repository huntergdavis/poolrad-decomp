#ifndef POOLRAD_FORMATS_EGA_H
#define POOLRAD_FORMATS_EGA_H

#include <stddef.h>
#include <stdint.h>

#define PR_EGA_HEADER_SIZE 17u
#define PR_EGA_MAX_BLOCK 65535u
#define PR_EGA_MAX_WIDTH 320u
#define PR_EGA_MAX_HEIGHT 200u

typedef enum {
    PR_EGA_OK = 0,
    PR_EGA_ARGUMENT,
    PR_EGA_SIZE,
    PR_EGA_DIMENSIONS,
    PR_EGA_COUNT,
    PR_EGA_LAYOUT,
    PR_EGA_ITEM,
    PR_EGA_PIXEL,
    PR_EGA_OUTPUT_SIZE
} pr_ega_status;

/* Borrows one already unpacked DAX block. The caller keeps it unchanged and
 * alive. x_raw, y_raw, and tail are preserved without assigning DOS meaning. */
typedef struct {
    const uint8_t *data;
    size_t size;
    uint16_t height;
    uint16_t width_units;
    uint16_t x_raw;
    uint16_t y_raw;
    uint8_t item_count;
    uint8_t tail[8];
    size_t width_pixels;
    size_t item_bytes;
} pr_ega_block;

pr_ega_status pr_ega_open(const uint8_t *data, size_t size, pr_ega_block *out);
/* Returns a palette index 0..15, without choosing an EGA palette. */
pr_ega_status pr_ega_pixel(const pr_ega_block *block, size_t item,
                            size_t x, size_t y, uint8_t *index);
/* Writes one byte per pixel in row-major order. On OUTPUT_SIZE, written is
 * the required capacity. Output must not overlap the borrowed block. */
pr_ega_status pr_ega_decode_item(const pr_ega_block *block, size_t item,
                                  uint8_t *out, size_t capacity,
                                  size_t *written);
const char *pr_ega_status_string(pr_ega_status status);

#endif
