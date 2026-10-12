#include "poolrad/recovered/resident_464_mode1.h"

#include <string.h>

#include "poolrad/recovered/resident_464_bitmask.h"

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int auxiliary_present(const uint8_t *p) {
    return p[0x13] || p[0x14] || p[0x15] || p[0x16];
}

static pr_r464_mode1_status stop(pr_r464_mode1_report *r,
                                  pr_r464_mode1_stop site,
                                  pr_r464_mode1_status status) {
    r->stopped_at = site;
    return status;
}

pr_r464_mode1_status pr_r464_postprocess_mode1(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *table,
    size_t table_size, size_t max_pixels, pr_r464_mode1_report *report) {
    uint8_t count;
    unsigned layer;

    (void)raw_unused; /* Parent body never reads DOS [BP+0x0C]. */
    if (!object || !report) return PR_R464_MODE1_ARGUMENT;
    memset(report, 0, sizeof *report);
    report->source_offset = source_offset;
    if (!object->bytes || object->capacity < 9u)
        return stop(report, PR_R464_MODE1_STOP_HEADER,
                    PR_R464_MODE1_BOUNDS);
    count = object->bytes[8];

    /* +0x0600..+0x064B: layer starts at one; width and height each
     * subtract one as WORDs, so zero dimensions underflow to 0xFFFF. */
    for (layer = 1; layer <= count; ++layer) {
        unsigned x, width_end;
        report->layer = (uint8_t)layer;
        width_end = (uint16_t)(word(object->bytes) - 1u);
        for (x = 0; x <= width_end; ++x) {
            unsigned y, height_end =
                (uint16_t)(word(object->bytes + 2) - 1u);
            for (y = 0; y <= height_end; ++y) {
                uint8_t local[4];
                uint8_t bit = 0x80u;
                unsigned nibble_index, q;
                size_t src = report->source_offset;

                report->x = (uint16_t)x;
                report->y = (uint16_t)y;
                if (report->pixels_completed >= max_pixels)
                    return stop(report, PR_R464_MODE1_STOP_LIMIT,
                                PR_R464_MODE1_LIMIT);
                /* +0x064B..+0x0663: runtime Move copies four source
                 * bytes to a local stack buffer. */
                if (!source || src + 4u > source_size ||
                    src + 4u > 0x10000u)
                    return stop(report, PR_R464_MODE1_STOP_MOVE,
                                PR_R464_MODE1_BOUNDS);
                memmove(local, source + src, sizeof local);
                ++report->move_calls;

                if (object->capacity < 0x17u)
                    return stop(report, PR_R464_MODE1_STOP_HEADER,
                                PR_R464_MODE1_BOUNDS);
                /* +0x0663..+0x0692: a present auxiliary pointer causes
                 * one +0x0000 bitmask call and byte store per pixel. */
                if (auxiliary_present(object->bytes)) {
                    uint16_t first = word(local);
                    uint16_t second = word(local + 2);
                    uint8_t mask = pr_r464_bitmask(first, second, selector);
                    uint16_t index = report->auxiliary_index;
                    if (!auxiliary || !auxiliary->bytes)
                        return stop(report, PR_R464_MODE1_STOP_AUX_WRITE,
                                    PR_R464_MODE1_AUX_MAPPING);
                    if (index >= auxiliary->capacity)
                        return stop(report, PR_R464_MODE1_STOP_AUX_WRITE,
                                    PR_R464_MODE1_BOUNDS);
                    auxiliary->bytes[index] = mask;
                    ++report->aux_writes;
                    report->auxiliary_index = (uint16_t)(index + 1u);
                }

                /* +0x0692..+0x072A: eight nibbles, high then low in
                 * each source byte. A DS table test gates OR writes to
                 * the four primary bytes at data_offset + q. */
                for (nibble_index = 0; nibble_index < 8; ++nibble_index) {
                    uint8_t value = local[nibble_index >> 1];
                    uint8_t nibble = (uint8_t)((nibble_index & 1u) ?
                        (value & 0x0Fu) : (value >> 4));
                    for (q = 0; q < 4; ++q) {
                        size_t dest;
                        if (!table || q >= table_size)
                            return stop(report, PR_R464_MODE1_STOP_TABLE,
                                        PR_R464_MODE1_TABLE);
                        if (!(table[q] & nibble)) continue;
                        dest = (size_t)report->data_offset + q + 0x17u;
                        if (dest >= object->capacity || dest >= 0x10000u)
                            return stop(report, PR_R464_MODE1_STOP_PRIMARY_OR,
                                        PR_R464_MODE1_BOUNDS);
                        object->bytes[dest] |= bit;
                        ++report->primary_or_writes;
                    }
                    bit = (uint8_t)(bit >> 1);
                }

                /* +0x072A..+0x0780: reread the auxiliary pointer gate,
                 * then AND each primary byte with complement of the
                 * just-stored auxiliary byte. */
                if (auxiliary_present(object->bytes)) {
                    uint16_t last =
                        (uint16_t)(report->auxiliary_index - 1u);
                    for (q = 0; q < 4; ++q) {
                        size_t dest =
                            (size_t)report->data_offset + q + 0x17u;
                        uint8_t mask;
                        if (!auxiliary || !auxiliary->bytes)
                            return stop(report, PR_R464_MODE1_STOP_AUX_READ,
                                        PR_R464_MODE1_AUX_MAPPING);
                        if (last >= auxiliary->capacity)
                            return stop(report, PR_R464_MODE1_STOP_AUX_READ,
                                        PR_R464_MODE1_BOUNDS);
                        mask = auxiliary->bytes[last];
                        if (dest >= object->capacity || dest >= 0x10000u)
                            return stop(report, PR_R464_MODE1_STOP_PRIMARY_AND,
                                        PR_R464_MODE1_BOUNDS);
                        object->bytes[dest] &= (uint8_t)~mask;
                        ++report->primary_and_writes;
                    }
                }

                /* +0x0780/+0x0784: WORD frame offsets advance only
                 * after all work for this source pixel. */
                report->data_offset =
                    (uint16_t)(report->data_offset + 4u);
                report->source_offset =
                    (uint16_t)(report->source_offset + 4u);
                ++report->pixels_completed;
            }
        }
    }
    return PR_R464_MODE1_OK;
}
