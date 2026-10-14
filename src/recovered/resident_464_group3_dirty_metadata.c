#include "poolrad/recovered/resident_464_group3_dirty_metadata.h"

#include <string.h>

#define DIRTY_ROWS 8u
#define DEST_ROWS 168u
#define DEST_COLUMNS 21u
#define DEST_PLANES 4u
#define HEADER_SIZE 0x17u
#define DS_END 0x4EC0u

static uint16_t word(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static unsigned put_byte(uint8_t *p, uint8_t v)
{
    unsigned changed = *p != v;
    *p = v;
    return changed;
}

static unsigned put_word(uint8_t *p, uint16_t v)
{
    unsigned changed = (p[0] != (uint8_t)v) +
                       (p[1] != (uint8_t)(v >> 8));
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    return changed;
}

pr_464_group3_dirty_status pr_464_group3_dirty_metadata(
    const pr_464_group3_dirty_state *state, pr_464_group3_dirty_report *report)
{
    pr_464_group3_dirty_report local;
    pr_464_group3_dirty_report *r = report ? report : &local;
    unsigned row;

    if (!state) return PR_464_GROUP3_DIRTY_ARGUMENT;
    memset(r, 0, sizeof *r);
    if (state->video_state != 1u || (state->flag & 1u) == 0u ||
        state->x >= DEST_COLUMNS ||
        (unsigned)state->y * DIRTY_ROWS + DIRTY_ROWS > DEST_ROWS)
        return PR_464_GROUP3_DIRTY_OTHER_PATH;
    if (!state->source || !state->destination || !state->ds ||
        state->source_size < HEADER_SIZE ||
        state->destination_size < HEADER_SIZE ||
        state->ds_size < DS_END)
        return PR_464_GROUP3_DIRTY_BOUNDS;
    if ((word(state->source + 0x13u) |
         word(state->source + 0x15u)) == 0u ||
        word(state->source) != DIRTY_ROWS ||
        word(state->source + 2u) != 1u ||
        word(state->destination) != DEST_ROWS ||
        word(state->destination + 2u) != DEST_COLUMNS)
        return PR_464_GROUP3_DIRTY_OTHER_PATH;

    /* +0x0BEB..+0x0C1B: the outer row starts at y*8; x is converted
     * to four interleaved plane bytes per column. +0x0C8C..+0x0CA4
     * extends the last index after the plane-byte loop. */
    for (row = 0u; row < DIRTY_ROWS; ++row) {
        unsigned index = (unsigned)state->y * DIRTY_ROWS + row;
        uint16_t first = (uint16_t)((index * DEST_COLUMNS +
                                     state->x) * DEST_PLANES);
        uint16_t last = (uint16_t)(first + DEST_PLANES - 1u);
        uint8_t *ds = state->ds;
        r->changed_bytes += put_byte(ds + 0x4A28u + index, 1u);
        ++r->rows_marked;
        if (word(ds + 0x4C20u + index * 2u) > first) {
            r->changed_bytes +=
                put_word(ds + 0x4AD0u + index * 2u, state->x);
            r->changed_bytes +=
                put_word(ds + 0x4C20u + index * 2u, first);
            ++r->first_extent_updates;
        }
        if (word(ds + 0x4D70u + index * 2u) < last) {
            r->changed_bytes +=
                put_word(ds + 0x4D70u + index * 2u, last);
            ++r->last_extent_updates;
        }
    }
    return PR_464_GROUP3_DIRTY_OK;
}
