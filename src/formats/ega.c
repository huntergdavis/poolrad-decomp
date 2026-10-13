#include "poolrad/formats/ega.h"

#include <string.h>

/* Header and nibble order adapted from Gold Box Explorer,
 * Copyright (c) 2018 Bil Simser; MIT notice:
 * docs/formats/GoldBoxExplorer-MIT.txt. */

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int valid_block(const pr_ega_block *block) {
    size_t item_bytes;
    if (!block || !block->data || block->size < PR_EGA_HEADER_SIZE ||
        block->size > PR_EGA_MAX_BLOCK || !block->height ||
        block->height > PR_EGA_MAX_HEIGHT || !block->width_units ||
        block->width_units > PR_EGA_MAX_WIDTH / 8 || !block->item_count)
        return 0;
    item_bytes = (size_t)block->height * block->width_units * 4;
    return block->width_pixels == (size_t)block->width_units * 8 &&
           block->item_bytes == item_bytes &&
           item_bytes <= (PR_EGA_MAX_BLOCK - PR_EGA_HEADER_SIZE) / block->item_count &&
           block->size == PR_EGA_HEADER_SIZE + item_bytes * block->item_count;
}

pr_ega_status pr_ega_open(const uint8_t *data, size_t size, pr_ega_block *out) {
    pr_ega_block result = {0};
    if (!data || !out) return PR_EGA_ARGUMENT;
    if (size < PR_EGA_HEADER_SIZE || size > PR_EGA_MAX_BLOCK) return PR_EGA_SIZE;
    result.data = data;
    result.size = size;
    result.height = word(data);
    result.width_units = word(data + 2);
    result.x_raw = word(data + 4);
    result.y_raw = word(data + 6);
    result.item_count = data[8];
    memcpy(result.tail, data + 9, sizeof result.tail);
    if (!result.height || result.height > PR_EGA_MAX_HEIGHT ||
        !result.width_units || result.width_units > PR_EGA_MAX_WIDTH / 8)
        return PR_EGA_DIMENSIONS;
    if (!result.item_count) return PR_EGA_COUNT;
    result.width_pixels = (size_t)result.width_units * 8;
    result.item_bytes = (size_t)result.height * result.width_units * 4;
    if (result.item_bytes > (PR_EGA_MAX_BLOCK - PR_EGA_HEADER_SIZE) / result.item_count ||
        size != PR_EGA_HEADER_SIZE + result.item_bytes * result.item_count)
        return PR_EGA_LAYOUT;
    *out = result;
    return PR_EGA_OK;
}

pr_ega_status pr_ega_pixel(const pr_ega_block *block, size_t item,
                            size_t x, size_t y, uint8_t *index) {
    size_t source;
    uint8_t packed;
    if (!valid_block(block) || !index) return PR_EGA_ARGUMENT;
    if (item >= block->item_count) return PR_EGA_ITEM;
    if (x >= block->width_pixels || y >= block->height) return PR_EGA_PIXEL;
    source = PR_EGA_HEADER_SIZE + item * block->item_bytes +
             y * block->width_units * 4 + x / 2;
    packed = block->data[source];
    *index = (uint8_t)(x & 1 ? packed & 15 : packed >> 4);
    return PR_EGA_OK;
}

pr_ega_status pr_ega_decode_item(const pr_ega_block *block, size_t item,
                                  uint8_t *out, size_t capacity,
                                  size_t *written) {
    size_t pixels, i, start;
    if (!valid_block(block) || !written) return PR_EGA_ARGUMENT;
    if (item >= block->item_count) return PR_EGA_ITEM;
    pixels = block->width_pixels * block->height;
    *written = pixels;
    if (!out || capacity < pixels) return PR_EGA_OUTPUT_SIZE;
    start = PR_EGA_HEADER_SIZE + item * block->item_bytes;
    for (i = 0; i < pixels; ++i) {
        uint8_t packed = block->data[start + i / 2];
        out[i] = (uint8_t)(i & 1 ? packed & 15 : packed >> 4);
    }
    return PR_EGA_OK;
}

pr_ega_status pr_ega_planar_byte(const uint8_t planes[4], uint8_t indices[8]) {
    uint8_t decoded[8];
    size_t x, plane;
    if (!planes || !indices) return PR_EGA_ARGUMENT;
    for (x = 0; x < 8; ++x) {
        uint8_t index = 0;
        for (plane = 0; plane < 4; ++plane)
            index |= (uint8_t)(((planes[plane] >> (7u - x)) & 1u) << plane);
        decoded[x] = index;
    }
    memcpy(indices, decoded, sizeof decoded);
    return PR_EGA_OK;
}

const char *pr_ega_status_string(pr_ega_status status) {
    switch (status) {
    case PR_EGA_OK: return "ok";
    case PR_EGA_ARGUMENT: return "invalid argument";
    case PR_EGA_SIZE: return "block size out of range";
    case PR_EGA_DIMENSIONS: return "invalid image dimensions";
    case PR_EGA_COUNT: return "zero item count";
    case PR_EGA_LAYOUT: return "image payload size mismatch";
    case PR_EGA_ITEM: return "item outside block";
    case PR_EGA_PIXEL: return "pixel outside item";
    case PR_EGA_OUTPUT_SIZE: return "output buffer too small";
    default: return "unknown error";
    }
}
