/* Shared local-only one-byte EGA model for the synthetic fixture and
 * capture-state bridge. Caller must supply all hardware and plane inputs. */
#ifndef RES464_16A1_EXPLICIT_BYTE_MODEL_H
#define RES464_16A1_EXPLICIT_BYTE_MODEL_H

#include "poolrad/formats/ega.h"

#include <stdbool.h>
#include <stdint.h>

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

#endif
