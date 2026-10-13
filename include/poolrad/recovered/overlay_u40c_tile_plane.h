#ifndef POOLRAD_RECOVERED_OVERLAY_U40C_TILE_PLANE_H
#define POOLRAD_RECOVERED_OVERLAY_U40C_TILE_PLANE_H

#include <stddef.h>
#include <stdint.h>

/* Saved first-view slice: GAME.OVR unit 0x40C entry 2 group-zero selection
 * followed by RES_0464:08D4's video-state-one, auxiliary-mask byte loop.
 * Source and destination are caller-owned object images, including their
 * 0x17-byte headers. Auxiliary mask bytes are supplied separately; the far
 * pointer in the source header is checked for presence but not followed. */
typedef struct {
    uint16_t item_id;           /* entry-2 [BP+0x0A], before group remap */
    uint16_t group0_remap;      /* DS:[0x2722] */
    uint8_t x, y;               /* entry-2 low bytes of [BP+0x0E/+0x0C] */
    uint8_t flag;               /* entry-2 low byte [BP+6] */
    uint8_t video_state;        /* DS:[0x4A1E] */
    const uint8_t *source;      /* far DS:[0x5E3A], group-zero object */
    size_t source_size;
    const uint8_t *aux_mask;    /* source object far field +0x13 */
    size_t aux_mask_size;
    uint8_t *destination;       /* far DS:[0x4A20], object data at +0x17 */
    size_t destination_size;
} pr_u40c_tile_plane_state;

typedef enum {
    PR_U40C_TILE_PLANE_OK = 0,
    PR_U40C_TILE_PLANE_ARGUMENT,
    PR_U40C_TILE_PLANE_OTHER_PATH,
    PR_U40C_TILE_PLANE_BOUNDS
} pr_u40c_tile_plane_status;

typedef struct {
    uint16_t item_index;        /* after DS:[0x2722] subtraction */
    size_t first_data_offset;   /* from destination data start, not header */
    size_t first_changed_offset;/* from data start; (size_t)-1 if unchanged */
    unsigned bytes_written;     /* 8 rows times 4 EGA plane bytes */
    unsigned bytes_changed;
} pr_u40c_tile_plane_report;

/* Supports group-zero IDs 1..45, flag 1, video state 1, source object
 * 8x1 with 32-byte item stride, and destination object 168x21. x/y must
 * place the full 8-row tile inside that object. These are host scope gates.
 * The original also writes dirty-row and min/max metadata; those effects,
 * other group/video routes, and the later display transfer are omitted.
 * All bounds are checked before the first destination byte is changed. */
pr_u40c_tile_plane_status pr_u40c_entry2_mode1_tile_plane(
    const pr_u40c_tile_plane_state *state, pr_u40c_tile_plane_report *report);

#endif
