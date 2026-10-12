#include "poolrad/recovered/resident_464_mode0_primary.h"

#include <string.h>

#include "poolrad/recovered/resident_464_nibble_007b.h"

static int auxiliary_present(const uint8_t *p) {
    return p[0x13] || p[0x14] || p[0x15] || p[0x16];
}

static pr_r464_mode0_primary_status stop(
    pr_r464_mode0_primary_report *r, pr_r464_mode0_primary_stop site,
    pr_r464_mode0_primary_status status) {
    r->stopped_at = site;
    return status;
}

pr_r464_mode0_primary_status pr_r464_mode0_primary_byte(
    const pr_r464_mode0_report *prefix, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *lookup,
    size_t lookup_size, pr_r464_mode0_primary_report *report) {
    unsigned index;
    size_t dest;

    if (!prefix || !object || !report ||
        prefix->stopped_at != PR_R464_MODE0_STOP_PRIMARY_HELPERS ||
        prefix->next_offset != 0x03D1u)
        return PR_R464_MODE0_PRIMARY_ARGUMENT;
    memset(report, 0, sizeof *report);
    report->source_offset = prefix->source_offset;
    report->data_offset = prefix->data_offset;
    report->auxiliary_index = prefix->auxiliary_index;
    dest = (size_t)report->data_offset + 0x17u;

    /* +0x03D1..+0x0481: four indices, high then low nibble in each
     * copied byte. The helper rereads object header +9..+0x10 each
     * time and returns one nibble in AL. The private DS lookup then
     * supplies the byte ORed into the same primary destination. */
    for (index = 0; index < 4u; ++index) {
        uint8_t packed, selector, nibble, value;
        size_t table_index;
        report->primary_index = (uint8_t)index;
        packed = prefix->copied[index >> 1];
        selector = (uint8_t)((index & 1u) ?
            (packed & 0x0Fu) : (packed >> 4));
        if (!object->bytes || object->capacity < 0x11u)
            return stop(report, PR_R464_MODE0_PRIMARY_STOP_HELPER_SOURCE,
                        PR_R464_MODE0_PRIMARY_BOUNDS);
        if (pr_r464_nibble_007b(object->bytes + 9,
                                object->capacity - 9u, selector,
                                &nibble) != PR_R464_NIBBLE_OK)
            return stop(report, PR_R464_MODE0_PRIMARY_STOP_HELPER_SOURCE,
                        PR_R464_MODE0_PRIMARY_BOUNDS);
        ++report->helper_calls;
        table_index = (size_t)nibble * 4u + index;
        if (!lookup || table_index >= lookup_size)
            return stop(report, PR_R464_MODE0_PRIMARY_STOP_TABLE,
                        PR_R464_MODE0_PRIMARY_TABLE);
        value = lookup[table_index];
        ++report->table_reads;
        if (dest >= object->capacity || dest >= 0x10000u)
            return stop(report, PR_R464_MODE0_PRIMARY_STOP_OBJECT_OR,
                        PR_R464_MODE0_PRIMARY_BOUNDS);
        object->bytes[dest] |= value;
        ++report->primary_or_writes;
    }

    /* +0x0484..+0x04BE: reread the far-pointer presence gate. If
     * present, use auxiliary_index-2 (WORD wrap) to mask the just
     * assembled primary byte. */
    report->primary_index = 4;
    if (auxiliary_present(object->bytes)) {
        uint16_t mask_index =
            (uint16_t)(report->auxiliary_index - 2u);
        uint8_t mask;
        if (!auxiliary || !auxiliary->bytes)
            return stop(report, PR_R464_MODE0_PRIMARY_STOP_AUX_READ,
                        PR_R464_MODE0_PRIMARY_AUX_MAPPING);
        if (mask_index >= auxiliary->capacity)
            return stop(report, PR_R464_MODE0_PRIMARY_STOP_AUX_READ,
                        PR_R464_MODE0_PRIMARY_BOUNDS);
        mask = auxiliary->bytes[mask_index];
        object->bytes[dest] &= (uint8_t)~mask;
        ++report->primary_and_writes;
    }

    /* +0x04C2/+0x04C5/+0x04C9: the first primary byte is complete;
     * index 4 enters the separate second group at +0x04CE. */
    report->data_offset = (uint16_t)(report->data_offset + 1u);
    report->next_offset = 0x04CEu;
    return stop(report, PR_R464_MODE0_PRIMARY_STOP_SECOND_GROUP,
                PR_R464_MODE0_PRIMARY_READY_SECOND);
}

pr_r464_mode0_primary_status pr_r464_mode0_first_primary_byte(
    const pr_r464_mode0_report *prefix, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *lookup,
    size_t lookup_size, pr_r464_mode0_primary_report *report) {
    if (!prefix || prefix->data_offset != 0u)
        return PR_R464_MODE0_PRIMARY_ARGUMENT;
    return pr_r464_mode0_primary_byte(prefix, object, auxiliary, lookup,
                                     lookup_size, report);
}
