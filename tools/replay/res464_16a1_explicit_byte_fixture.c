/* One synthetic RES_0464 first-page Move byte at 0AF8:16A1.
 * Missing native hardware/plane inputs cause an unsupported stop.
 * This fixture does not provide or infer saved-route values. */
#include "poolrad/formats/ega.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PLANES 4u
#define PIXELS 8u
#define SOURCE_RAW 0x23C3u /* A200:03C3 relative to A0000 */
#define PAGE_RAW   0x03C3u /* A000:03C3 relative to A0000 */

enum {
    HAVE_GC6 = 1u << 0, HAVE_SEQ4 = 1u << 1,
    HAVE_SEQ2 = 1u << 2, HAVE_GC4 = 1u << 3,
    HAVE_GC5 = 1u << 4, HAVE_HANDLER = 1u << 5,
    HAVE_MEMORY = 1u << 6, HAVE_BANKS = 1u << 7,
    HAVE_SOURCE = 1u << 8, HAVE_PAGE = 1u << 9,
    HAVE_ALL = (1u << 10) - 1u
};

typedef struct {
    unsigned known;
    uint8_t gc6, seq4, seq2, gc4, gc5;
    bool unchained, compatible_chain4, ignore_odd_even;
    uint16_t memmask, bank_read, bank_write;
    uint8_t source[PLANES], page_before[PLANES];
} explicit_byte_input;

typedef struct {
    uint8_t plane;
    uint8_t map_bit, enabled, source_byte, before_byte, after_byte;
    uint16_t source_planar_offset, page_planar_offset;
} plane_result;

typedef struct {
    plane_result plane[PLANES];
    uint16_t source_planar_offset, page_planar_offset;
    uint8_t cpu_byte, effective_mask;
    uint8_t page_after[PLANES], bit_mask[PIXELS], pixel_index[PIXELS];
} explicit_byte_result;

static bool project_one_byte(const explicit_byte_input *input,
                             explicit_byte_result *output) {
    explicit_byte_result next = {0};
    unsigned read_plane, mask, plane, x;
    if (!input || !output || (input->known & HAVE_ALL) != HAVE_ALL ||
        (input->gc6 != 0x04u && input->gc6 != 0x06u) ||
        (input->seq4 != 0x00u && input->seq4 != 0x04u) ||
        input->gc4 >= PLANES || input->gc5 != 1u ||
        (input->seq2 & 0xF0u) != 0u ||
        !input->unchained || input->compatible_chain4 ||
        input->ignore_odd_even || input->memmask != 0xFFFFu ||
        input->bank_read != 0u || input->bank_write != 0u)
        return false;

    /* GC6 map=01 addresses A0000..AFFFF. At these low offsets,
     * GC6 bit1 either preserves bit 0 or replaces it with zero. */
    next.source_planar_offset =
        (uint16_t)(SOURCE_RAW & ((input->gc6 & 2u) ? 0x3FFEu : 0x3FFFu));
    next.page_planar_offset =
        (uint16_t)(PAGE_RAW & ((input->gc6 & 2u) ? 0x3FFEu : 0x3FFFu));
    read_plane = input->gc4;
    mask = input->seq2;
    if ((input->seq4 & 4u) == 0u) {
        read_plane = (read_plane & ~1u) + (SOURCE_RAW & 1u);
        mask &= (PAGE_RAW & 1u) ? 0x0Au : 0x05u;
    }
    next.cpu_byte = input->source[read_plane];
    next.effective_mask = (uint8_t)mask;
    for (plane = 0; plane < PLANES; ++plane) {
        plane_result *item = &next.plane[plane];
        item->plane = (uint8_t)plane;
        item->map_bit = (uint8_t)(1u << plane);
        item->enabled = (uint8_t)((mask & item->map_bit) != 0u);
        item->source_planar_offset = next.source_planar_offset;
        item->page_planar_offset = next.page_planar_offset;
        item->source_byte = input->source[plane];
        item->before_byte = input->page_before[plane];
        item->after_byte = item->enabled ? item->source_byte : item->before_byte;
        next.page_after[plane] = item->after_byte;
    }
    for (x = 0; x < PIXELS; ++x)
        next.bit_mask[x] = (uint8_t)(0x80u >> x);
    if (pr_ega_planar_byte(next.page_after, next.pixel_index) != PR_EGA_OK)
        return false;
    *output = next;
    return true;
}

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
