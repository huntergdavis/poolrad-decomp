#include "poolrad/recovered/resident_464_mode0_two_y.h"

#include <string.h>

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

pr_r464_mode0_two_y_status pr_r464_mode0_two_y_pixels(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *aux_pattern_low,
    size_t aux_pattern_size, const uint8_t *lookup, size_t lookup_size,
    pr_r464_mode0_two_y_report *report) {
    unsigned pixel;
    pr_r464_mode0_status prefix_status;
    pr_r464_mode0_primary_status first_status;
    pr_r464_mode0_second_status second_status;
    pr_r464_mode0_loop_status loop_status;

    if (!object || !report) return PR_R464_MODE0_TWO_Y_ARGUMENT;
    memset(report, 0, sizeof *report);
    if (!object->bytes || object->capacity < 9u) {
        report->stage = PR_R464_MODE0_TWO_Y_STAGE_PREFIX;
        prefix_status = pr_r464_mode0_first_pixel_prefix(
            source, source_size, source_offset, raw_unused, selector,
            object, auxiliary, aux_pattern_low, aux_pattern_size,
            &report->prefix[0]);
        report->component_status = prefix_status;
        return PR_R464_MODE0_TWO_Y_PREFIX_STOP;
    }
    if (word(object->bytes) != 1u ||
        word(object->bytes + 2) != 2u || object->bytes[8] != 1u) {
        report->stage = PR_R464_MODE0_TWO_Y_STAGE_SHAPE;
        return PR_R464_MODE0_TWO_Y_SHAPE;
    }

    for (pixel = 0; pixel < 2u; ++pixel) {
        report->pixel = pixel;
        report->stage = PR_R464_MODE0_TWO_Y_STAGE_PREFIX;
        if (pixel == 0u) {
            prefix_status = pr_r464_mode0_first_pixel_prefix(
                source, source_size, source_offset, raw_unused, selector,
                object, auxiliary, aux_pattern_low, aux_pattern_size,
                &report->prefix[pixel]);
        } else {
            const pr_r464_mode0_loop_report *carried = &report->loop[0];
            if (carried->stopped_at != PR_R464_MODE0_LOOP_STOP_NEXT_MOVE ||
                carried->branch != PR_R464_MODE0_LOOP_BRANCH_Y ||
                carried->next_offset != 0x0328u) {
                report->stage = PR_R464_MODE0_TWO_Y_STAGE_LOOP;
                return PR_R464_MODE0_TWO_Y_LOOP_STOP;
            }
            prefix_status = pr_r464_mode0_resume_pixel_prefix(
                source, source_size, carried->source_offset,
                carried->data_offset, carried->auxiliary_index,
                carried->width_end, carried->height_end,
                carried->x, carried->y, carried->layer, carried->count,
                selector, object, auxiliary, aux_pattern_low,
                aux_pattern_size, &report->prefix[pixel]);
        }
        report->component_status = prefix_status;
        if (prefix_status != PR_R464_MODE0_READY_PRIMARY)
            return PR_R464_MODE0_TWO_Y_PREFIX_STOP;

        report->stage = PR_R464_MODE0_TWO_Y_STAGE_FIRST;
        first_status = pr_r464_mode0_primary_byte(
            &report->prefix[pixel], object, auxiliary, lookup, lookup_size,
            &report->first[pixel]);
        report->component_status = first_status;
        if (first_status != PR_R464_MODE0_PRIMARY_READY_SECOND)
            return PR_R464_MODE0_TWO_Y_FIRST_STOP;

        report->stage = PR_R464_MODE0_TWO_Y_STAGE_SECOND;
        second_status = pr_r464_mode0_secondary_byte(
            &report->prefix[pixel], &report->first[pixel], object,
            auxiliary, lookup, lookup_size, &report->second[pixel]);
        report->component_status = second_status;
        if (second_status != PR_R464_MODE0_SECOND_READY_LOOPS)
            return PR_R464_MODE0_TWO_Y_SECOND_STOP;

        report->stage = PR_R464_MODE0_TWO_Y_STAGE_LOOP;
        loop_status = pr_r464_mode0_after_pixel(
            &report->prefix[pixel], &report->second[pixel], object,
            &report->loop[pixel]);
        report->component_status = loop_status;
        if (pixel == 0u && loop_status != PR_R464_MODE0_LOOP_NEXT_PIXEL)
            return PR_R464_MODE0_TWO_Y_LOOP_STOP;
        if (pixel == 1u && loop_status != PR_R464_MODE0_LOOP_RETURN)
            return PR_R464_MODE0_TWO_Y_LOOP_STOP;
    }

    report->stage = PR_R464_MODE0_TWO_Y_STAGE_RETURN;
    return PR_R464_MODE0_TWO_Y_RETURN;
}
