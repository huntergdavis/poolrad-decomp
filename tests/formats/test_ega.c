#include "poolrad/formats/ega.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static void sample(uint8_t *data, size_t size, unsigned height,
                   unsigned width_units, unsigned count) {
    size_t i;
    memset(data, 0, size);
    data[0] = (uint8_t)height;
    data[1] = (uint8_t)(height >> 8);
    data[2] = (uint8_t)width_units;
    data[3] = (uint8_t)(width_units >> 8);
    data[4] = 0x12;
    data[5] = 0x34;
    data[6] = 0x56;
    data[7] = 0x78;
    data[8] = (uint8_t)count;
    for (i = 0; i < 8; ++i) data[9 + i] = (uint8_t)(0xa0 + i);
}

static int test_pixels(void) {
    uint8_t data[81], pixels[64], index = 0;
    pr_ega_block block, altered;
    size_t written = 0, i;
    sample(data, sizeof data, 8, 1, 2);
    data[17] = 0xa5;
    data[21] = 0x3c; /* first byte of row one */
    data[49] = 0xe1; /* first byte of item one */
    CHECK(pr_ega_open(data, sizeof data, &block) == PR_EGA_OK);
    CHECK(block.height == 8 && block.width_units == 1 &&
          block.width_pixels == 8 && block.item_bytes == 32 &&
          block.item_count == 2 && block.x_raw == 0x3412 &&
          block.y_raw == 0x7856);
    for (i = 0; i < 8; ++i) CHECK(block.tail[i] == 0xa0 + i);
    CHECK(pr_ega_pixel(&block, 0, 0, 0, &index) == PR_EGA_OK && index == 10);
    CHECK(pr_ega_pixel(&block, 0, 1, 0, &index) == PR_EGA_OK && index == 5);
    CHECK(pr_ega_pixel(&block, 0, 0, 1, &index) == PR_EGA_OK && index == 3);
    CHECK(pr_ega_pixel(&block, 0, 1, 1, &index) == PR_EGA_OK && index == 12);
    CHECK(pr_ega_pixel(&block, 1, 0, 0, &index) == PR_EGA_OK && index == 14);
    CHECK(pr_ega_pixel(&block, 1, 1, 0, &index) == PR_EGA_OK && index == 1);
    CHECK(pr_ega_decode_item(&block, 0, NULL, 0, &written) == PR_EGA_OUTPUT_SIZE);
    CHECK(written == 64);
    CHECK(pr_ega_decode_item(&block, 0, pixels, 63, &written) == PR_EGA_OUTPUT_SIZE);
    CHECK(pr_ega_decode_item(&block, 0, pixels, sizeof pixels, &written) == PR_EGA_OK);
    CHECK(written == 64 && pixels[0] == 10 && pixels[1] == 5 &&
          pixels[8] == 3 && pixels[9] == 12);
    CHECK(pr_ega_decode_item(&block, 1, pixels, sizeof pixels, &written) == PR_EGA_OK);
    CHECK(pixels[0] == 14 && pixels[1] == 1);
    CHECK(pr_ega_pixel(&block, 2, 0, 0, &index) == PR_EGA_ITEM);
    CHECK(pr_ega_pixel(&block, 0, 8, 0, &index) == PR_EGA_PIXEL);
    CHECK(pr_ega_pixel(&block, 0, 0, 8, &index) == PR_EGA_PIXEL);
    CHECK(pr_ega_pixel(&block, 0, 0, 0, NULL) == PR_EGA_ARGUMENT);
    CHECK(pr_ega_decode_item(&block, 2, pixels, sizeof pixels, &written) == PR_EGA_ITEM);
    altered = block;
    altered.item_bytes++;
    CHECK(pr_ega_pixel(&altered, 0, 0, 0, &index) == PR_EGA_ARGUMENT);
    return 0;
}

static int test_dimensions(void) {
    static uint8_t title[32017];
    uint8_t body[2129], head[1777], sprite[305], tiles[2257];
    pr_ega_block block;
    sample(sprite, sizeof sprite, 24, 3, 1);
    CHECK(pr_ega_open(sprite, sizeof sprite, &block) == PR_EGA_OK);
    CHECK(block.width_pixels == 24 && block.height == 24);
    sample(body, sizeof body, 48, 11, 1);
    CHECK(pr_ega_open(body, sizeof body, &block) == PR_EGA_OK);
    CHECK(block.width_pixels == 88 && block.height == 48);
    sample(head, sizeof head, 40, 11, 1);
    CHECK(pr_ega_open(head, sizeof head, &block) == PR_EGA_OK);
    CHECK(block.width_pixels == 88 && block.height == 40);
    sample(tiles, sizeof tiles, 8, 1, 70);
    CHECK(pr_ega_open(tiles, sizeof tiles, &block) == PR_EGA_OK);
    CHECK(block.width_pixels == 8 && block.item_count == 70);
    sample(title, sizeof title, 200, 40, 1);
    CHECK(pr_ega_open(title, sizeof title, &block) == PR_EGA_OK);
    CHECK(block.width_pixels == 320 && block.height == 200);
    return 0;
}

static int test_malformed(void) {
    uint8_t data[82];
    pr_ega_block block;
    sample(data, 81, 8, 1, 2);
    CHECK(pr_ega_open(data, 16, &block) == PR_EGA_SIZE);
    CHECK(pr_ega_open(data, PR_EGA_MAX_BLOCK + 1, &block) == PR_EGA_SIZE);
    CHECK(pr_ega_open(NULL, 81, &block) == PR_EGA_ARGUMENT);
    CHECK(pr_ega_open(data, 81, NULL) == PR_EGA_ARGUMENT);
    CHECK(pr_ega_open(data, 80, &block) == PR_EGA_LAYOUT);
    CHECK(pr_ega_open(data, 82, &block) == PR_EGA_LAYOUT);
    data[0] = 0;
    CHECK(pr_ega_open(data, 81, &block) == PR_EGA_DIMENSIONS);
    data[0] = 8;
    data[2] = 0;
    CHECK(pr_ega_open(data, 81, &block) == PR_EGA_DIMENSIONS);
    data[2] = 41;
    CHECK(pr_ega_open(data, 81, &block) == PR_EGA_DIMENSIONS);
    data[2] = 1;
    data[0] = 201;
    CHECK(pr_ega_open(data, 81, &block) == PR_EGA_DIMENSIONS);
    data[0] = 8;
    data[8] = 0;
    CHECK(pr_ega_open(data, 81, &block) == PR_EGA_COUNT);
    return 0;
}

static int test_planar_byte(void) {
    uint8_t planes[4] = {0, 0, 0, 0};
    uint8_t pixels[8];
    static const uint8_t mixed[8] = {7, 6, 5, 4, 11, 10, 9, 8};
    size_t i;
    memset(pixels, 0xa5, sizeof pixels);
    CHECK(pr_ega_planar_byte(planes, pixels) == PR_EGA_OK);
    for (i = 0; i < 8; ++i) CHECK(pixels[i] == 0);
    planes[0] = 0x80;
    planes[1] = 0x40;
    planes[2] = 0x20;
    planes[3] = 0x10;
    CHECK(pr_ega_planar_byte(planes, pixels) == PR_EGA_OK);
    CHECK(pixels[0] == 1 && pixels[1] == 2 && pixels[2] == 4 &&
          pixels[3] == 8);
    for (i = 4; i < 8; ++i) CHECK(pixels[i] == 0);
    planes[0] = 0xaa;
    planes[1] = 0xcc;
    planes[2] = 0xf0;
    planes[3] = 0x0f;
    CHECK(pr_ega_planar_byte(planes, pixels) == PR_EGA_OK);
    CHECK(memcmp(pixels, mixed, sizeof mixed) == 0);
    memset(pixels, 0x5a, sizeof pixels);
    CHECK(pr_ega_planar_byte(NULL, pixels) == PR_EGA_ARGUMENT);
    for (i = 0; i < 8; ++i) CHECK(pixels[i] == 0x5a);
    CHECK(pr_ega_planar_byte(planes, NULL) == PR_EGA_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_pixels() || test_dimensions() || test_malformed() ||
        test_planar_byte()) return 1;
    puts("synthetic EGA image tests passed");
    return 0;
}
