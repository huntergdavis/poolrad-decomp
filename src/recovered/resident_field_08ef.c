#include "poolrad/recovered/resident_field_08ef.h"

static int emit(const pr_rec_08ef_io *io, pr_rec_08ef_event_kind kind,
                pr_rec_08ef_far node, uint16_t number, uint16_t limit,
                uint8_t x, uint8_t y, uint8_t style, uint8_t *al) {
    pr_rec_08ef_event event = {kind, node, number, limit, x, y, style};
    return !io->effect || io->effect(io->ctx, &event, al);
}

pr_rec_08ef_status pr_rec_res_00ba_08ef(const pr_rec_08ef_io *io,
                                        pr_rec_08ef_far node, uint8_t x,
                                        uint8_t y, uint8_t *al) {
    uint8_t first, second, called_al = 0;
    uint16_t distance;

    if (!io || !al) return PR_REC_08EF_ARGUMENT;

    /* 0x0149C..0x014B0: read +0x111 and format abs(value - 0x3C).
     * The 0x01BD7 formatter reads only the low byte of the pushed word. */
    if (!io->read_byte || io->read_byte(io->ctx, node, 0x111, &first))
        return PR_REC_08EF_BOUNDARY;
    distance = first >= 0x3C ? (uint16_t)(first - 0x3Cu)
                             : (uint16_t)(0x3Cu - first);
    if (emit(io, PR_REC_08EF_FORMAT, node, distance, 0, 0, 0, 0,
             &called_al)) return PR_REC_08EF_BOUNDARY;

    /* 0x014B3..0x014BD: capped Pascal copy to the display buffer. */
    if (emit(io, PR_REC_08EF_COPY_FIRST, node, 0, 0xFF, 0, 0, 0,
             &called_al)) return PR_REC_08EF_BOUNDARY;

    /* 0x014C2..0x014F2: this is a second read after both earlier calls.
     * A value above 0x3C copies the private literal to the first buffer,
     * appends the number from the second buffer, then copies the result
     * into the second buffer for display. */
    if (!io->read_byte || io->read_byte(io->ctx, node, 0x111, &second))
        return PR_REC_08EF_BOUNDARY;
    if (second > 0x3C) {
        if (emit(io, PR_REC_08EF_COPY_LITERAL, node, 0, 0, 0, 0, 0,
                 &called_al) ||
            emit(io, PR_REC_08EF_APPEND_NUMBER, node, 0, 0, 0, 0, 0,
                 &called_al) ||
            emit(io, PR_REC_08EF_COPY_FINAL, node, 0, 0xFF, 0, 0, 0,
                 &called_al)) return PR_REC_08EF_BOUNDARY;
    }

    /* 0x014F7..0x01512: display the second buffer at (x,y) with style
     * 0x0A. RETF 8 leaves this callee's AL as the return value. */
    if (emit(io, PR_REC_08EF_DISPLAY, node, 0, 0, x, y, 0x0A,
             &called_al)) return PR_REC_08EF_BOUNDARY;
    *al = called_al;
    return PR_REC_08EF_OK;
}
