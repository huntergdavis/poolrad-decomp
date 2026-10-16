#include "poolrad/replay/cold_start_credits_rgb.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FONT_SIZE 1416u
#define TILE_SIZE 1009u
#define ARCHIVE_SIZE (20u + FONT_SIZE + TILE_SIZE)
#define OVR_SIZE 0x700u

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t archive[ARCHIVE_SIZE];
static uint8_t ovr[OVR_SIZE];
static uint8_t output[PR_COLD_CREDITS_RGB24_BYTES];

static void word(uint8_t *p, unsigned value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}

static void make_archive(void) {
    uint8_t *font, *tile;
    memset(archive, 0, sizeof archive);
    word(archive, 18u);
    archive[2] = 201u;
    word(archive + 9u, FONT_SIZE);
    archive[11] = 202u;
    archive[12] = (uint8_t)FONT_SIZE;
    archive[13] = (uint8_t)(FONT_SIZE >> 8u);
    word(archive + 18u, TILE_SIZE);
    font = archive + 20u;
    tile = font + FONT_SIZE;
    font[1u * 8u] = 0x80u; /* UpCase('a') % 64 = 1. */
    font[2u * 8u] = 0x01u; /* UpCase('b') % 64 = 2. */
    word(tile, 8u);
    word(tile + 2u, 1u);
    tile[8] = 31u;
    memset(tile + 17u + 20u * 32u, 0x44, 32u);
    memset(tile + 17u + 21u * 32u, 0x55, 32u);
    memset(tile + 17u + 22u * 32u, 0x66, 32u);
}

static void call_setup(size_t n, uint8_t column, uint8_t row,
                       uint8_t color, unsigned source) {
    const size_t start = 0x2DAu + n * 29u;
    uint8_t *p = ovr + start;
    static const uint8_t layout[29] = {
        0xB0u, 0u, 0x50u, 0xB0u, 0u, 0x50u, 0xB0u, 0u,
        0x50u, 0x8Du, 0x7Eu, 0u, 0x16u, 0x57u, 0xBFu, 0u,
        0u, 0x0Eu, 0x57u, 0x9Au, 0x0Bu, 0x0Bu, 0xF8u,
        0x0Au, 0x9Au, 0x26u, 0x06u, 0x09u, 0x07u
    };
    memcpy(p, layout, sizeof layout);
    p[1] = column;
    p[4] = row;
    p[7] = color;
    word(p + 15u, source);
}

static void make_ovr(void) {
    size_t i;
    memset(ovr, 0, sizeof ovr);
    ovr[0x1D4u] = 1u;
    ovr[0x1D5u] = (uint8_t)'a';
    ovr[0x1E4u] = 1u;
    ovr[0x1E5u] = (uint8_t)'b';
    for (i = 0u; i < 33u; ++i)
        call_setup(i, 1u, 1u, 10u, 0x120u);
    call_setup(0u, 1u, 1u, 10u, 0x100u);
    call_setup(1u, 1u, 1u, 14u, 0x110u);
}

static int pixel(size_t x, size_t y, const uint8_t rgb[3]) {
    return memcmp(output + (y * PR_COLD_CREDITS_WIDTH + x) * 3u,
                  rgb, 3u) == 0;
}

static int test_render(void) {
    static const uint8_t black[3] = {0u, 0u, 0u};
    static const uint8_t red[3] = {170u, 0u, 0u};
    static const uint8_t magenta[3] = {170u, 0u, 170u};
    static const uint8_t brown[3] = {170u, 85u, 0u};
    static const uint8_t yellow[3] = {255u, 255u, 85u};
    size_t written = 0u;
    make_archive();
    make_ovr();
    memset(output, 0xA5, sizeof output);
    CHECK(pr_cold_credits_rgb24(archive, sizeof archive, ovr, sizeof ovr,
                                output, sizeof output, &written) ==
          PR_COLD_CREDITS_OK);
    CHECK(written == sizeof output);
    CHECK(pixel(0u, 0u, red) && pixel(1u, 1u, red));
    CHECK(pixel(16u, 0u, brown) && pixel(17u, 1u, brown));
    CHECK(pixel(0u, 16u, magenta));
    CHECK(pixel(16u, 16u, black));
    CHECK(pixel(16u, 18u, black)); /* Second glyph erases first glyph's bit. */
    CHECK(pixel(30u, 16u, yellow) && pixel(31u, 17u, yellow));
    CHECK(pixel(16u, 64u, brown)); /* Shared box border row. */
    CHECK(pixel(16u, 48u, black));
    CHECK(pixel(639u, 399u, red));
    return 0;
}

static int test_rejections(void) {
    size_t written = 0u;
    make_archive();
    make_ovr();
    output[0] = 0xA5u;
    CHECK(pr_cold_credits_rgb24(archive, sizeof archive, ovr, sizeof ovr,
                                output, sizeof output - 1u, &written) ==
          PR_COLD_CREDITS_OUTPUT_SIZE);
    CHECK(written == sizeof output && output[0] == 0xA5u);
    CHECK(pr_cold_credits_rgb24(NULL, sizeof archive, ovr, sizeof ovr,
                                output, sizeof output, &written) ==
          PR_COLD_CREDITS_ARGUMENT && written == 0u);
    CHECK(pr_cold_credits_rgb24(archive, sizeof archive, ovr, sizeof ovr,
                                output, sizeof output, NULL) ==
          PR_COLD_CREDITS_ARGUMENT);
    archive[11] = 203u;
    CHECK(pr_cold_credits_rgb24(archive, sizeof archive, ovr, sizeof ovr,
                                output, sizeof output, &written) ==
          PR_COLD_CREDITS_TILES && written == 0u);
    make_archive();
    ovr[0x2DAu + 9u] = 0x90u;
    CHECK(pr_cold_credits_rgb24(archive, sizeof archive, ovr, sizeof ovr,
                                output, sizeof output, &written) ==
          PR_COLD_CREDITS_SCRIPT && written == 0u);
    make_ovr();
    ovr[0x2DAu + 32u * 29u + 24u] = 0u;
    CHECK(pr_cold_credits_rgb24(archive, sizeof archive, ovr, sizeof ovr,
                                output, sizeof output, &written) ==
          PR_COLD_CREDITS_SCRIPT && written == 0u);
    make_ovr();
    ovr[0x2DAu + 1u] = 40u;
    CHECK(pr_cold_credits_rgb24(archive, sizeof archive, ovr, sizeof ovr,
                                output, sizeof output, &written) ==
          PR_COLD_CREDITS_SCRIPT && written == 0u);
    make_ovr();
    call_setup(0u, 1u, 1u, 10u, 0x62Bu);
    ovr[0x6FFu] = 1u;
    CHECK(pr_cold_credits_rgb24(archive, sizeof archive, ovr, sizeof ovr,
                                output, sizeof output, &written) ==
          PR_COLD_CREDITS_SCRIPT && written == 0u);
    CHECK(output[0] == 0xA5u);
    return 0;
}

int main(void) {
    if (test_render() || test_rejections()) return 1;
    puts("synthetic cold-start credits RGB24 tests passed");
    return 0;
}
