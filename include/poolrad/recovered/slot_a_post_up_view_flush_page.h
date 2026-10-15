#ifndef POOLRAD_RECOVERED_SLOT_A_POST_UP_VIEW_FLUSH_PAGE_H
#define POOLRAD_RECOVERED_SLOT_A_POST_UP_VIEW_FLUSH_PAGE_H

#include "poolrad/recovered/resident_464_dirty_transfer.h"

#define PR_SLOT_A_POST_UP_PLANE_BYTES 0x3F40u

typedef struct {
    uint8_t *plane[4];       /* persistent A000 [0,1F40), A200 [2000,3F40) */
    size_t plane_bytes;
} pr_slot_a_post_up_view_page;

typedef enum {
    PR_SLOT_A_POST_UP_FLUSH_OK = 0,
    PR_SLOT_A_POST_UP_FLUSH_BLOCKED_DIRTY_FRAME,
    PR_SLOT_A_POST_UP_FLUSH_BLOCKED_OFFSCREEN,
    PR_SLOT_A_POST_UP_FLUSH_BAD_ARG,
    PR_SLOT_A_POST_UP_FLUSH_TRANSFER_BOUNDARY
} pr_slot_a_post_up_flush_result;

/* Compose the reached video-state-1 RES_0464:1B52 transfer into four
 * persistent EGA planes. The caller supplies the pre-flush dirty metadata
 * and offscreen object. Missing dirty-frame or offscreen generation is an
 * explicit stop;
 * no plane, metadata or report changes on any stop. This does not model
 * the preceding U1C view call or other RES_0464 video states. */
pr_slot_a_post_up_flush_result pr_slot_a_post_up_view_flush_page(
    pr_r464_dirty_state *dirty, pr_slot_a_post_up_view_page *page,
    const uint8_t *offscreen, size_t offscreen_size,
    pr_r464_dirty_report *report);

#endif
