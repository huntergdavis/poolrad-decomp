#ifndef POOLRAD_RECOVERED_OVERLAY_U40C_GROUP3_TILE_PLANE_H
#define POOLRAD_RECOVERED_OVERLAY_U40C_GROUP3_TILE_PLANE_H

#include <stddef.h>
#include <stdint.h>

/* Reached unit 0x40C entry-2 group-three, video-state-one tile effect.
 * The far pointer in source header +0x13 is checked for presence; aux_mask
 * supplies its caller-resolved data. Only the destination object is modeled. */
typedef struct {
    uint16_t item_id;           /* entry-2 [BP+0x0A] */
    uint16_t group3_remap;      /* DS:[0x2728] */
    uint8_t x, y;               /* entry-2 low coordinate bytes */
    uint8_t flag;               /* entry-2 low [BP+8] */
    uint8_t video_state;        /* DS:[0x4A1E] */
    const uint8_t *source;      /* group-three object, including header */
    size_t source_size;
    const uint8_t *aux_mask;    /* resolved source far field +0x13 */
    size_t aux_mask_size;
    uint8_t *destination;       /* offscreen object, including header */
    size_t destination_size;
} pr_u40c_group3_tile_plane_state;

typedef enum {
    PR_U40C_GROUP3_TILE_OK = 0,
    PR_U40C_GROUP3_TILE_ARGUMENT,
    PR_U40C_GROUP3_TILE_OTHER_PATH,
    PR_U40C_GROUP3_TILE_BOUNDS
} pr_u40c_group3_tile_plane_status;

typedef struct {
    uint16_t item_index;
    size_t first_data_offset;   /* from destination data start */
    size_t first_changed_offset;/* (size_t)-1 if unchanged */
    unsigned bytes_written;
    unsigned bytes_changed;
} pr_u40c_group3_tile_plane_report;

/* Supports only group-three IDs 0xBA..0xFF, nonzero reached flag 1,
 * video state 1, 8x1 source tiles with a present auxiliary far pointer,
 * and 168x21 destination. Checks every span before the first write.
 * Original DS dirty-row and extent metadata effects are outside this model. */
pr_u40c_group3_tile_plane_status pr_u40c_entry2_group3_tile_plane(
    const pr_u40c_group3_tile_plane_state *state,
    pr_u40c_group3_tile_plane_report *report);

#endif
