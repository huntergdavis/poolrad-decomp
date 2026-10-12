#include "poolrad/recovered/overlay_u6f_party_insert_1446.h"

#include <string.h>

static int is_null(pr_u6f_insert_far p)
{
    return (uint16_t)(p.offset | p.segment) == 0u;
}

static const pr_u6f_insert_segment *segment_for(
    const pr_u6f_insert_state *state, uint16_t segment)
{
    size_t i;
    for (i = 0u; i < state->segment_count; ++i)
        if (state->segments[i].segment == segment)
            return &state->segments[i];
    return NULL;
}

static uint8_t *at(const pr_u6f_insert_state *state,
                   pr_u6f_insert_far p, unsigned delta)
{
    const pr_u6f_insert_segment *view = segment_for(state, p.segment);
    size_t offset = (uint16_t)(p.offset + delta);
    if (!view || !view->bytes || offset >= view->size)
        return NULL;
    return view->bytes + offset;
}

static int read_word(const pr_u6f_insert_state *state,
                     pr_u6f_insert_far p, unsigned delta, uint16_t *value)
{
    uint8_t *lo = at(state, p, delta);
    uint8_t *hi = at(state, p, delta + 1u);
    if (!lo || !hi)
        return 0;
    *value = (uint16_t)((uint16_t)*lo | (uint16_t)((uint16_t)*hi << 8));
    return 1;
}

static int write_word(const pr_u6f_insert_state *state,
                      pr_u6f_insert_far p, unsigned delta, uint16_t value)
{
    uint8_t *lo = at(state, p, delta);
    uint8_t *hi = at(state, p, delta + 1u);
    if (!lo || !hi)
        return 0;
    *lo = (uint8_t)value;
    *hi = (uint8_t)(value >> 8);
    return 1;
}

pr_u6f_insert_status pr_u6f_party_insert_1446(
    pr_u6f_insert_state *state, pr_u6f_insert_far incoming,
    size_t max_nodes, pr_u6f_insert_result *out)
{
    pr_u6f_insert_result result = {0};
    pr_u6f_insert_far cursor, next;
    uint8_t used[8] = {0};
    uint8_t *slot;
    uint16_t count;
    size_t i;
    const pr_u6f_insert_segment *record_view;
    pr_u9b_record record;

    if (!out)
        return PR_U6F_INSERT_ARGUMENT;
    result.next_offset = 0x144fu;
    *out = result;
    if (!state || (state->segment_count != 0u && !state->segments) ||
        max_nodes == 0u)
        return PR_U6F_INSERT_ARGUMENT;
    for (i = 0u; i < state->segment_count; ++i) {
        size_t j;
        if (state->segments[i].size > 0x10000u)
            return PR_U6F_INSERT_ARGUMENT;
        for (j = i + 1u; j < state->segment_count; ++j)
            if (state->segments[i].segment == state->segments[j].segment)
                return PR_U6F_INSERT_ARGUMENT;
    }

    /* +0x144C..+0x144F happens before even reading the head. */
    slot = at(state, incoming, 0xbfu);
    if (!slot)
        return PR_U6F_INSERT_BOUNDS;
    *slot = 0xffu;

    if (is_null(state->head)) {
        /* +0x1455..+0x1478: empty list writes head, then active character. */
        state->head.offset = incoming.offset;
        state->head.segment = incoming.segment;
        state->active_character.offset = incoming.offset;
        state->active_character.segment = incoming.segment;
    } else {
        /* +0x147A..+0x14B9: follow +0x104/+0x106 until a null link,
         * then store incoming's offset word followed by its segment word. */
        cursor = state->head;
        for (;;) {
            result.next_offset = 0x1487u;
            if (result.append_nodes >= max_nodes) {
                *out = result;
                return PR_U6F_INSERT_TRAVERSAL_LIMIT;
            }
            ++result.append_nodes;
            result.next_offset = 0x148au;
            if (!read_word(state, cursor, 0x104u, &next.offset)) {
                *out = result;
                return PR_U6F_INSERT_BOUNDS;
            }
            result.next_offset = 0x148fu;
            if (!read_word(state, cursor, 0x106u, &next.segment)) {
                *out = result;
                return PR_U6F_INSERT_BOUNDS;
            }
            if (is_null(next))
                break;
            cursor = next;
        }
        result.next_offset = 0x14b4u;
        if (!write_word(state, cursor, 0x104u, incoming.offset)) {
            *out = result;
            return PR_U6F_INSERT_BOUNDS;
        }
        result.next_offset = 0x14b9u;
        if (!write_word(state, cursor, 0x106u, incoming.segment)) {
            *out = result;
            return PR_U6F_INSERT_BOUNDS;
        }
    }

    /* +0x14BE..+0x14CA: SYSTEM FillChar clears eight stack-local bytes.
     * +0x14CF..+0x151C: the source set at +0x1426 has members 0..7.
     * SYSTEM membership leaves ZF clear exactly for those indices. */
    cursor = state->head;
    while (!is_null(cursor)) {
        result.next_offset = 0x14dcu;
        if (result.mark_nodes >= max_nodes) {
            *out = result;
            return PR_U6F_INSERT_TRAVERSAL_LIMIT;
        }
        ++result.mark_nodes;
        result.next_offset = 0x14e7u;
        slot = at(state, cursor, 0xbfu);
        if (!slot) {
            *out = result;
            return PR_U6F_INSERT_BOUNDS;
        }
        if (*slot < 8u)
            used[*slot] = 1u;
        result.next_offset = 0x150cu;
        if (!read_word(state, cursor, 0x104u, &next.offset)) {
            *out = result;
            return PR_U6F_INSERT_BOUNDS;
        }
        result.next_offset = 0x1511u;
        if (!read_word(state, cursor, 0x106u, &next.segment)) {
            *out = result;
            return PR_U6F_INSERT_BOUNDS;
        }
        cursor = next;
    }

    /* +0x151E..+0x1563: active-character update, reset the incoming index, and
     * increment it once per occupied slot until a free index or eight. */
    state->active_character.offset = incoming.offset;
    state->active_character.segment = incoming.segment;
    result.next_offset = 0x1538u;
    slot = at(state, incoming, 0xbfu);
    if (!slot) {
        *out = result;
        return PR_U6F_INSERT_BOUNDS;
    }
    *slot = 0u;
    while (*slot < 8u && used[*slot] != 0u)
        *slot = (uint8_t)(*slot + 1u);
    result.selected_slot = *slot;
    result.has_selected_slot = 1u;

    /* +0x1565..+0x1569: one word increment at window-1 +0x67C. */
    result.next_offset = 0x1569u;
    if (!read_word(state, state->window1, 0x67cu, &count) ||
        !write_word(state, state->window1, 0x67cu,
                    (uint16_t)(count + 1u))) {
        *out = result;
        return PR_U6F_INSERT_BOUNDS;
    }
    result.incremented_window = 1u;

    /* +0x156E..+0x1574: the thunk 0xAC:0x25 selects unit 0x9B:+0x0000.
     * This +0x01AF call is a provisional post-transfer substitute. */
    result.next_offset = 0x1574u;
    record_view = segment_for(state, incoming.segment);
    if (!record_view || !record_view->bytes) {
        *out = result;
        return PR_U6F_INSERT_BOUNDS;
    }
    record.segment = record_view->bytes;
    record.segment_size = record_view->size;
    record.offset = incoming.offset;
    if (pr_u9b_record_incr(&record, &result.callee) != PR_U9B_INCR_OK) {
        *out = result;
        return PR_U6F_INSERT_CALLEE_BOUNDARY;
    }
    result.next_offset = 0x157cu;
    *out = result;
    return PR_U6F_INSERT_OK;
}
