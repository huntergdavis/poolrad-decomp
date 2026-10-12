#include "poolrad/recovered/resident_464_mode0_iterator.h"

#include <string.h>

pr_r464_mode0_iter_status pr_r464_mode0_iterate(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *aux_pattern_low,
    size_t aux_pattern_size, const uint8_t *lookup, size_t lookup_size,
    size_t pixel_budget, pr_r464_mode0_pixel_report *pixels,
    size_t pixels_capacity, pr_r464_mode0_iter_report *report) {
    size_t index;

    if (!object || !report || pixels_capacity < pixel_budget ||
        (pixel_budget && !pixels))
        return PR_R464_MODE0_ITER_ARGUMENT;
    memset(report, 0, sizeof *report);
    if (pixel_budget == 0u) {
        report->stage = PR_R464_MODE0_ITER_STAGE_BUDGET;
        report->next_offset = 0x02D3u;
        return PR_R464_MODE0_ITER_BUDGET;
    }

    for (index = 0; index < pixel_budget; ++index) {
        pr_r464_mode0_pixel_report *pixel = &pixels[index];
        pr_r464_mode0_status prefix_status;
        pr_r464_mode0_primary_status first_status;
        pr_r464_mode0_second_status second_status;
        pr_r464_mode0_loop_status loop_status;

        memset(pixel, 0, sizeof *pixel);
        report->pixel_index = index;
        report->records_used = index + 1u;
        report->stage = PR_R464_MODE0_ITER_STAGE_PREFIX;
        if (index == 0u) {
            prefix_status = pr_r464_mode0_first_pixel_prefix(
                source, source_size, source_offset, raw_unused, selector,
                object, auxiliary, aux_pattern_low, aux_pattern_size,
                &pixel->prefix);
        } else {
            const pr_r464_mode0_loop_report *carried = &pixels[index - 1u].loop;
            /* The reviewed gate is the only route to another Move. */
            if (carried->stopped_at != PR_R464_MODE0_LOOP_STOP_NEXT_MOVE ||
                carried->next_offset != 0x0328u) {
                report->stage = PR_R464_MODE0_ITER_STAGE_LOOP;
                report->component_status = PR_R464_MODE0_LOOP_ARGUMENT;
                return PR_R464_MODE0_ITER_LOOP_STOP;
            }
            prefix_status = pr_r464_mode0_resume_pixel_prefix(
                source, source_size, carried->source_offset,
                carried->data_offset, carried->auxiliary_index,
                carried->width_end, carried->height_end, carried->x,
                carried->y, carried->layer, carried->count, selector,
                object, auxiliary, aux_pattern_low, aux_pattern_size,
                &pixel->prefix);
        }
        report->component_status = prefix_status;
        if (index == 0u && prefix_status == PR_R464_MODE0_NO_WORK) {
            report->stage = PR_R464_MODE0_ITER_STAGE_RETURN;
            report->next_offset = pixel->prefix.next_offset;
            return PR_R464_MODE0_ITER_NO_WORK;
        }
        if (prefix_status != PR_R464_MODE0_READY_PRIMARY)
            return PR_R464_MODE0_ITER_PREFIX_STOP;

        report->stage = PR_R464_MODE0_ITER_STAGE_FIRST;
        first_status = pr_r464_mode0_primary_byte(
            &pixel->prefix, object, auxiliary, lookup, lookup_size,
            &pixel->first);
        report->component_status = first_status;
        if (first_status != PR_R464_MODE0_PRIMARY_READY_SECOND)
            return PR_R464_MODE0_ITER_FIRST_STOP;

        report->stage = PR_R464_MODE0_ITER_STAGE_SECOND;
        second_status = pr_r464_mode0_secondary_byte(
            &pixel->prefix, &pixel->first, object, auxiliary,
            lookup, lookup_size, &pixel->second);
        report->component_status = second_status;
        if (second_status != PR_R464_MODE0_SECOND_READY_LOOPS)
            return PR_R464_MODE0_ITER_SECOND_STOP;

        report->stage = PR_R464_MODE0_ITER_STAGE_LOOP;
        loop_status = pr_r464_mode0_after_pixel(
            &pixel->prefix, &pixel->second, object, &pixel->loop);
        report->component_status = loop_status;
        report->completed_pixels = index + 1u;
        if (loop_status == PR_R464_MODE0_LOOP_RETURN) {
            report->stage = PR_R464_MODE0_ITER_STAGE_RETURN;
            report->next_offset = pixel->loop.next_offset;
            return PR_R464_MODE0_ITER_RETURN;
        }
        if (loop_status != PR_R464_MODE0_LOOP_NEXT_PIXEL)
            return PR_R464_MODE0_ITER_LOOP_STOP;
        report->next_offset = pixel->loop.next_offset;
    }

    report->stage = PR_R464_MODE0_ITER_STAGE_BUDGET;
    return PR_R464_MODE0_ITER_BUDGET;
}
