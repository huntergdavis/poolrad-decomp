#include "poolrad/recovered/resident_464_mode0_prefix.h"

#include <string.h>

#include "poolrad/recovered/resident_464_bitmask.h"

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int auxiliary_present(const uint8_t *p) {
    return p[0x13] || p[0x14] || p[0x15] || p[0x16];
}

static pr_r464_mode0_status stop(pr_r464_mode0_report *r,
                                  pr_r464_mode0_stop site,
                                  pr_r464_mode0_status status) {
    r->stopped_at = site;
    return status;
}

static pr_r464_mode0_status run_pixel_prefix(
    const uint8_t *source, size_t source_size, uint8_t selector,
    pr_r464_block_object *object, pr_r464_block_object *auxiliary,
    const uint8_t *aux_pattern_low, size_t aux_pattern_size,
    pr_r464_mode0_report *report) {
    unsigned bit_index;
    uint8_t bit;

    /* +0x0328..+0x033B: runtime Move into four local bytes. */
    if (!source || (size_t)report->source_offset + 4u > source_size ||
        (size_t)report->source_offset + 4u > 0x10000u)
        return stop(report, PR_R464_MODE0_STOP_MOVE,
                    PR_R464_MODE0_BOUNDS);
    memmove(report->copied, source + report->source_offset,
            sizeof report->copied);
    ++report->move_calls;

    /* +0x0340..+0x03C6: a nonzero auxiliary far pointer invokes the
     * complete +0x0000 bitmask helper, then tests eight mask bits.
     * Each group of four ORs a distinct auxiliary byte before its index
     * advances. A zero mask still advances the index twice. */
    if (!object->bytes || object->capacity < 0x17u)
        return stop(report, PR_R464_MODE0_STOP_HEADER,
                    PR_R464_MODE0_BOUNDS);
    if (auxiliary_present(object->bytes)) {
        uint16_t first = word(report->copied);
        uint16_t second = word(report->copied + 2);
        report->bitmask = pr_r464_bitmask(first, second, selector);
        ++report->bitmask_calls;
        bit = 0x80u;
        for (bit_index = 0; bit_index < 8; ++bit_index) {
            unsigned table_index = bit_index & 3u;
            if (report->bitmask & bit) {
                uint16_t index = report->auxiliary_index;
                uint8_t value;
                if (!aux_pattern_low || table_index >= aux_pattern_size)
                    return stop(report, PR_R464_MODE0_STOP_AUX_TABLE,
                                PR_R464_MODE0_TABLE);
                value = aux_pattern_low[table_index];
                ++report->table_reads;
                if (!auxiliary || !auxiliary->bytes)
                    return stop(report, PR_R464_MODE0_STOP_AUX_READ,
                                PR_R464_MODE0_AUX_MAPPING);
                if (index >= auxiliary->capacity)
                    return stop(report, PR_R464_MODE0_STOP_AUX_READ,
                                PR_R464_MODE0_BOUNDS);
                value = (uint8_t)(auxiliary->bytes[index] | value);
                auxiliary->bytes[index] = value;
                ++report->aux_or_writes;
            }
            bit = (uint8_t)(bit >> 1);
            if (table_index == 3u)
                report->auxiliary_index =
                    (uint16_t)(report->auxiliary_index + 1u);
        }
    }

    /* +0x03C8/+0x03CC initialize the primary nibble index and jump to
     * +0x03D1. The remaining primary path is deliberately outside this
     * bounded prefix. */
    report->next_offset = 0x03D1u;
    return stop(report, PR_R464_MODE0_STOP_PRIMARY_HELPERS,
                PR_R464_MODE0_READY_PRIMARY);
}

pr_r464_mode0_status pr_r464_mode0_first_pixel_prefix(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *aux_pattern_low,
    size_t aux_pattern_size, pr_r464_mode0_report *report) {
    (void)raw_unused; /* [BP+0x0C] is never read by the parent procedure. */
    if (!object || !report) return PR_R464_MODE0_ARGUMENT;
    memset(report, 0, sizeof *report);
    report->source_offset = source_offset;
    if (!object->bytes || object->capacity < 9u)
        return stop(report, PR_R464_MODE0_STOP_HEADER,
                    PR_R464_MODE0_BOUNDS);

    /* +0x02D3..+0x02E4: zero count takes the common return. */
    report->count = object->bytes[8];
    if (report->count == 0) {
        report->next_offset = 0x08CEu;
        return stop(report, PR_R464_MODE0_STOP_RETURN,
                    PR_R464_MODE0_NO_WORK);
    }
    report->layer = 1;
    report->width_end = (uint16_t)(word(object->bytes) - 1u);
    report->height_end = (uint16_t)(word(object->bytes + 2) - 1u);
    return run_pixel_prefix(source, source_size, selector, object, auxiliary,
                            aux_pattern_low, aux_pattern_size, report);
}

pr_r464_mode0_status pr_r464_mode0_resume_pixel_prefix(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint16_t data_offset, uint16_t auxiliary_index,
    uint16_t width_end, uint16_t height_end, uint16_t x, uint16_t y,
    uint8_t layer, uint8_t count, uint8_t selector,
    pr_r464_block_object *object, pr_r464_block_object *auxiliary,
    const uint8_t *aux_pattern_low, size_t aux_pattern_size,
    pr_r464_mode0_report *report) {
    if (!object || !report || !count || !layer || layer > count)
        return PR_R464_MODE0_ARGUMENT;
    memset(report, 0, sizeof *report);
    report->source_offset = source_offset;
    report->data_offset = data_offset;
    report->auxiliary_index = auxiliary_index;
    report->width_end = width_end;
    report->height_end = height_end;
    report->x = x;
    report->y = y;
    report->layer = layer;
    report->count = count;
    return run_pixel_prefix(source, source_size, selector, object, auxiliary,
                            aux_pattern_low, aux_pattern_size, report);
}
