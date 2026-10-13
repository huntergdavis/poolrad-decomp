#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_FIRST_WALL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_FIRST_WALL_H

#include <stddef.h>
#include <stdint.h>

/* Bounded path through GAME.OVR unit 0x3BD entry 5: the first, distance-two
 * front-wall sample (+0x0682..+0x0743) and its mode-zero local placement
 * helper (+0x0211..+0x031E). This is not the whole entry-5 renderer.
 * All original data tables and decoded art stay in caller-owned storage. */
typedef struct {
    uint8_t party_x, party_y, facing;
    uint8_t map_id;
    uint8_t step_x, step_y;       /* DS:[0x2736+facing], [0x273F+facing] */
    const uint8_t *geo;           /* far [0x6A5C], 1,024 decoded bytes */
    size_t geo_size;
    const uint8_t *pieces;        /* far [0x6A58], already loaded/remapped */
    size_t pieces_size;
    uint8_t view0_offset;        /* DS:[0x0C8C] */
    uint8_t view0_columns;       /* DS:[0x0C96] */
    uint8_t view0_rows;          /* DS:[0x0CA0] */
    uint16_t view0_x;            /* DS:[0x0CAA], low byte read by helper */
    uint8_t view0_y;             /* DS:[0x0CBE] */
    uint8_t lookup_frame_residue;/* entry-2's unwritten-result path */
} pr_u3bd_first_wall_state;

/* The original +0x02FB calls overlay 0x428:0x002A with five pushed words:
 * tile x, tile y, item, 1, 1. This callback receives the same values in
 * named order. It is reached only for a positive item on a visible cell. */
typedef int (*pr_u3bd_first_wall_draw)(void *ctx, uint16_t x, uint16_t y,
                                      uint8_t item, uint16_t one_a,
                                      uint16_t one_b);

typedef enum {
    PR_U3BD_FIRST_WALL_OK = 0,
    PR_U3BD_FIRST_WALL_ARGUMENT,
    PR_U3BD_FIRST_WALL_BOUNDS,
    PR_U3BD_FIRST_WALL_OTHER_PATH,
    PR_U3BD_FIRST_WALL_LOOKUP_UNSPECIFIED,
    PR_U3BD_FIRST_WALL_DRAW_HOOK
} pr_u3bd_first_wall_status;

typedef struct {
    int16_t sample_x, sample_y;  /* signed word locals; +0x031F sees low bytes */
    uint8_t wall;                /* AL from merged +0x047B lookup */
    uint8_t entered_draw;        /* nonzero wall takes +0x0726..+0x0742 */
    unsigned cells_seen;         /* +0x0211 loop cells, including clipped */
    unsigned calls_done;         /* positive, unclipped items passed to hook */
} pr_u3bd_first_wall_report;

/* Starts with the entry-5 frame state at +0x0682: first distance is 2,
 * previous wall and horizontal offset are zero. This is the in-grid path
 * after +0x06DB/+0x06EB narrow each word sample to a signed low byte;
 * an out-of-grid byte returns OTHER_PATH because +0x06F8 performs a
 * second lookup. A zero wall stops at the +0x0744 continuation; a nonzero
 * wall returns after the +0x073F call.
 * Remaining entry-5 scan iterations and renderer effects are outside this
 * slice. The report describes any effects reached before a host error. */
pr_u3bd_first_wall_status pr_u3bd_entry5_first_wall(
    const pr_u3bd_first_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_first_wall_report *report);

#endif
