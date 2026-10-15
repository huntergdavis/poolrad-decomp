#ifndef POOLRAD_RECOVERED_OVERLAY_U40C_GROUP2_TILE_PLANE_H
#define POOLRAD_RECOVERED_OVERLAY_U40C_GROUP2_TILE_PLANE_H

#include <stddef.h>
#include <stdint.h>

/* Reached U40C entry-2 group-two path into RES_0464:08D4. Caller-owned
 * source, auxiliary mask, and destination include the DOS object headers. */
typedef struct {
    uint16_t item_id;           /* entry-2 [BP+0x0A], before remap */
    uint16_t group2_remap;      /* DS:[0x2726] */
    uint8_t x, y;               /* low bytes of entry-2 coordinates */
    uint8_t flag;               /* low byte of entry-2 [BP+8] */
    uint8_t video_state;        /* DS:[0x4A1E] */
    const uint8_t *source;      /* far DS:[0x5E42], group-two object */
    size_t source_size;
    const uint8_t *aux_mask;    /* caller-resolved source header +0x13 */
    size_t aux_mask_size;
    uint8_t *destination;       /* far DS:[0x4A20] */
    size_t destination_size;
} pr_u40c_group2_tile_plane_state;

typedef enum {
    PR_U40C_GROUP2_TILE_OK = 0,
    PR_U40C_GROUP2_TILE_ARGUMENT,
    PR_U40C_GROUP2_TILE_OTHER_PATH,
    PR_U40C_GROUP2_TILE_BOUNDS
} pr_u40c_group2_tile_plane_status;

typedef struct {
    uint16_t item_index;
    size_t first_data_offset;    /* from destination data start */
    size_t first_changed_offset; /* (size_t)-1 if unchanged */
    unsigned bytes_written;
    unsigned bytes_changed;
} pr_u40c_group2_tile_plane_report;

/* Supports group-two IDs 0x74..0xB9, nonzero flag 1, video state 1,
 * 8x1 source tiles with a present auxiliary far pointer, and a 168x21
 * destination. Rejects unsupported routes and bounds before the first
 * destination write. Dirty metadata, other video paths, and full entry-2
 * behavior remain outside this function. */
pr_u40c_group2_tile_plane_status pr_u40c_entry2_group2_tile_plane(
    const pr_u40c_group2_tile_plane_state *state,
    pr_u40c_group2_tile_plane_report *report);

#endif
