#ifndef POOLRAD_RECOVERED_OVERLAY_U25_MOVE_H
#define POOLRAD_RECOVERED_OVERLAY_U25_MOVE_H

/* DOS v1.3 GAME.OVR unit 0x25 entry 19 (+0x19DB, stub 0x2B:0x007F).
 * This routine advances the party x/y bytes once according to facing,
 * then refreshes the third-quarter cell byte and first/second-quarter
 * nibble through the translated unit 0x3BD entries 4 and 2.
 * Evidence: docs/re/U25_MOVE_ENTRY19.md. No game assets are embedded. */
#include <stdint.h>

typedef struct {
    uint8_t x;                  /* DS:0x6AAD */
    uint8_t y;                  /* DS:0x6AAE */
    uint8_t facing;             /* DS:0x6AAF */
    uint8_t cell;               /* DS:0x6AB1 */
    uint8_t nibble;             /* DS:0x6AB0 */
    uint8_t map_id;             /* DS:0x84DC */
    const uint8_t *map_buffer;  /* 1,024 bytes at far DS:[0x6A5C] */
    uint8_t nibble_frame_residue; /* entry 2's unwritten frame byte for an unmatched facing */
} pr_u25_move_state;

typedef enum {
    PR_U25_MOVE_OK = 0,
    PR_U25_MOVE_ARGUMENT,       /* null state, or a map read with null map_buffer */
    PR_U25_MOVE_UNSPECIFIED     /* entry 2 returned its unwritten frame byte */
} pr_u25_move_status;

/* Mutates the fields in the original instruction order. A failed map read
 * leaves any earlier position write intact, as the original would have done
 * before touching an absent far pointer. */
pr_u25_move_status pr_u25_entry19(pr_u25_move_state *state);

#endif
