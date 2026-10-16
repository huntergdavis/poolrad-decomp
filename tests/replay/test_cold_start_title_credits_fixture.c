#include "poolrad/replay/cold_start_title_credits_fixture.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TITLE_BLOCK 32017u
#define TITLE_ARCHIVE (11u + TITLE_BLOCK)
#define FONT_BLOCK 1416u
#define TILES_BLOCK 1009u
#define CREDITS_ARCHIVE (20u + FONT_BLOCK + TILES_BLOCK)
#define OVR_SIZE 0x700u

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t title[TITLE_ARCHIVE];
static uint8_t credits[CREDITS_ARCHIVE];
static uint8_t ovr[OVR_SIZE];
static uint8_t sequence[PR_COLD_TITLE_CREDITS_BYTES];

static void word(uint8_t *p, unsigned value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}

static void make_title(void) {
    uint8_t *image;
    memset(title, 0, sizeof title);
    word(title, 9u);
    title[2] = 2u;
    word(title + 9u, TITLE_BLOCK);
    image = title + 11u;
    word(image, 200u);
    word(image + 2u, 40u);
    image[8] = 1u;
    image[17] = 0x12u;
}

static void make_credits(void) {
    uint8_t *font, *tile;
    memset(credits, 0, sizeof credits);
    word(credits, 18u);
    credits[2] = 201u;
    word(credits + 9u, FONT_BLOCK);
    credits[11] = 202u;
    word(credits + 12u, FONT_BLOCK);
    word(credits + 18u, TILES_BLOCK);
    font = credits + 20u;
    font[8u] = 0x80u; /* Synthetic glyph index 1, row 0. */
    tile = font + FONT_BLOCK;
    word(tile, 8u);
    word(tile + 2u, 1u);
    tile[8] = 31u;
    memset(tile + 17u + 20u * 32u, 0x44, 32u);
    memset(tile + 17u + 21u * 32u, 0x55, 32u);
    memset(tile + 17u + 22u * 32u, 0x66, 32u);
}

static void make_ovr(void) {
    static const uint8_t layout[29] = {
        0xB0u, 1u, 0x50u, 0xB0u, 1u, 0x50u, 0xB0u, 14u,
        0x50u, 0x8Du, 0x7Eu, 0u, 0x16u, 0x57u, 0xBFu,
        0x20u, 0x01u, 0x0Eu, 0x57u, 0x9Au, 0x0Bu, 0x0Bu,
        0xF8u, 0x0Au, 0x9Au, 0x26u, 0x06u, 0x09u, 0x07u
    };
    size_t i;
    memset(ovr, 0, sizeof ovr);
    ovr[0x1F4u] = 1u;
    ovr[0x1F5u] = (uint8_t)'a';
    for (i = 0u; i < 33u; ++i) {
        uint8_t *call = ovr + 0x2DAu + i * sizeof layout;
        memcpy(call, layout, sizeof layout);
        if (i != 0u) word(call + 15u, 0x130u); /* Empty Pascal string. */
    }
}

static int rgb(size_t offset, size_t x, size_t y, const uint8_t colour[3]) {
    return memcmp(sequence + offset + (y * 640u + x) * 3u,
                  colour, 3u) == 0;
}

static int unchanged(void) {
    size_t i;
    for (i = 0u; i < sizeof sequence; ++i)
        if (sequence[i] != 0xA5u) return 0;
    return 1;
}

static int test_order_and_boundary(void) {
    static const uint8_t blue[3] = {0u, 0u, 170u};
    static const uint8_t green[3] = {0u, 170u, 0u};
    static const uint8_t red[3] = {170u, 0u, 0u};
    static const uint8_t yellow[3] = {255u, 255u, 85u};
    size_t written = 0u, boundary = 0u;
    make_title();
    make_credits();
    make_ovr();
    memset(sequence, 0xA5, sizeof sequence);
    CHECK(pr_cold_title_credits_fixture(title, sizeof title,
                                        credits, sizeof credits,
                                        ovr, sizeof ovr,
                                        sequence, sizeof sequence,
                                        &written, &boundary) ==
          PR_COLD_TITLE_CREDITS_OK);
    CHECK(written == sizeof sequence && boundary == 1u);
    CHECK(rgb(0u, 0u, 0u, blue) && rgb(0u, 1u, 1u, blue));
    CHECK(rgb(0u, 2u, 0u, green));
    CHECK(rgb(PR_COLD_TITLE_CREDITS_FRAME_BYTES, 0u, 0u, red));
    CHECK(rgb(PR_COLD_TITLE_CREDITS_FRAME_BYTES, 16u, 16u, yellow));
    CHECK(!rgb(0u, 0u, 0u, red));
    return 0;
}

static int test_atomic_failures(void) {
    size_t written = 0u, boundary = 9u;
    make_title();
    make_credits();
    make_ovr();
    memset(sequence, 0xA5, sizeof sequence);
    CHECK(pr_cold_title_credits_fixture(title, sizeof title,
                                        credits, sizeof credits,
                                        ovr, sizeof ovr,
                                        sequence, sizeof sequence - 1u,
                                        &written, &boundary) ==
          PR_COLD_TITLE_CREDITS_OUTPUT_SIZE);
    CHECK(written == sizeof sequence && boundary == 0u);
    CHECK(unchanged());
    ovr[0x2DAu + 24u] = 0u;
    CHECK(pr_cold_title_credits_fixture(title, sizeof title,
                                        credits, sizeof credits,
                                        ovr, sizeof ovr,
                                        sequence, sizeof sequence,
                                        &written, &boundary) ==
          PR_COLD_TITLE_CREDITS_CREDITS);
    CHECK(written == 0u && boundary == 0u);
    CHECK(unchanged());
    make_ovr();
    title[2] = 3u;
    CHECK(pr_cold_title_credits_fixture(title, sizeof title,
                                        credits, sizeof credits,
                                        ovr, sizeof ovr,
                                        sequence, sizeof sequence,
                                        &written, &boundary) ==
          PR_COLD_TITLE_CREDITS_TITLE);
    CHECK(written == 0u && boundary == 0u && unchanged());
    return 0;
}

int main(void) {
    if (test_order_and_boundary() || test_atomic_failures()) return 1;
    puts("synthetic title-to-credits transition fixture tests passed");
    return 0;
}
