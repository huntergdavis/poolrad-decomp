/* Portable C translation of GAME.OVR unit 0x68 internal +0x05EE.
 * See docs/re/U68_LIST_SEARCH_05EE.md for exact instruction evidence. */
#include "poolrad/recovered/overlay_u68_list_search.h"

#include "poolrad/recovered/overlay_u68_record_scan.h"

static int pr_u68_far_is_null(pr_u68_far pointer)
{
    return (pointer.offset | pointer.segment) == 0u;
}

static uint16_t pr_u68_segment_word(const uint8_t *bytes, uint16_t offset)
{
    return (uint16_t)(bytes[offset] | ((uint16_t)bytes[(unsigned)offset + 1u] << 8));
}

static const pr_u68_segment_view *pr_u68_find_segment(const pr_u68_segment_view *views,
                                                      size_t count, uint16_t selector)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (views[i].selector == selector)
            return &views[i];
    return NULL;
}

pr_u68_list_status pr_u68_list_search(pr_u68_far head,
                                      const pr_u68_segment_view *views,
                                      size_t view_count,
                                      uint16_t value_word,
                                      pr_u68_far *result,
                                      pr_u68_far *visited,
                                      size_t visited_capacity)
{
    pr_u68_far current = head;
    size_t visited_count = 0;

    if (!result)
        return PR_U68_LIST_ARGUMENT;
    if (pr_u68_far_is_null(current)) { /* +0x0601..+0x0607: OR both words */
        *result = current;
        return PR_U68_LIST_OK;
    }
    if (!views || !view_count || !visited || !visited_capacity)
        return PR_U68_LIST_ARGUMENT;

    while (!pr_u68_far_is_null(current)) {
        const pr_u68_segment_view *view;
        uint16_t scan_ax;
        size_t i;

        for (i = 0; i < visited_count; ++i)
            if (visited[i].offset == current.offset && visited[i].segment == current.segment)
                return PR_U68_LIST_CYCLE; /* host guard; DOS would keep looping */
        if (visited_count == visited_capacity)
            return PR_U68_LIST_CAPACITY;
        visited[visited_count++] = current;

        view = pr_u68_find_segment(views, view_count, current.segment);
        if (!view || !view->bytes || view->size < PR_U68_RECORD_SEGMENT_SIZE)
            return PR_U68_LIST_ARGUMENT;
        /* +0x0609..+0x0619: low byte of [bp+6], far node pointer, then
         * call +0x05AB; compare returned AL with 0xFF. */
        if (pr_u68_record_scan(view->bytes, view->size, current.offset,
                               value_word, &scan_ax) != PR_U68_RECORD_SCAN_OK)
            return PR_U68_LIST_ARGUMENT;
        if (scan_ax != PR_U68_RECORD_SCAN_MISS) {
            *result = current; /* +0x0630..+0x0645: AX:DX = far node */
            return PR_U68_LIST_OK;
        }

        /* +0x061B..+0x062E: ES:[node+0x104] / ES:[node+0x106]. The
         * effective address wraps to 16 bits; a word beginning at FFFF
         * straddles the supplied segment view and is a host guard. */
        {
            const uint16_t node_offset = current.offset;
            const uint16_t next_offset_at = (uint16_t)(node_offset + 0x104u);
            const uint16_t next_segment_at = (uint16_t)(node_offset + 0x106u);
            if (next_offset_at == 0xFFFFu || next_segment_at == 0xFFFFu)
                return PR_U68_LIST_ARGUMENT;
            current.offset = pr_u68_segment_word(view->bytes, next_offset_at);
            current.segment = pr_u68_segment_word(view->bytes, next_segment_at);
        }
    }

    *result = current; /* exhausted list: AX:DX = 0:0 */
    return PR_U68_LIST_OK;
}
