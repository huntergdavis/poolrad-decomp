#ifndef POOLRAD_RECOVERED_OVERLAY_U68_MOVEMENT_FLAG_H
#define POOLRAD_RECOVERED_OVERLAY_U68_MOVEMENT_FLAG_H

/* Bounded C model of DOS v1.3 GAME.OVR unit 0x68 +0x04DF..+0x05AA.
 * Iterate the far list at DS:[0x5D96], roll once per reached node, select
 * the first node with roll <= byte +0x78 and byte +0x10C == 0, clear
 * DS:[0x6DDF], and on selection call local +0x0109 twice. The roll at
 * 0xB0:0x0048 and local map mutation remain explicit hooks. A missing hook
 * is reported only when reached; prior effects are retained. See
 * docs/re/U68_MOVEMENT_FLAG_04DF.md. */

#include "poolrad/recovered/overlay_u68_list_search.h"

/* Effective low bytes read by unit 0x9E entry 8 at [bp+6]/[bp+8].
 * The caller pushes AX after setting only AL, so pushed AH is unproved. */
typedef uint8_t (*pr_u68_move_roll_hook)(void *ctx, uint8_t range_byte,
                                          uint8_t count_byte);
typedef void (*pr_u68_move_mark_hook)(void *ctx, uint8_t x, uint8_t y,
                                      uint8_t selector);

typedef struct {
    pr_u68_move_roll_hook roll_9e_entry8;
    pr_u68_move_mark_hook mark_0109;
    void *ctx;
} pr_u68_move_hooks;

typedef struct {
    pr_u68_far head;                   /* DS:[0x5D96]:[0x5D98] */
    const pr_u68_segment_view *views; /* 64 KiB ES views for reached nodes */
    size_t view_count;
    pr_u68_far *visited;              /* caller scratch for host cycle guard */
    size_t visited_capacity;
    uint8_t party_x;                  /* DS:[0x6AAD], read on selection */
    uint8_t party_y;                  /* DS:[0x6AAE], read on selection */
    uint8_t facing;                   /* DS:[0x6AAF], read on selection */
    uint8_t ds_6ddf;                 /* set to 0 after list traversal */
    const uint8_t *direction_table;   /* DS bytes from 0x2736, reached on selection */
    size_t direction_table_size;
    pr_u68_move_hooks hooks;
} pr_u68_move_state;

typedef enum {
    PR_U68_MOVE_OK = 0,
    PR_U68_MOVE_ARGUMENT,
    PR_U68_MOVE_CYCLE,
    PR_U68_MOVE_CAPACITY,
    PR_U68_MOVE_HOOK
} pr_u68_move_status;

/* On OK, AL is 0 or 1. On host error, al_out is untouched. The original
 * does not have host mapping, cycle, capacity, or missing-hook exits. */
pr_u68_move_status pr_u68_movement_flag(pr_u68_move_state *state,
                                         uint8_t *al_out);

#endif
