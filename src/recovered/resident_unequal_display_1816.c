#include "poolrad/recovered/resident_unequal_display_1816.h"

static int emit(const pr_rec_1816_io *io, pr_rec_1816_effect_kind kind,
                pr_rec_1816_far node, uint8_t column, uint8_t row,
                uint8_t color, uint8_t *al) {
    const pr_rec_1816_effect event = {kind, node, column, row, color};
    return !io->effect || io->effect(io->ctx, &event, al);
}

pr_rec_1816_status pr_rec_res_00ba_1816(
    const pr_rec_1816_io *io, pr_rec_1816_far node,
    uint8_t flag, uint8_t row, uint8_t column, uint8_t *al) {
    uint8_t field, color, length, called_al = 0;

    if (!io || !al) return PR_REC_1816_ARGUMENT;

    /* 181C..1842: the second field is read only when the first is nonzero. */
    if (!io->read_byte || io->read_byte(io->ctx, node, 0x10D, &field))
        return PR_REC_1816_BOUNDARY;
    if (field == 0) {
        color = 0x0C;
    } else {
        if (io->read_byte(io->ctx, node, 0x10E, &field))
            return PR_REC_1816_BOUNDARY;
        color = field == 1 ? 0x0E : 0x0B;
    }

    /* 1842..1858: first print consumes the node's Pascal string. */
    if (emit(io, PR_REC_1816_PRINT_NODE, node, column, row, color,
             &called_al)) return PR_REC_1816_BOUNDARY;

    /* 1858..188C: only the low byte of the flag is tested. The length is
     * read after the first printer, then added to the zero-extended column.
     * The printer consumes the low byte of the resulting word. */
    if (flag != 0) {
        if (io->read_byte(io->ctx, node, 0, &length))
            return PR_REC_1816_BOUNDARY;
        column = (uint8_t)((uint16_t)column + length);
        if (emit(io, PR_REC_1816_COPY_SUFFIX, node, column, row, color,
                 &called_al) ||
            emit(io, PR_REC_1816_PRINT_SUFFIX, node, column, row, color,
                 &called_al)) return PR_REC_1816_BOUNDARY;
    }

    /* 188C..1892: epilogue does not write AL; RETF 0x0A. */
    *al = called_al;
    return PR_REC_1816_OK;
}
