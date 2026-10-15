/* One synthetic RES_0464 first-page Move byte at 0AF8:16A1.
 * Missing native hardware/plane inputs cause an unsupported stop.
 * This fixture does not provide or infer saved-route values. */
#include "res464_16a1_explicit_byte_model.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "explicit-byte fixture failed line %d: %s\n", \
            __LINE__, #expr); \
    return 1; \
} } while (0)

static explicit_byte_input synthetic(void) {
    explicit_byte_input input = {0};
    const uint8_t source[PLANES] = {0x80, 0x40, 0x20, 0x10};
    input.known = HAVE_ALL;
    input.gc6 = 0x04; input.seq4 = 0x04; input.seq2 = 0x0F;
    input.gc4 = 0; input.gc5 = 1;
    input.unchained = true; input.memmask = 0xFFFF;
    memcpy(input.source, source, PLANES);
    return input;
}

static int checked_case(const explicit_byte_input *input,
                        uint16_t source_offset, uint16_t page_offset,
                        uint8_t cpu, uint8_t mask,
                        const uint8_t after[PLANES],
                        const uint8_t pixels[PIXELS]) {
    const uint8_t bits[PIXELS] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
    explicit_byte_result result;
    unsigned plane;
    memset(&result, 0xEE, sizeof result);
    CHECK(project_one_byte(input, &result));
    CHECK(result.source_planar_offset == source_offset);
    CHECK(result.page_planar_offset == page_offset);
    CHECK(result.cpu_byte == cpu && result.effective_mask == mask);
    CHECK(memcmp(result.page_after, after, PLANES) == 0);
    CHECK(memcmp(result.bit_mask, bits, PIXELS) == 0);
    CHECK(memcmp(result.pixel_index, pixels, PIXELS) == 0);
    for (plane = 0; plane < PLANES; ++plane) {
        const plane_result *item = &result.plane[plane];
        CHECK(item->plane == plane && item->map_bit == (1u << plane));
        CHECK(item->enabled == ((mask & (1u << plane)) != 0u));
        CHECK(item->source_planar_offset == source_offset);
        CHECK(item->page_planar_offset == page_offset);
        CHECK(item->source_byte == input->source[plane]);
        CHECK(item->before_byte == input->page_before[plane]);
        CHECK(item->after_byte == after[plane]);
    }
    return 0;
}

static int missing_or_unsupported(void) {
    explicit_byte_input input = synthetic();
    explicit_byte_result output, before;
    const unsigned required[] = {
        HAVE_GC6, HAVE_SEQ4, HAVE_SEQ2, HAVE_GC4, HAVE_GC5,
        HAVE_HANDLER, HAVE_MEMORY, HAVE_BANKS, HAVE_SOURCE, HAVE_PAGE
    };
    unsigned i;
    memset(&output, 0xA5, sizeof output);
    before = output;
    for (i = 0; i < sizeof required / sizeof required[0]; ++i) {
        input.known = HAVE_ALL & ~required[i];
        CHECK(!project_one_byte(&input, &output));
        CHECK(memcmp(&output, &before, sizeof output) == 0);
    }
    input = synthetic(); input.gc6 = 0x08;
    CHECK(!project_one_byte(&input, &output));
    CHECK(memcmp(&output, &before, sizeof output) == 0);
    input = synthetic(); input.unchained = false;
    CHECK(!project_one_byte(&input, &output));
    CHECK(memcmp(&output, &before, sizeof output) == 0);
    return 0;
}

int main(void) {
    explicit_byte_input input = synthetic();
    const uint8_t all_after[PLANES] = {0x80,0x40,0x20,0x10};
    const uint8_t all_pixels[PIXELS] = {1,2,4,8,0,0,0,0};
    const uint8_t odd_after[PLANES] = {0xFF,0x40,0xFF,0x10};
    const uint8_t odd_pixels[PIXELS] = {5,7,5,13,5,5,5,5};
    const uint8_t partial_after[PLANES] = {0x80,0xFF,0x20,0xFF};
    const uint8_t partial_pixels[PIXELS] = {11,10,14,10,10,10,10,10};
    CHECK(!checked_case(&input,0x23C3,0x03C3,0x80,0x0F,
                        all_after,all_pixels));
    input.gc6 = 0x06;
    CHECK(!checked_case(&input,0x23C2,0x03C2,0x80,0x0F,
                        all_after,all_pixels));
    input = synthetic(); input.seq4 = 0x00;
    input.page_before[0] = 0xFF; input.page_before[2] = 0xFF;
    CHECK(!checked_case(&input,0x23C3,0x03C3,0x40,0x0A,
                        odd_after,odd_pixels));
    input.gc6 = 0x06;
    CHECK(!checked_case(&input,0x23C2,0x03C2,0x40,0x0A,
                        odd_after,odd_pixels));
    input = synthetic(); input.seq2 = 0x05;
    input.page_before[1] = 0xFF; input.page_before[3] = 0xFF;
    CHECK(!checked_case(&input,0x23C3,0x03C3,0x80,0x05,
                        partial_after,partial_pixels));
    CHECK(!missing_or_unsupported());
    puts("explicit one-byte EGA: five projections, ten missing-input and two mode stops");
    return 0;
}
