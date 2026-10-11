#include "poolrad/recovered/resident_load_pieces_0767.h"

static int far_zero(pr_rec_0767_far p) {
    return p.offset == 0 && p.segment == 0;
}

static int far_equal(pr_rec_0767_far a, pr_rec_0767_far b) {
    return a.offset == b.offset && a.segment == b.segment;
}

static int emit(const pr_rec_0767_io *io, pr_rec_0767_event_kind kind,
                pr_rec_0767_far node, uint8_t count, uint8_t a, uint8_t b,
                uint8_t c, uint8_t d, uint8_t *al) {
    pr_rec_0767_event event = {kind, node, {a, b, c, d}, count};
    return !io->effect || io->effect(io->ctx, &event, al);
}

static uint8_t class_111(uint8_t value) {
    if (value <= 0x32 || (value >= 0x3D && value <= 0x45)) return 1;
    if (value >= 0x33 && value <= 0x3C) return 2;
    return 0;
}

static uint8_t class_11b(uint8_t value) {
    if (value <= 9) return 2;
    if (value <= 0x63) return 1;
    return 0;
}

pr_rec_0767_status pr_rec_res_00ba_0767(
    const pr_rec_0767_io *io, pr_rec_0767_far active,
    uint8_t mode_49f3, size_t max_nodes, uint8_t *al) {
    const pr_rec_0767_far none = {0, 0};
    pr_rec_0767_far node;
    uint8_t x, row = 2, value, called_al = 0;
    size_t nodes = 0;

    if (!io || !al) return PR_REC_0767_ARGUMENT;

    /* 0x0130D..0x0135F: two private Pascal literals are copied to stack
     * buffers and sent to 0x0709:0626 with low-byte display arguments. */
    x = mode_49f3 == 0 ? 1 : 0x11;
    if (emit(io, PR_REC_0767_TITLE_A, none, 3, x, row, 0x0F, 0, &called_al) ||
        emit(io, PR_REC_0767_TITLE_B, none, 3, 0x21, row, 0x0F, 0, &called_al))
        return PR_REC_0767_BOUNDARY;
    row = (uint8_t)(row + 2u);

    /* 0x01363..0x0137B: the list head is read after both title calls. */
    if (!io->read_head || io->read_head(io->ctx, &node))
        return PR_REC_0767_BOUNDARY;
    while (!far_zero(node)) {
        uint8_t kind;
        if (nodes == max_nodes) return PR_REC_0767_NODE_LIMIT;

        /* 0x0137B..0x013CB: frame call, then exactly one of the active
         * text call or the separate 0x023B6 routine. */
        if (emit(io, PR_REC_0767_FRAME, none, 4, x, row, 0x26, row,
                 &called_al)) return PR_REC_0767_BOUNDARY;
        if (far_equal(node, active)) {
            if (emit(io, PR_REC_0767_ACTIVE_TEXT, node, 3,
                     x, row, 0x0F, 0, &called_al))
                return PR_REC_0767_BOUNDARY;
        } else {
            if (emit(io, PR_REC_0767_OTHER_TEXT, node, 3,
                     x, row, 0, 0, &called_al))
                return PR_REC_0767_BOUNDARY;
        }

        /* 0x013CB..0x01415: classify node +0x111, then call 0x0148F. */
        if (!io->read_byte || io->read_byte(io->ctx, node, 0x111, &value))
            return PR_REC_0767_BOUNDARY;
        kind = class_111(value);
        if (emit(io, PR_REC_0767_FIELD_111, node, 2,
                 (uint8_t)(0x20u + kind), row, 0, 0, &called_al))
            return PR_REC_0767_BOUNDARY;

        /* 0x01418..0x01457: classify node +0x11B, then call 0x01513. */
        if (!io->read_byte || io->read_byte(io->ctx, node, 0x11B, &value))
            return PR_REC_0767_BOUNDARY;
        kind = class_11b(value);
        if (emit(io, PR_REC_0767_FIELD_11B, node, 3,
                 (uint8_t)(0x24u + kind), row, 0, 0, &called_al))
            return PR_REC_0767_BOUNDARY;

        /* 0x0145A..0x01470: increment the byte row and read the next far
         * pointer at node +0x104 after all earlier call effects. */
        row = (uint8_t)(row + 1u);
        if (!io->read_next || io->read_next(io->ctx, node, &node))
            return PR_REC_0767_BOUNDARY;
        ++nodes;
    }

    /* 0x01473..0x0148C: terminal frame; its callee supplies returned AL. */
    if (emit(io, PR_REC_0767_FRAME, none, 4, x, row, 0x26, row,
             &called_al)) return PR_REC_0767_BOUNDARY;
    *al = called_al;
    return PR_REC_0767_OK;
}
