#include "poolrad/recovered/resident_464_mode0_single.h"

#include <string.h>

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

pr_r464_mode0_single_status pr_r464_mode0_single_pixel(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *aux_pattern_low,
    size_t aux_pattern_size, const uint8_t *lookup, size_t lookup_size,
    pr_r464_mode0_single_report *report) {
    pr_r464_mode0_status prefix_status;
    pr_r464_mode0_primary_status first_status;
    pr_r464_mode0_second_status second_status;
    pr_r464_mode0_loop_status loop_status;

    if (!object || !report) return PR_R464_MODE0_SINGLE_ARGUMENT;
    memset(report, 0, sizeof *report);
    if (!object->bytes || object->capacity < 9u) {
        report->stage = PR_R464_MODE0_SINGLE_STAGE_PREFIX;
        prefix_status = pr_r464_mode0_first_pixel_prefix(
            source, source_size, source_offset, raw_unused, selector,
            object, auxiliary, aux_pattern_low, aux_pattern_size,
            &report->prefix);
        report->component_status = prefix_status;
        return PR_R464_MODE0_SINGLE_PREFIX_STOP;
    }

    /* Host scope gate. The DOS route itself loops for other shapes. */
    if (word(object->bytes) != 1u ||
        word(object->bytes + 2) != 1u || object->bytes[8] != 1u) {
        report->stage = PR_R464_MODE0_SINGLE_STAGE_SHAPE;
        return PR_R464_MODE0_SINGLE_SHAPE;
    }

    report->stage = PR_R464_MODE0_SINGLE_STAGE_PREFIX;
    prefix_status = pr_r464_mode0_first_pixel_prefix(
        source, source_size, source_offset, raw_unused, selector,
        object, auxiliary, aux_pattern_low, aux_pattern_size,
        &report->prefix);
    report->component_status = prefix_status;
    if (prefix_status != PR_R464_MODE0_READY_PRIMARY)
        return PR_R464_MODE0_SINGLE_PREFIX_STOP;

    report->stage = PR_R464_MODE0_SINGLE_STAGE_FIRST;
    first_status = pr_r464_mode0_first_primary_byte(
        &report->prefix, object, auxiliary, lookup, lookup_size,
        &report->first);
    report->component_status = first_status;
    if (first_status != PR_R464_MODE0_PRIMARY_READY_SECOND)
        return PR_R464_MODE0_SINGLE_FIRST_STOP;

    report->stage = PR_R464_MODE0_SINGLE_STAGE_SECOND;
    second_status = pr_r464_mode0_second_primary_byte(
        &report->prefix, &report->first, object, auxiliary,
        lookup, lookup_size, &report->second);
    report->component_status = second_status;
    if (second_status != PR_R464_MODE0_SECOND_READY_LOOPS)
        return PR_R464_MODE0_SINGLE_SECOND_STOP;

    report->stage = PR_R464_MODE0_SINGLE_STAGE_LOOP;
    loop_status = pr_r464_mode0_after_first_pixel(
        &report->prefix, &report->second, object, &report->loop);
    report->component_status = loop_status;
    if (loop_status != PR_R464_MODE0_LOOP_RETURN)
        return PR_R464_MODE0_SINGLE_LOOP_STOP;
    report->stage = PR_R464_MODE0_SINGLE_STAGE_RETURN;
    return PR_R464_MODE0_SINGLE_RETURN;
}
