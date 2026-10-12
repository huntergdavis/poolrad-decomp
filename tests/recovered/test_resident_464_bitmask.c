#include "poolrad/recovered/resident_464_bitmask.h"

#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static int test_bit_positions(void) {
    /* Four synthetic bytes A1, 2A, AA, 03; selector A matches bits
     * 7, 4, 3 and 2 in high-then-low order. */
    CHECK(pr_r464_bitmask(0x2AA1, 0x03AA, 0x0A) == 0x9C);
    CHECK(pr_r464_bitmask(0xFFFF, 0xFFFF, 0x0F) == 0xFF);
    CHECK(pr_r464_bitmask(0x1234, 0x5678, 0x09) == 0);
    CHECK(pr_r464_bitmask(0x0000, 0x0000, 0) == 0xFF);
    return 0;
}

static int test_raw_selector(void) {
    /* The high-nibble comparison uses low8(selector << 4), while the
     * low-nibble comparison uses the unmasked raw byte. */
    CHECK(pr_r464_bitmask(0x0000, 0x0000, 0x10) == 0xAA);
    CHECK(pr_r464_bitmask(0xFFFF, 0xFFFF, 0x1F) == 0xAA);
    CHECK(pr_r464_bitmask(0xFFFF, 0xFFFF, 0xFF) == 0xAA);
    CHECK(pr_r464_bitmask(0x0A0A, 0x0A0A, 0x0A) == 0x55);
    return 0;
}

int main(void) {
    if (test_bit_positions()) return 1;
    if (test_raw_selector()) return 1;
    return 0;
}
