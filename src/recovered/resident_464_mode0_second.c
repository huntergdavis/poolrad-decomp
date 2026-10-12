#include "poolrad/recovered/resident_464_mode0_second.h"

#include <string.h>

#include "poolrad/recovered/resident_464_nibble_007b.h"

static int auxiliary_present(const uint8_t *p) {
    return p[0x13] || p[0x14] || p[0x15] || p[0x16];
}

static pr_r464_mode0_second_status stop(
    pr_r464_mode0_second_report *r, pr_r464_mode0_second_stop site,
    pr_r464_mode0_second_status status) {
    r->stopped_at = site;
    return status;
}

pr_r464_mode0_second_status pr_r464_mode0_second_primary_byte(
    const pr_r464_mode0_report *prefix,
    const pr_r464_mode0_primary_report *first,
    pr_r464_block_object *object, pr_r464_block_object *auxiliary,
    const uint8_t *lookup, size_t lookup_size,
    pr_r464_mode0_second_report *report) {
    unsigned index;
    size_t dest;

    if (!prefix || !first || !object || !report ||
        prefix->stopped_at != PR_R464_MODE0_STOP_PRIMARY_HELPERS ||
        prefix->next_offset != 0x03D1u ||
        first->stopped_at != PR_R464_MODE0_PRIMARY_STOP_SECOND_GROUP ||
        first->next_offset != 0x04CEu || first->data_offset != 1u ||
        first->primary_index != 4u ||
        first->source_offset != prefix->source_offset ||
        first->auxiliary_index != prefix->auxiliary_index)
        return PR_R464_MODE0_SECOND_ARGUMENT;
    memset(report, 0, sizeof *report);
    report->source_offset = first->source_offset;
    report->data_offset = first->data_offset;
    report->auxiliary_index = first->auxiliary_index;
    dest = (size_t)report->data_offset + 0x17u;

    /* +0x04CE..+0x0584: indices 4..7 select high/low nibbles from
     * copied source bytes two and three, call +0x007B on object+9,
     * read DS +0x25FA + 4*AL + (index-4), then OR object data. */
    for (index = 4; index < 8u; ++index) {
        uint8_t packed, selector, nibble, value;
        size_t table_index;
        report->primary_index = (uint8_t)index;
        packed = prefix->copied[index >> 1];
        selector = (uint8_t)((index & 1u) ?
            (packed & 0x0Fu) : (packed >> 4));
        if (!object->bytes || object->capacity < 0x11u)
            return stop(report, PR_R464_MODE0_SECOND_STOP_HELPER_SOURCE,
                        PR_R464_MODE0_SECOND_BOUNDS);
        if (pr_r464_nibble_007b(object->bytes + 9,
                                object->capacity - 9u, selector,
                                &nibble) != PR_R464_NIBBLE_OK)
            return stop(report, PR_R464_MODE0_SECOND_STOP_HELPER_SOURCE,
                        PR_R464_MODE0_SECOND_BOUNDS);
        ++report->helper_calls;
        table_index = (size_t)nibble * 4u + (index - 4u);
        if (!lookup || table_index >= lookup_size)
            return stop(report, PR_R464_MODE0_SECOND_STOP_TABLE,
                        PR_R464_MODE0_SECOND_TABLE);
        value = lookup[table_index];
        ++report->table_reads;
        if (dest >= object->capacity || dest >= 0x10000u)
            return stop(report, PR_R464_MODE0_SECOND_STOP_OBJECT_OR,
                        PR_R464_MODE0_SECOND_BOUNDS);
        object->bytes[dest] |= value;
        ++report->primary_or_writes;
    }

    /* +0x0587..+0x05C0: reread auxiliary pointer gate and optionally
     * AND the assembled second byte with complement of aux_index-1. */
    if (auxiliary_present(object->bytes)) {
        uint16_t mask_index =
            (uint16_t)(report->auxiliary_index - 1u);
        uint8_t mask;
        if (!auxiliary || !auxiliary->bytes)
            return stop(report, PR_R464_MODE0_SECOND_STOP_AUX_READ,
                        PR_R464_MODE0_SECOND_AUX_MAPPING);
        if (mask_index >= auxiliary->capacity)
            return stop(report, PR_R464_MODE0_SECOND_STOP_AUX_READ,
                        PR_R464_MODE0_SECOND_BOUNDS);
        mask = auxiliary->bytes[mask_index];
        object->bytes[dest] &= (uint8_t)~mask;
        ++report->primary_and_writes;
    }

    /* +0x05C4/+0x05C7: both WORD offsets advance only after all
     * reached writes. The next instruction starts y/x/layer control. */
    report->data_offset = (uint16_t)(report->data_offset + 1u);
    report->source_offset = (uint16_t)(report->source_offset + 4u);
    report->next_offset = 0x05CBu;
    return stop(report, PR_R464_MODE0_SECOND_STOP_LOOP_CONTINUATION,
                PR_R464_MODE0_SECOND_READY_LOOPS);
}
