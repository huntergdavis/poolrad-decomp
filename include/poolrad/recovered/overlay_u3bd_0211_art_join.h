#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_0211_ART_JOIN_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_0211_ART_JOIN_H

#include "poolrad/recovered/overlay_u40c_tile_plane.h"

#include <stddef.h>
#include <stdint.h>

/* Reached mode-zero path of U3BD local +0x0211, called at +0x0860 on the
 * saved first-view route. The tile template supplies caller-owned objects;
 * item_id/x/y are replaced at each reached +0x02FB callback. */
typedef struct {
    uint8_t wall;
    uint16_t origin_x;
    uint8_t origin_y;
    uint8_t view0_offset;
    uint8_t view0_columns;
    uint8_t view0_rows;
    const uint8_t *pieces;
    size_t pieces_size;
    pr_u40c_tile_plane_state tile;
} pr_u3bd_0211_art_join_state;

typedef enum {
    PR_U3BD_0211_ART_OK = 0,
    PR_U3BD_0211_ART_ARGUMENT,
    PR_U3BD_0211_ART_BOUNDS,
    PR_U3BD_0211_ART_OTHER_PATH
} pr_u3bd_0211_art_join_status;

typedef struct {
    unsigned cells_seen;
    unsigned tile_calls;
    unsigned plane_bytes_written;
    unsigned plane_bytes_changed;
    uint8_t first_item;
    uint8_t last_item;
    uint16_t first_x, first_y;
    uint16_t last_x, last_y;
} pr_u3bd_0211_art_join_report;

/* Models the mode-zero placement walk and each reached group-zero,
 * video-state-one tile-data effect. It does not model U40C stack/CPU
 * changes, dirty metadata, other groups, or visible display transfer.
 * Earlier successful tile effects remain visible if a later call fails. */
pr_u3bd_0211_art_join_status pr_u3bd_0211_art_join(
    const pr_u3bd_0211_art_join_state *state,
    pr_u3bd_0211_art_join_report *report);

#endif
