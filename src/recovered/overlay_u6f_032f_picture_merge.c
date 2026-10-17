#include "poolrad/recovered/overlay_u6f_032f_picture_merge.h"

static pr_u6f_032f_segment *find_segment(const pr_u6f_032f_state *state, uint16_t segment) {
    for (size_t i = 0; i < state->segment_count; ++i) {
        if (state->segments[i].segment == segment && state->segments[i].bytes != NULL) {
            return &state->segments[i];
        }
    }
    return NULL;
}

/* One byte at segment:(offset + displacement) with 16-bit wrap. */
static uint8_t *byte_at(const pr_u6f_032f_state *state, pr_u6f_032f_far base, uint32_t displacement) {
    pr_u6f_032f_segment *seg = find_segment(state, base.segment);
    uint16_t off = (uint16_t)(base.offset + displacement);
    if (seg == NULL || off >= seg->size) {
        return NULL;
    }
    return seg->bytes + off;
}

static int read_word(const pr_u6f_032f_state *state, pr_u6f_032f_far base, uint32_t displacement, uint16_t *value) {
    uint8_t *lo = byte_at(state, base, displacement);
    uint8_t *hi = byte_at(state, base, displacement + 1u);
    if (lo == NULL || hi == NULL) {
        return -1;
    }
    *value = (uint16_t)(*lo | ((uint16_t)*hi << 8));
    return 0;
}

static int read_far(const pr_u6f_032f_state *state, pr_u6f_032f_far base, uint32_t displacement, pr_u6f_032f_far *value) {
    return read_word(state, base, displacement, &value->offset) != 0 ||
           read_word(state, base, displacement + 2u, &value->segment) != 0 ? -1 : 0;
}

/* `for i := 0 to n` with the engine's 16-bit counter: n = 0xFFFF runs 65,536 times. */
static int or_loop(const pr_u6f_032f_state *state, pr_u6f_032f_far src, pr_u6f_032f_far dst, uint16_t n,
                   uint32_t *steps) {
    uint16_t i = 0u;
    for (;;) {
        uint8_t *s = byte_at(state, src, PR_U6F_032F_OFF_DATA + i);
        uint8_t *d = byte_at(state, dst, PR_U6F_032F_OFF_DATA + i);
        uint8_t v;
        if (s == NULL || d == NULL) {
            return -1;
        }
        v = *s;                                                 /* dl := src byte, then al := dst byte */
        *d = (uint8_t)(*d | v);
        (*steps)++;
        if (i == n) {
            return 0;
        }
        i++;
    }
}

pr_u6f_032f_status pr_u6f_032f_picture_merge(
    const pr_u6f_032f_state *state,
    pr_u6f_032f_far src, pr_u6f_032f_far dst,
    pr_u6f_032f_result *out) {
    pr_u6f_032f_result r;
    pr_u6f_032f_far ds;
    uint16_t n;
    r.mode = 0u; r.route = 2u; r.base.offset = 0u; r.base.segment = 0u;
    r.data_bytes = 0u; r.aux_bytes = 0u; r.src_span = 0u; r.base_span = 0u; r.next_offset = 0x032Fu;
    if (out != NULL) {
        *out = r;
    }
    if (out == NULL || state == NULL || (state->segments == NULL && state->segment_count != 0u)) {
        return PR_U6F_032F_ARGUMENT;
    }
    ds.segment = state->ds_segment;
    ds.offset = 0u;
    /* +0335: mode := DS:[0x4A1E], through the DS view */
    r.next_offset = 0x0335u;
    {
        const uint8_t *m = byte_at(state, ds, PR_U6F_032F_DS_MODE);
        if (m == NULL) { *out = r; return PR_U6F_032F_BOUNDS; }
        r.mode = *m;
    }
    /* +0338..+0340: mode 0 or 2 -> +0343; otherwise +03C1 */
    if (r.mode == 0u || r.mode == 2u) {
        pr_u6f_032f_far src_aux, dst_aux;
        uint16_t i;
        r.route = 0u;
        /* +0343..+034B: n := src.span - 1; +034E..+0353: the unsigned 0 > n test never skips */
        r.next_offset = 0x0346u;
        if (read_word(state, src, PR_U6F_032F_OFF_SPAN, &r.src_span) != 0) { *out = r; return PR_U6F_032F_BOUNDS; }
        n = (uint16_t)(r.src_span - 1u);
        /* +035D..+03BC: per offset: OR the data bytes, then AND the auxiliary bytes (the
         * auxiliary far pointers are reread from both headers at every step) */
        i = 0u;
        for (;;) {
            uint8_t *s, *d, *sa, *da, v;
            r.next_offset = 0x035Du;
            s = byte_at(state, src, PR_U6F_032F_OFF_DATA + i);
            d = byte_at(state, dst, PR_U6F_032F_OFF_DATA + i);
            if (s == NULL || d == NULL) { *out = r; return PR_U6F_032F_BOUNDS; }
            v = *s;
            *d = (uint8_t)(*d | v);
            r.data_bytes++;
            r.next_offset = 0x0385u;
            if (read_far(state, src, PR_U6F_032F_OFF_AUX, &src_aux) != 0) { *out = r; return PR_U6F_032F_BOUNDS; }
            sa = byte_at(state, src_aux, i);
            if (sa == NULL) { *out = r; return PR_U6F_032F_BOUNDS; }
            v = *sa;
            r.next_offset = 0x0394u;
            if (read_far(state, dst, PR_U6F_032F_OFF_AUX, &dst_aux) != 0) { *out = r; return PR_U6F_032F_BOUNDS; }
            da = byte_at(state, dst_aux, i);
            if (da == NULL) { *out = r; return PR_U6F_032F_BOUNDS; }
            *da = (uint8_t)(*da & v);
            r.aux_bytes++;
            /* +03B6..+03BC: i = n ends the loop */
            if (i == n) {
                break;
            }
            i++;
        }
    } else if (r.mode == 1u) {
        pr_u6f_032f_far src_aux, dst_aux;
        uint16_t m;
        r.route = 1u;
        /* +03C8..+03D0: n := src.span - 1; +03D3..+03D8 never skips */
        r.next_offset = 0x03CBu;
        if (read_word(state, src, PR_U6F_032F_OFF_SPAN, &r.src_span) != 0) { *out = r; return PR_U6F_032F_BOUNDS; }
        n = (uint16_t)(r.src_span - 1u);
        /* +03E2..+0410: OR the data bytes */
        r.next_offset = 0x03E2u;
        if (or_loop(state, src, dst, n, &r.data_bytes) != 0) { *out = r; return PR_U6F_032F_BOUNDS; }
        /* +0412..+042D: base := the far pointer DS:[0x5EB2], read here, after the OR pass (an OR
         * that aliased it changes it); m := (base.span shr 2) - 1; the unsigned test never skips */
        r.next_offset = 0x0412u;
        if (read_far(state, ds, PR_U6F_032F_DS_SLOT11_FIRST, &r.base) != 0) { *out = r; return PR_U6F_032F_BOUNDS; }
        r.next_offset = 0x0416u;
        if (read_word(state, r.base, PR_U6F_032F_OFF_SPAN, &r.base_span) != 0) { *out = r; return PR_U6F_032F_BOUNDS; }
        m = (uint16_t)((r.base_span >> 2) - 1u);
        /* +0437..+046E: AND the auxiliary bytes through the two headers' far pointers (reread each step) */
        {
            uint16_t i = 0u;
            for (;;) {
                uint8_t *sa, *da, v;
                r.next_offset = 0x0437u;
                if (read_far(state, src, PR_U6F_032F_OFF_AUX, &src_aux) != 0) { *out = r; return PR_U6F_032F_BOUNDS; }
                sa = byte_at(state, src_aux, i);
                if (sa == NULL) { *out = r; return PR_U6F_032F_BOUNDS; }
                v = *sa;
                if (read_far(state, dst, PR_U6F_032F_OFF_AUX, &dst_aux) != 0) { *out = r; return PR_U6F_032F_BOUNDS; }
                da = byte_at(state, dst_aux, i);
                if (da == NULL) { *out = r; return PR_U6F_032F_BOUNDS; }
                *da = (uint8_t)(*da & v);
                r.aux_bytes++;
                if (i == m) {
                    break;
                }
                i++;
            }
        }
    } else {
        /* +03C1..+03C5: any other mode jumps to +0470 */
        r.route = 2u;
    }
    r.next_offset = PR_U6F_032F_END_OFFSET;                     /* +0470..+0473: mov sp,bp; pop bp; retf 8 */
    *out = r;
    return PR_U6F_032F_OK;
}
