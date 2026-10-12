#include "poolrad/recovered/resident_464_nibble_007b.h"

#include <stdio.h>
#include <stdint.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static int test_reached_index_range(void) {
    const uint8_t source[8] = {0xA1, 0x2B, 0xC3, 0x4D,
                               0xE5, 0x6F, 0x07, 0x89};
    const uint8_t expected[16] = {0xA, 1, 2, 0xB, 0xC, 3, 4, 0xD,
                                  0xE, 5, 6, 0xF, 0, 7, 8, 9};
    unsigned i;
    uint8_t out = 0x55;
    for (i = 0; i < 16; ++i) {
        CHECK(pr_r464_nibble_007b(source, sizeof source,
                                   (uint8_t)i, &out) == PR_R464_NIBBLE_OK);
        CHECK(out == expected[i]);
    }
    return 0;
}

static int test_host_bounds(void) {
    const uint8_t source[8] = {0};
    uint8_t out = 0x5A;
    CHECK(pr_r464_nibble_007b(source, 7, 0, &out) == PR_R464_NIBBLE_BOUNDS);
    CHECK(out == 0x5A);
    CHECK(pr_r464_nibble_007b(source, 8, 16, &out) == PR_R464_NIBBLE_BOUNDS);
    CHECK(out == 0x5A);
    CHECK(pr_r464_nibble_007b(source, 8, 255, &out) == PR_R464_NIBBLE_BOUNDS);
    CHECK(out == 0x5A);
    CHECK(pr_r464_nibble_007b(NULL, 8, 0, &out) == PR_R464_NIBBLE_ARGUMENT);
    CHECK(pr_r464_nibble_007b(source, 8, 0, NULL) == PR_R464_NIBBLE_ARGUMENT);
    CHECK(out == 0x5A);
    return 0;
}

int main(void) {
    if (test_reached_index_range()) return 1;
    if (test_host_bounds()) return 1;
    return 0;
}
