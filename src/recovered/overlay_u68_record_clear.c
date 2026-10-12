/* Portable C translation of GAME.OVR unit 0x68 internal +0x0648.
 * See docs/re/U68_RECORD_CLEAR_0648.md for instruction evidence. */
#include "poolrad/recovered/overlay_u68_record_clear.h"

#include "poolrad/recovered/overlay_u68_record_scan.h"

pr_u68_clear_status pr_u68_record_clear(pr_u68_far head,
                                        const pr_u68_mutable_segment_view *views,
                                        size_t view_count,
                                        pr_u68_segment_view *read_views,
                                        size_t read_views_capacity,
                                        pr_u68_far *visited,
                                        size_t visited_capacity,
                                        uint16_t *ax_out)
{
    pr_u68_far found;
    pr_u68_list_status status;
    size_t i;

    if (!ax_out)
        return PR_U68_CLEAR_ARGUMENT;
    if ((head.offset | head.segment) != 0u) {
        if (!views || !read_views || !view_count || read_views_capacity < view_count)
            return PR_U68_CLEAR_ARGUMENT;
        for (i = 0; i < view_count; ++i) {
            read_views[i].selector = views[i].selector;
            read_views[i].bytes = views[i].bytes;
            read_views[i].size = views[i].size;
        }
    }

    /* +0x064E..+0x0652: the argument's low byte is 0x1F. */
    status = pr_u68_list_search(head, read_views, view_count, 0x001Fu,
                                &found, visited, visited_capacity);
    if (status == PR_U68_LIST_CYCLE)
        return PR_U68_CLEAR_CYCLE;
    if (status == PR_U68_LIST_CAPACITY)
        return PR_U68_CLEAR_CAPACITY;
    if (status != PR_U68_LIST_OK)
        return PR_U68_CLEAR_ARGUMENT;

    /* +0x065B..+0x0661: only 0:0 takes the no-write return path. */
    if ((found.offset | found.segment) == 0u) {
        *ax_out = 0;
        return PR_U68_CLEAR_OK;
    }

    for (i = 0; i < view_count; ++i)
        if (views[i].selector == found.segment)
            break;
    if (i == view_count || !views[i].bytes ||
        views[i].size < PR_U68_RECORD_SEGMENT_SIZE)
        return PR_U68_CLEAR_ARGUMENT;

    {
        uint16_t index;
        uint16_t target;
        /* +0x0663..+0x0670: repeat the scan and zero AH. The original
         * does not test a second miss; index 0xFF still selects a byte. */
        if (pr_u68_record_scan(views[i].bytes, views[i].size, found.offset,
                               0x001Fu, &index) != PR_U68_RECORD_SCAN_OK)
            return PR_U68_CLEAR_ARGUMENT;
        target = (uint16_t)(found.offset + 0x17u + index);
        views[i].bytes[target] = 0; /* +0x0672..+0x0677: ES:[DI+0x17] */
    }
    *ax_out = 1; /* +0x067C..+0x068C */
    return PR_U68_CLEAR_OK;
}
