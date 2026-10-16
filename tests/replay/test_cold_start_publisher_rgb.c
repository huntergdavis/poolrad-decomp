#include "poolrad/replay/cold_start_publisher_rgb.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 32017u
#define ARCHIVE_SIZE (20u + 2u * BLOCK_SIZE)

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t archive[ARCHIVE_SIZE];
static uint8_t output[PR_COLD_PUBLISHER_RGB24_BYTES];

static void set_word(uint8_t *p, unsigned value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void make_archive(void) {
    uint8_t *first, *second;
    memset(archive, 0, sizeof archive);
    set_word(archive, 18u);
    archive[2] = 1u;
    set_word(archive + 9u, BLOCK_SIZE);
    archive[11] = 2u;
    set_word(archive + 12u, BLOCK_SIZE);
    set_word(archive + 18u, BLOCK_SIZE);
    first = archive + 20u;
    second = first + BLOCK_SIZE;
    set_word(first, 200u);
    set_word(first + 2u, 40u);
    first[8] = 1u;
    first[17] = 0x1eu;
    memcpy(second, first, BLOCK_SIZE);
    second[17] = 0x55u;
}

static int pixel(size_t x, size_t y, const uint8_t rgb[3]) {
    const size_t offset = (y * PR_COLD_PUBLISHER_WIDTH + x) * 3u;
    return memcmp(output + offset, rgb, 3u) == 0;
}

static int test_render(void) {
    static const uint8_t blue[3] = {0, 0, 170};
    static const uint8_t yellow[3] = {255, 255, 85};
    static const uint8_t black[3] = {0, 0, 0};
    size_t written = 0;
    make_archive();
    memset(output, 0xa5, sizeof output);
    CHECK(pr_cold_publisher_rgb24(archive, sizeof archive, output,
                                  sizeof output, &written) ==
          PR_COLD_PUBLISHER_OK);
    CHECK(written == sizeof output);
    CHECK(pixel(0, 0, blue) && pixel(1, 0, blue));
    CHECK(pixel(2, 0, yellow) && pixel(3, 0, yellow));
    CHECK(pixel(0, 1, blue) && pixel(2, 1, yellow));
    CHECK(pixel(4, 0, black) && pixel(0, 2, black));
    CHECK(pixel(639, 399, black));
    return 0;
}

static int test_bounds(void) {
    size_t written = 0;
    make_archive();
    output[0] = 0xa5u;
    CHECK(pr_cold_publisher_rgb24(archive, sizeof archive, output,
                                  sizeof output - 1u, &written) ==
          PR_COLD_PUBLISHER_OUTPUT_SIZE);
    CHECK(written == sizeof output && output[0] == 0xa5u);
    CHECK(pr_cold_publisher_rgb24(archive, sizeof archive, NULL, 0u,
                                  &written) == PR_COLD_PUBLISHER_OUTPUT_SIZE);
    CHECK(pr_cold_publisher_rgb24(NULL, sizeof archive, output,
                                  sizeof output, &written) ==
          PR_COLD_PUBLISHER_ARGUMENT);
    CHECK(written == 0u);
    CHECK(pr_cold_publisher_rgb24(archive, sizeof archive, output,
                                  sizeof output, NULL) ==
          PR_COLD_PUBLISHER_ARGUMENT);
    archive[2] = 3u;
    CHECK(pr_cold_publisher_rgb24(archive, sizeof archive, output,
                                  sizeof output, &written) ==
          PR_COLD_PUBLISHER_BLOCK);
    CHECK(written == 0u);
    archive[2] = 1u;
    archive[20] = 199u;
    CHECK(pr_cold_publisher_rgb24(archive, sizeof archive, output,
                                  sizeof output, &written) ==
          PR_COLD_PUBLISHER_IMAGE);
    CHECK(written == 0u);
    archive[20] = 200u;
    archive[0] = 0u;
    CHECK(pr_cold_publisher_rgb24(archive, sizeof archive, output,
                                  sizeof output, &written) ==
          PR_COLD_PUBLISHER_ARCHIVE);
    CHECK(written == 0u);
    return 0;
}

int main(void) {
    if (test_render() || test_bounds()) return 1;
    puts("synthetic cold-start publisher RGB24 tests passed");
    return 0;
}
