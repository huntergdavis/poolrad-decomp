#ifndef POOLRAD_RECOVERED_OVERLAY_U68_LIST_SEARCH_H
#define POOLRAD_RECOVERED_OVERLAY_U68_LIST_SEARCH_H

/* DOS v1.3 GAME.OVR unit 0x68 internal +0x05EE..+0x0647.
 * Starting at DS:[0x5D96], inspect each far node with the merged
 * +0x05AB record-byte scan. A hit returns that node in AX:DX
 * (offset:segment); a miss advances through node +0x104; list end returns
 * 0:0. The word argument's low byte is the scan value. See
 * docs/re/U68_LIST_SEARCH_05EE.md.
 *
 * The caller provides a 64 KiB view for each reachable ES selector and
 * scratch far pointers to detect cycles. These guards are host-only: DOS
 * has no missing-view or cycle exit. The result is untouched on error. */

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr_u68_far;

typedef struct {
    uint16_t selector;
    const uint8_t *bytes;
    size_t size;
} pr_u68_segment_view;

typedef enum {
    PR_U68_LIST_OK = 0,
    PR_U68_LIST_ARGUMENT,
    PR_U68_LIST_CYCLE,
    PR_U68_LIST_CAPACITY
} pr_u68_list_status;

pr_u68_list_status pr_u68_list_search(pr_u68_far head,
                                      const pr_u68_segment_view *views,
                                      size_t view_count,
                                      uint16_t value_word,
                                      pr_u68_far *result,
                                      pr_u68_far *visited,
                                      size_t visited_capacity);

#endif
