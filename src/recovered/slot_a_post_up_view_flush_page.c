#include "poolrad/recovered/slot_a_post_up_view_flush_page.h"

#include <string.h>

pr_slot_a_post_up_flush_result pr_slot_a_post_up_view_flush_page(
    pr_r464_dirty_state *dirty, pr_slot_a_post_up_view_page *page,
    const uint8_t *offscreen, size_t offscreen_size,
    pr_r464_dirty_report *report) {
    pr_r464_dirty_state reached;
    unsigned plane;

    if (!dirty)
        return PR_SLOT_A_POST_UP_FLUSH_BLOCKED_DIRTY_FRAME;
    if (!page || !report)
        return PR_SLOT_A_POST_UP_FLUSH_BAD_ARG;
    if (!offscreen)
        return PR_SLOT_A_POST_UP_FLUSH_BLOCKED_OFFSCREEN;
    if (page->plane_bytes < PR_SLOT_A_POST_UP_PLANE_BYTES ||
        dirty->vram_size > 0x1F40u ||
        dirty->vram_origin > 0x1F40u - dirty->vram_size ||
        (size_t)dirty->vram_origin + dirty->vram_size >
            page->plane_bytes - 0x2000u)
        return PR_SLOT_A_POST_UP_FLUSH_TRANSFER_BOUNDARY;

    reached = *dirty;
    for (plane = 0; plane < 4; ++plane) {
        if (!page->plane[plane])
            return PR_SLOT_A_POST_UP_FLUSH_BAD_ARG;
        reached.page[plane] = page->plane[plane] + dirty->vram_origin;
        reached.staging[plane] = page->plane[plane] + 0x2000u +
                                 dirty->vram_origin;
    }
    if (pr_r464_dirty_transfer(&reached, offscreen, offscreen_size, report)
        != PR_R464_DIRTY_OK)
        return PR_SLOT_A_POST_UP_FLUSH_TRANSFER_BOUNDARY;
    memcpy(dirty->row, reached.row, sizeof dirty->row);
    return PR_SLOT_A_POST_UP_FLUSH_OK;
}
