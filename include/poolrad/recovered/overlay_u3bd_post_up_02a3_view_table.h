#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_02A3_VIEW_TABLE_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_02A3_VIEW_TABLE_H

#include <stddef.h>
#include <stdint.h>

/* Original unit 0x3BD:+02A3..+02C4, reached after the view bounds gate.
 * ram is a conventional-memory image indexed by real-mode physical address. */
typedef struct {
    const uint8_t *ram;
    size_t ram_size;
    uint16_t ss, ds, bp;
    uint16_t ax, cx, dx, es, di, flags;
    uint16_t known_arith_flags; /* Defined bits among CF/PF/AF/ZF/SF/OF. */
} pr_u3bd_post_up_02a3_view_table_state;

typedef struct {
    uint8_t column, slice, item;
    uint16_t item_offset, next_ip; /* +02C4 for item > 0, +0300 for zero. */
    uint32_t item_physical;
} pr_u3bd_post_up_02a3_view_table_report;

typedef enum {
    PR_U3BD_POST_UP_02A3_VIEW_TABLE_OK = 0,
    PR_U3BD_POST_UP_02A3_VIEW_TABLE_ARGUMENT,
    PR_U3BD_POST_UP_02A3_VIEW_TABLE_BOUNDARY
} pr_u3bd_post_up_02a3_view_table_status;

pr_u3bd_post_up_02a3_view_table_status
pr_u3bd_post_up_02a3_view_table(
    pr_u3bd_post_up_02a3_view_table_state *state,
    pr_u3bd_post_up_02a3_view_table_report *report);

#endif
