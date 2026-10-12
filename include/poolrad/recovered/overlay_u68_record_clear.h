#ifndef POOLRAD_RECOVERED_OVERLAY_U68_RECORD_CLEAR_H
#define POOLRAD_RECOVERED_OVERLAY_U68_RECORD_CLEAR_H

/* DOS v1.3 GAME.OVR unit 0x68 internal +0x0648..+0x068C.
 * Search the far list for the first node containing byte 0x1F, scan that
 * node again, and clear the matching byte at node+0x17+index. Return AX=1
 * for a found node and AX=0 for a null list result. The caller supplies
 * mutable 64 KiB segment views plus read-only adapters and visited scratch
 * for the merged list-search helper. Host guard errors leave AX unchanged.
 * See docs/re/U68_RECORD_CLEAR_0648.md. */

#include "poolrad/recovered/overlay_u68_list_search.h"

typedef struct {
    uint16_t selector;
    uint8_t *bytes;
    size_t size;
} pr_u68_mutable_segment_view;

typedef enum {
    PR_U68_CLEAR_OK = 0,
    PR_U68_CLEAR_ARGUMENT,
    PR_U68_CLEAR_CYCLE,
    PR_U68_CLEAR_CAPACITY
} pr_u68_clear_status;

pr_u68_clear_status pr_u68_record_clear(pr_u68_far head,
                                        const pr_u68_mutable_segment_view *views,
                                        size_t view_count,
                                        pr_u68_segment_view *read_views,
                                        size_t read_views_capacity,
                                        pr_u68_far *visited,
                                        size_t visited_capacity,
                                        uint16_t *ax_out);

#endif
