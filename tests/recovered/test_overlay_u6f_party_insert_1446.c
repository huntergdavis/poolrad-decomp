#include "poolrad/recovered/overlay_u6f_party_insert_1446.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define RECORD_SEG 0x1111u
#define WINDOW_SEG 0x2222u

static uint8_t records[0x10000];
static uint8_t window[0x10000];

static pr_u6f_insert_far far_at(uint16_t offset, uint16_t segment)
{
    pr_u6f_insert_far p = {offset, segment};
    return p;
}

static size_t eff(uint16_t base, unsigned delta)
{
    return (uint16_t)(base + delta);
}

static void put16(uint8_t *segment, uint16_t base,
                  unsigned delta, uint16_t value)
{
    segment[eff(base, delta)] = (uint8_t)value;
    segment[eff(base, delta + 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *segment, uint16_t base, unsigned delta)
{
    return (uint16_t)((uint16_t)segment[eff(base, delta)] |
                      (uint16_t)((uint16_t)segment[eff(base, delta + 1u)] << 8));
}

static void link_to(uint16_t from, pr_u6f_insert_far to)
{
    put16(records, from, 0x104u, to.offset);
    put16(records, from, 0x106u, to.segment);
}

static void reset_memory(void)
{
    memset(records, 0, sizeof records);
    memset(window, 0, sizeof window);
}

static pr_u6f_insert_state state_with(pr_u6f_insert_segment *views)
{
    pr_u6f_insert_state state = {0};
    views[0].segment = RECORD_SEG;
    views[0].bytes = records;
    views[0].size = sizeof records;
    views[1].segment = WINDOW_SEG;
    views[1].bytes = window;
    views[1].size = sizeof window;
    state.window1 = far_at(0u, WINDOW_SEG);
    state.segments = views;
    state.segment_count = 2u;
    return state;
}

static void test_empty_and_final_callee(void)
{
    pr_u6f_insert_segment views[2];
    pr_u6f_insert_state state;
    pr_u6f_insert_result out;
    pr_u6f_insert_far incoming = far_at(0x100u, RECORD_SEG);
    reset_memory();
    state = state_with(views);
    records[eff(incoming.offset, 0x12u)] = 0x11u;
    records[eff(incoming.offset, 0xb2u)] = 1u;
    records[eff(incoming.offset, 0xb3u)] = 2u;
    records[eff(incoming.offset, 0xb4u)] = 3u;
    put16(window, 0u, 0x67cu, 0xffffu);

    assert(pr_u6f_party_insert_1446(&state, incoming, 1u, &out) ==
           PR_U6F_INSERT_OK);
    assert(state.head.offset == incoming.offset &&
           state.head.segment == incoming.segment);
    assert(state.tail.offset == incoming.offset &&
           state.tail.segment == incoming.segment);
    assert(records[eff(incoming.offset, 0xbfu)] == 0u);
    assert(out.selected_slot == 0u && out.has_selected_slot);
    assert(out.append_nodes == 0u && out.mark_nodes == 1u);
    assert(out.incremented_window && out.next_offset == 0x157cu);
    assert(get16(window, 0u, 0x67cu) == 0u);
    assert(records[eff(incoming.offset, 0xb2u)] == 3u);
    assert(records[eff(incoming.offset, 0xb3u)] == 4u);
    assert(records[eff(incoming.offset, 0xb4u)] == 4u);
    assert(out.callee.increments == 5u && out.callee.steps_taken == 0x1fu);
}

static void test_append_and_gap(void)
{
    pr_u6f_insert_segment views[2];
    pr_u6f_insert_state state;
    pr_u6f_insert_result out;
    pr_u6f_insert_far incoming = far_at(0x500u, RECORD_SEG);
    reset_memory();
    state = state_with(views);
    state.head = far_at(0x100u, RECORD_SEG);
    state.tail = far_at(0x300u, RECORD_SEG);
    records[eff(0x100u, 0xbfu)] = 0u;
    records[eff(0x300u, 0xbfu)] = 2u;
    link_to(0x100u, state.tail);
    put16(window, 0u, 0x67cu, 3u);

    assert(pr_u6f_party_insert_1446(&state, incoming, 3u, &out) ==
           PR_U6F_INSERT_OK);
    assert(out.append_nodes == 2u && out.mark_nodes == 3u);
    assert(get16(records, 0x300u, 0x104u) == incoming.offset);
    assert(get16(records, 0x300u, 0x106u) == incoming.segment);
    assert(state.tail.offset == incoming.offset &&
           state.tail.segment == incoming.segment);
    assert(records[eff(incoming.offset, 0xbfu)] == 1u);
    assert(out.selected_slot == 1u && get16(window, 0u, 0x67cu) == 4u);
    assert(out.callee.increments == 0u);
}

static void test_all_slot_counts(void)
{
    pr_u6f_insert_segment views[2];
    pr_u6f_insert_state state;
    pr_u6f_insert_result out;
    size_t n, i;
    for (n = 0u; n <= 8u; ++n) {
        pr_u6f_insert_far incoming = far_at((uint16_t)(0x1100u + n * 0x200u),
                                             RECORD_SEG);
        reset_memory();
        state = state_with(views);
        if (n != 0u) {
            state.head = far_at(0x100u, RECORD_SEG);
            state.tail = far_at((uint16_t)(0x100u + (n - 1u) * 0x200u),
                                RECORD_SEG);
        }
        for (i = 0u; i < n; ++i) {
            uint16_t offset = (uint16_t)(0x100u + i * 0x200u);
            records[eff(offset, 0xbfu)] = (uint8_t)i;
            if (i + 1u < n)
                link_to(offset, far_at((uint16_t)(offset + 0x200u), RECORD_SEG));
        }
        assert(pr_u6f_party_insert_1446(&state, incoming, n + 1u, &out) ==
               PR_U6F_INSERT_OK);
        assert(out.selected_slot == n && out.has_selected_slot);
        assert(records[eff(incoming.offset, 0xbfu)] == n);
        assert(out.mark_nodes == n + 1u && out.append_nodes == n);
    }
}

static void test_cycle_guard_preserves_first_write(void)
{
    pr_u6f_insert_segment views[2];
    pr_u6f_insert_state state;
    pr_u6f_insert_result out;
    pr_u6f_insert_far incoming = far_at(0x500u, RECORD_SEG);
    reset_memory();
    state = state_with(views);
    state.head = far_at(0x100u, RECORD_SEG);
    state.tail = far_at(0x300u, RECORD_SEG);
    link_to(0x100u, state.tail);
    link_to(0x300u, state.head);
    put16(window, 0u, 0x67cu, 9u);
    assert(pr_u6f_party_insert_1446(&state, incoming, 2u, &out) ==
           PR_U6F_INSERT_TRAVERSAL_LIMIT);
    assert(out.next_offset == 0x1487u && out.append_nodes == 2u);
    assert(records[eff(incoming.offset, 0xbfu)] == 0xffu);
    assert(get16(records, 0x300u, 0x104u) == state.head.offset);
    assert(state.tail.offset == 0x300u && get16(window, 0u, 0x67cu) == 9u);
}

static void test_missing_reached_views(void)
{
    pr_u6f_insert_segment views[2];
    pr_u6f_insert_state state;
    pr_u6f_insert_result out;
    pr_u6f_insert_far incoming = far_at(0x100u, RECORD_SEG);
    reset_memory();
    state = state_with(views);
    state.segment_count = 1u;
    put16(window, 0u, 0x67cu, 7u);
    assert(pr_u6f_party_insert_1446(&state, incoming, 1u, &out) ==
           PR_U6F_INSERT_BOUNDS);
    assert(out.next_offset == 0x1569u && out.selected_slot == 0u);
    assert(out.has_selected_slot && !out.incremented_window);
    assert(state.head.offset == incoming.offset && state.tail.offset == incoming.offset);
    assert(records[eff(incoming.offset, 0xbfu)] == 0u);
    assert(get16(window, 0u, 0x67cu) == 7u);

    reset_memory();
    state = state_with(views);
    state.head = far_at(0x100u, 0x3333u);
    state.tail = state.head;
    assert(pr_u6f_party_insert_1446(&state, incoming, 2u, &out) ==
           PR_U6F_INSERT_BOUNDS);
    assert(out.next_offset == 0x148au && out.append_nodes == 1u);
    assert(records[eff(incoming.offset, 0xbfu)] == 0xffu);
    assert(state.tail.segment == 0x3333u);

    state = state_with(views);
    incoming.segment = 0x4444u;
    assert(pr_u6f_party_insert_1446(&state, incoming, 1u, &out) ==
           PR_U6F_INSERT_BOUNDS);
    assert(out.next_offset == 0x144fu && out.append_nodes == 0u);
}

static void test_mark_limit_retains_append_link(void)
{
    pr_u6f_insert_segment views[2];
    pr_u6f_insert_state state;
    pr_u6f_insert_result out;
    pr_u6f_insert_far incoming = far_at(0x500u, RECORD_SEG);
    reset_memory();
    state = state_with(views);
    state.head = far_at(0x100u, RECORD_SEG);
    state.tail = far_at(0x300u, RECORD_SEG);
    link_to(0x100u, state.tail);
    assert(pr_u6f_party_insert_1446(&state, incoming, 2u, &out) ==
           PR_U6F_INSERT_TRAVERSAL_LIMIT);
    assert(out.next_offset == 0x14dcu);
    assert(out.append_nodes == 2u && out.mark_nodes == 2u);
    assert(get16(records, 0x300u, 0x104u) == incoming.offset);
    assert(get16(records, 0x300u, 0x106u) == incoming.segment);
    assert(state.tail.offset == 0x300u);
    assert(records[eff(incoming.offset, 0xbfu)] == 0xffu);
    assert(get16(window, 0u, 0x67cu) == 0u);
}

static void test_final_callee_partial_boundary(void)
{
    pr_u6f_insert_segment views[2];
    pr_u6f_insert_state state;
    pr_u6f_insert_result out;
    pr_u6f_insert_far incoming = far_at(0xff4cu, RECORD_SEG);
    reset_memory();
    state = state_with(views);
    views[0].size = 0xffffu; /* +0xB3 wraps to the one missing byte. */
    records[eff(incoming.offset, 0x12u)] = 0x11u;
    records[eff(incoming.offset, 0xb2u)] = 1u;
    assert(pr_u6f_party_insert_1446(&state, incoming, 1u, &out) ==
           PR_U6F_INSERT_CALLEE_BOUNDARY);
    assert(out.next_offset == 0x1574u && out.incremented_window);
    assert(out.selected_slot == 0u && get16(window, 0u, 0x67cu) == 1u);
    assert(out.callee.increments == 2u && out.callee.steps_taken == 3u);
    assert(records[eff(incoming.offset, 0xb2u)] == 3u);
}

int main(void)
{
    test_empty_and_final_callee();
    test_append_and_gap();
    test_all_slot_counts();
    test_cycle_guard_preserves_first_write();
    test_missing_reached_views();
    test_mark_limit_retains_append_link();
    test_final_callee_partial_boundary();
    return 0;
}
