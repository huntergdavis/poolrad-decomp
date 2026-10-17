#include "poolrad/recovered/overlay_u3b9_entry7_0708.h"

/* Locals image: [0] = BP-0x46, so [BP+d] is locals[d + 0x46]. */
static uint16_t local_word(const pr_u3b9_0708_frame *f, size_t at) {
    return (uint16_t)(f->locals[at] | ((uint16_t)f->locals[at + 1u] << 8));
}

static void set_local_word(pr_u3b9_0708_frame *f, size_t at, uint16_t v) {
    f->locals[at] = (uint8_t)(v & 0xFFu);
    f->locals[at + 1u] = (uint8_t)(v >> 8);
}

/* di = index << 3 (shl di,cl with cl = 3); [BP+di-0x44] is locals[di + 2] and
 * [BP+di-0x42] is locals[di + 4]. Returns 0 when both words lie in the image. */
static int slot_base(const pr_u3b9_0708_frame *f, size_t *di) {
    *di = (size_t)(f->index_word & 0xFFu) << 3;
    return (*di + 5u < PR_U3B9_0708_LOCALS_SIZE) ? 0 : -1;
}

pr_u3b9_0708_status pr_u3b9_0708_show(
    pr_u3b9_0708_frame *frame, const pr_u3b9_0708_state *state,
    const uint8_t *message, size_t message_size,
    const pr_u3b9_0708_hooks *hooks, pr_u3b9_0708_result *out) {
    pr_u3b9_0708_result r = {0u, 0u, 0u, 0u, 0u, 0u, 0x0708u};
    uint8_t idx;
    int bad;
    size_t di;
    uint16_t word = 0u, x, y, s_seg, s_off;
    if (out != NULL) {
        *out = r;
    }
    if (out == NULL || frame == NULL || state == NULL || hooks == NULL || message == NULL ||
        message_size != PR_U3B9_0708_MESSAGE_LENGTH + 1u || message[0] != PR_U3B9_0708_MESSAGE_LENGTH) {
        return PR_U3B9_0708_ARGUMENT;
    }
    /* +070E..+071C: 0xAF8:0x0A3F (far [BP+6] -> SS:BP-0x42, 0x42 bytes) */
    r.next_offset = 0x071Cu;
    if (hooks->copy_record == NULL) { *out = r; return PR_U3B9_0708_NEED_HOOK; }
    if (hooks->copy_record(hooks->context, frame, frame->record_seg, frame->record_off,
                           frame->locals + PR_U3B9_0708_COPY_AT, PR_U3B9_0708_RECORD_SIZE) != 0) {
        *out = r; return PR_U3B9_0708_HOST_FAILURE;
    }
    r.copied = 1u;
    /* +0721: cmp byte [BP+0xA],1; jb +072D. +0727: cmp byte [BP+0xA],3; jbe +074E */
    r.next_offset = 0x0721u;
    idx = (uint8_t)(frame->index_word & 0xFFu);
    if (idx < PR_U3B9_0708_INDEX_MIN) {
        bad = 1;                                                /* jb +072D */
    } else {
        r.next_offset = 0x0727u;
        idx = (uint8_t)(frame->index_word & 0xFFu);             /* the second, separate read */
        bad = idx > PR_U3B9_0708_INDEX_MAX;                     /* jbe +074E otherwise */
    }
    if (bad) {
        /* +072D: resident 0x464:0x00FF */
        r.next_offset = 0x072Du;
        if (hooks->resident_464_00ff == NULL) { *out = r; return PR_U3B9_0708_NEED_HOOK; }
        if (hooks->resident_464_00ff(hooks->context, frame) != 0) { *out = r; return PR_U3B9_0708_HOST_FAILURE; }
        /* +0732..+073F: Write(DS:0x8608, CS:0x06E9 : 0) */
        r.next_offset = 0x073Fu;
        if (hooks->write_string == NULL) { *out = r; return PR_U3B9_0708_NEED_HOOK; }
        if (hooks->write_string(hooks->context, frame, PR_U3B9_0708_OUTPUT_DS, message, message_size, 0u) != 0) {
            *out = r; return PR_U3B9_0708_HOST_FAILURE;
        }
        /* +0744: Writeln(DS:0x8608) */
        r.next_offset = 0x0744u;
        if (hooks->write_line_end == NULL) { *out = r; return PR_U3B9_0708_NEED_HOOK; }
        if (hooks->write_line_end(hooks->context, frame, PR_U3B9_0708_OUTPUT_DS) != 0) { *out = r; return PR_U3B9_0708_HOST_FAILURE; }
        /* +0749: resident 0x899:0x0000; falls through to +074E if it returns */
        r.next_offset = 0x0749u;
        if (hooks->resident_899_0000 == NULL) { *out = r; return PR_U3B9_0708_NEED_HOOK; }
        if (hooks->resident_899_0000(hooks->context, frame) != 0) { *out = r; return PR_U3B9_0708_HOST_FAILURE; }
        r.error_path = 1u;
    }
    /* +074E..+075F: the slot's offset and segment words; both zero -> +07B3 -> +07BA */
    r.next_offset = 0x074Eu;
    if (slot_base(frame, &di) != 0) { *out = r; return PR_U3B9_0708_UNMODELED_FRAME; }
    if ((local_word(frame, di + 2u) | local_word(frame, di + 4u)) == 0u) {
        r.next_offset = PR_U3B9_0708_END_OFFSET;
        *out = r;
        return PR_U3B9_0708_OK;
    }
    /* +0761..+0772: les di,[BP+di-0x44]; temporary at BP-0x46/BP-0x44 := that far pointer */
    r.next_offset = 0x0761u;
    if (slot_base(frame, &di) != 0) { *out = r; return PR_U3B9_0708_UNMODELED_FRAME; }
    {
        uint16_t p_off = local_word(frame, di + 2u);
        uint16_t p_seg = local_word(frame, di + 4u);
        set_local_word(frame, 0u, p_off);
        set_local_word(frame, 2u, p_seg);
    }
    /* +0775..+077F: x = es:[di+4] + 3 - 1 through the temporary */
    r.next_offset = 0x0778u;
    if (hooks->read_far_word == NULL) { *out = r; return PR_U3B9_0708_NEED_HOOK; }
    if (hooks->read_far_word(hooks->context, frame, local_word(frame, 2u),
                             (uint16_t)(local_word(frame, 0u) + 4u), &word) != 0) {
        *out = r; return PR_U3B9_0708_HOST_FAILURE;
    }
    x = (uint16_t)(word + 2u);
    r.x = x;
    /* +0781..+078B: y = es:[di+6] + 3 - 1 through the re-read temporary */
    r.next_offset = 0x0784u;
    if (hooks->read_far_word(hooks->context, frame, local_word(frame, 2u),
                             (uint16_t)(local_word(frame, 0u) + 6u), &word) != 0) {
        *out = r; return PR_U3B9_0708_HOST_FAILURE;
    }
    y = (uint16_t)(word + 2u);
    r.y = y;
    /* +078D..+07A1: k = AL 0, flags = AL 1 (AH carried from y); S re-read from the slot */
    r.next_offset = 0x0793u;
    if (slot_base(frame, &di) != 0) { *out = r; return PR_U3B9_0708_UNMODELED_FRAME; }
    s_seg = local_word(frame, di + 4u);     /* push [BP+di-0x42] */
    s_off = local_word(frame, di + 2u);     /* push [BP+di-0x44] */
    /* +07A4..+07AC: D = DS:[0x4A22]:[0x4A20]; resident 0x464:0x0D71 */
    r.next_offset = 0x07ACu;
    if (hooks->resident_464_0d71 == NULL) { *out = r; return PR_U3B9_0708_NEED_HOOK; }
    if (hooks->resident_464_0d71(hooks->context, frame, x, y, (uint16_t)(y & 0xFF00u),
                                 (uint16_t)((y & 0xFF00u) | 1u), s_seg, s_off,
                                 state->dest_seg, state->dest_off) != 0) {
        *out = r; return PR_U3B9_0708_HOST_FAILURE;
    }
    r.drawn = 1u;
    /* +07B1 jmp +07B5: resident 0x464:0x1B52 */
    r.next_offset = 0x07B5u;
    if (hooks->resident_464_1b52 == NULL) { *out = r; return PR_U3B9_0708_NEED_HOOK; }
    if (hooks->resident_464_1b52(hooks->context, frame) != 0) { *out = r; return PR_U3B9_0708_HOST_FAILURE; }
    r.flushed = 1u;
    r.next_offset = PR_U3B9_0708_END_OFFSET;   /* +07BA..+07BD: mov sp,bp; pop bp; retf 6 */
    *out = r;
    return PR_U3B9_0708_OK;
}
