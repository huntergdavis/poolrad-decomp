#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_INSERT_1446_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_INSERT_1446_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u9b_record_incr_01af.h"

/* DOS far-pointer words are offset then segment. The host supplies one
 * mutable view per segment. Effective offsets wrap modulo 0x10000. */
typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr_u6f_insert_far;

typedef struct {
    uint16_t segment;
    uint8_t *bytes;
    size_t size;
} pr_u6f_insert_segment;

typedef struct {
    pr_u6f_insert_far head;       /* DS:0x5D96/0x5D98 */
    pr_u6f_insert_far tail;       /* DS:0x5D92/0x5D94 */
    pr_u6f_insert_far window1;    /* DS:0x49D6/0x49D8 */
    const pr_u6f_insert_segment *segments;
    size_t segment_count;
} pr_u6f_insert_state;

typedef struct {
    size_t append_nodes;
    size_t mark_nodes;
    uint8_t selected_slot;       /* incoming +0xBF after selection, if assigned */
    uint8_t has_selected_slot;
    uint8_t incremented_window;
    uint16_t next_offset;        /* source boundary; +0x157C is terminal RETF */
    pr_u9b_incr_report callee;   /* may contain partial final-call effects */
} pr_u6f_insert_result;

typedef enum {
    PR_U6F_INSERT_OK = 0,
    PR_U6F_INSERT_ARGUMENT,
    PR_U6F_INSERT_BOUNDS,
    PR_U6F_INSERT_TRAVERSAL_LIMIT,
    PR_U6F_INSERT_CALLEE_BOUNDARY
} pr_u6f_insert_status;

/* Complete code path of unit 0x6F:+0x1446..+0x157E for mapped memory.
 * The next-node limit is a host guard for malformed cycles. It stops at the
 * next loop head, preserving earlier record/global writes. The caller owns
 * records and views; this routine never loads files or allocates records. */
pr_u6f_insert_status pr_u6f_party_insert_1446(
    pr_u6f_insert_state *state, pr_u6f_insert_far incoming,
    size_t max_nodes, pr_u6f_insert_result *out);

#endif
