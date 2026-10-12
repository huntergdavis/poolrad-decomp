#include "poolrad/recovered/resident_field_0973.h"

static int emit(const pr_rec_0973_io *io, pr_rec_0973_event_kind kind,
                pr_rec_0973_far node, uint8_t number, uint8_t x, uint8_t y,
                uint8_t style, uint8_t *al) {
    pr_rec_0973_event event = {kind, node, number, x, y, style};
    return !io->effect || io->effect(io->ctx, &event, al);
}

pr_rec_0973_status pr_rec_res_00ba_0973(const pr_rec_0973_io *io,
                                        pr_rec_0973_far node, uint8_t x,
                                        uint8_t y, uint8_t override_flag,
                                        uint8_t *al) {
    uint8_t first, threshold, formatted, style, called_al = 0;

    if (!io || !al) return PR_REC_0973_ARGUMENT;

    /* 0x0151A..0x01535: unsigned compare of the first +0x11B byte with
     * the raw +0x32 byte. The two fields have no assigned visual meaning. */
    if (!io->read_byte || io->read_byte(io->ctx, node, 0x11B, &first))
        return PR_REC_0973_BOUNDARY;
    if (!io->read_byte || io->read_byte(io->ctx, node, 0x32, &threshold))
        return PR_REC_0973_BOUNDARY;
    style = first < threshold ? 0x0E : 0x0A;

    /* 0x01535..0x0153F: nonzero low byte of the last frame word overrides
     * the earlier comparison's style. */
    if (override_flag != 0) style = 0x0D;

    /* 0x0153F..0x0155B: the formatter argument is a fresh +0x11B read
     * after the style decision, not the first byte saved in AL. */
    if (!io->read_byte || io->read_byte(io->ctx, node, 0x11B, &formatted))
        return PR_REC_0973_BOUNDARY;
    if (emit(io, PR_REC_0973_FORMAT_NUMBER, node, formatted, 0, 0, 0,
             &called_al)) return PR_REC_0973_BOUNDARY;

    /* 0x0155E..0x01568: display the formatted stack string. RETF 0x0A
     * leaves the display callee's AL as this function's return AL. */
    if (emit(io, PR_REC_0973_DISPLAY, node, 0, x, y, style,
             &called_al)) return PR_REC_0973_BOUNDARY;
    *al = called_al;
    return PR_REC_0973_OK;
}
