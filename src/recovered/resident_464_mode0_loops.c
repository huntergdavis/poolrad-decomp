#include "poolrad/recovered/resident_464_mode0_loops.h"

#include <string.h>

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static pr_r464_mode0_loop_status stop(
    pr_r464_mode0_loop_report *r, pr_r464_mode0_loop_stop site,
    pr_r464_mode0_loop_status status) {
    r->stopped_at = site;
    return status;
}

pr_r464_mode0_loop_status pr_r464_mode0_after_first_pixel(
    const pr_r464_mode0_report *prefix,
    const pr_r464_mode0_second_report *second,
    const pr_r464_block_object *object,
    pr_r464_mode0_loop_report *report) {
    if (!prefix || !second || !report ||
        prefix->stopped_at != PR_R464_MODE0_STOP_PRIMARY_HELPERS ||
        prefix->next_offset != 0x03D1u || prefix->count == 0u ||
        prefix->layer != 1u || prefix->x != 0u || prefix->y != 0u ||
        second->stopped_at != PR_R464_MODE0_SECOND_STOP_LOOP_CONTINUATION ||
        second->next_offset != 0x05CBu || second->data_offset != 2u ||
        second->source_offset !=
            (uint16_t)(prefix->source_offset + 4u) ||
        second->auxiliary_index != prefix->auxiliary_index)
        return PR_R464_MODE0_LOOP_ARGUMENT;

    memset(report, 0, sizeof *report);
    report->source_offset = second->source_offset;
    report->data_offset = second->data_offset;
    report->auxiliary_index = second->auxiliary_index;
    report->width_end = prefix->width_end;
    report->height_end = prefix->height_end;
    report->x = prefix->x;
    report->y = prefix->y;
    report->layer = prefix->layer;
    report->count = prefix->count;

    /* +0x05CB..+0x05D3, then +0x0325: equality ends the y loop;
     * otherwise increment the y WORD and reach the next Move. */
    if (report->y != report->height_end) {
        report->branch = PR_R464_MODE0_LOOP_BRANCH_Y;
        report->y = (uint16_t)(report->y + 1u);
        report->next_offset = 0x0328u;
        return stop(report, PR_R464_MODE0_LOOP_STOP_NEXT_MOVE,
                    PR_R464_MODE0_LOOP_NEXT_PIXEL);
    }

    /* +0x05D6..+0x05DE, then +0x0308..+0x0323: x increments
     * before the object height WORD is reread and y resets to zero. */
    if (report->x != report->width_end) {
        report->branch = PR_R464_MODE0_LOOP_BRANCH_X;
        report->x = (uint16_t)(report->x + 1u);
        if (!object || !object->bytes || object->capacity < 4u)
            return stop(report, PR_R464_MODE0_LOOP_STOP_HEIGHT_RELOAD,
                        PR_R464_MODE0_LOOP_BOUNDS);
        report->height_end = (uint16_t)(word(object->bytes + 2) - 1u);
        report->y = 0;
        report->next_offset = 0x0328u;
        return stop(report, PR_R464_MODE0_LOOP_STOP_NEXT_MOVE,
                    PR_R464_MODE0_LOOP_NEXT_PIXEL);
    }

    /* +0x05E1..+0x05E9, then +0x02EC..+0x0323: layer increments
     * before both dimension WORDs are reread in order. */
    if (report->layer != report->count) {
        report->branch = PR_R464_MODE0_LOOP_BRANCH_LAYER;
        report->layer = (uint8_t)(report->layer + 1u);
        if (!object || !object->bytes || object->capacity < 2u)
            return stop(report, PR_R464_MODE0_LOOP_STOP_WIDTH_RELOAD,
                        PR_R464_MODE0_LOOP_BOUNDS);
        report->width_end = (uint16_t)(word(object->bytes) - 1u);
        report->x = 0;
        if (object->capacity < 4u)
            return stop(report, PR_R464_MODE0_LOOP_STOP_HEIGHT_RELOAD,
                        PR_R464_MODE0_LOOP_BOUNDS);
        report->height_end = (uint16_t)(word(object->bytes + 2) - 1u);
        report->y = 0;
        report->next_offset = 0x0328u;
        return stop(report, PR_R464_MODE0_LOOP_STOP_NEXT_MOVE,
                    PR_R464_MODE0_LOOP_NEXT_PIXEL);
    }

    /* +0x05EC branches to the common +0x08CE epilogue and RETF 0x0E. */
    report->branch = PR_R464_MODE0_LOOP_BRANCH_RETURN;
    report->next_offset = 0x08CEu;
    return stop(report, PR_R464_MODE0_LOOP_STOP_COMMON_RETURN,
                PR_R464_MODE0_LOOP_RETURN);
}
