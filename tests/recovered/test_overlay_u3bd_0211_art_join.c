#include "poolrad/recovered/overlay_u3bd_0211_art_join.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define HEADER 0x17u
#define DEST_DATA (21u * 168u * 4u)
#define FIRST_PIECE (13u * 156u)
#define FIRST_DATA ((6u * 8u * 21u + 7u) * 4u)
#define SECOND_DATA ((7u * 8u * 21u + 7u) * 4u)

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

int main(void)
{
    uint8_t pieces[FIRST_PIECE + 2u] = {0};
    uint8_t source[HEADER + 13u * 32u] = {0};
    uint8_t mask[13u * 8u] = {0};
    uint8_t destination[HEADER + DEST_DATA] = {0};
    pr_u3bd_0211_art_join_state state = {0};
    pr_u3bd_0211_art_join_report report;

    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 1u);
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    pieces[FIRST_PIECE] = 1u;
    pieces[FIRST_PIECE + 1u] = 13u;
    source[HEADER] = 0x80u;
    source[HEADER + 12u * 32u + 1u] = 0x40u;

    state.wall = 14u;
    state.origin_x = 5u;
    state.origin_y = 4u;
    state.view0_columns = 1u;
    state.view0_rows = 2u;
    state.pieces = pieces;
    state.pieces_size = sizeof pieces;
    state.tile.group0_remap = 1u;
    state.tile.flag = 1u;
    state.tile.video_state = 1u;
    state.tile.source = source;
    state.tile.source_size = sizeof source;
    state.tile.aux_mask = mask;
    state.tile.aux_mask_size = sizeof mask;
    state.tile.destination = destination;
    state.tile.destination_size = sizeof destination;

    /* Two reached placement calls feed the merged tile-data effect. */
    assert(pr_u3bd_0211_art_join(&state, &report) == PR_U3BD_0211_ART_OK);
    assert(report.cells_seen == 2u && report.tile_calls == 2u);
    assert(report.first_item == 1u && report.first_x == 7u && report.first_y == 6u);
    assert(report.last_item == 13u && report.last_x == 7u && report.last_y == 7u);
    assert(report.plane_bytes_written == 64u && report.plane_bytes_changed == 2u);
    assert(destination[HEADER + FIRST_DATA] == 0x80u);
    assert(destination[HEADER + SECOND_DATA + 1u] == 0x40u);
    assert(destination[HEADER + FIRST_DATA - 1u] == 0u);
    assert(destination[HEADER + SECOND_DATA + 2u] == 0u);

    /* A zero item skips the second callback; a clipped cell skips its
     * piece read as well as its callback. */
    memset(destination + HEADER, 0, DEST_DATA);
    pieces[FIRST_PIECE + 1u] = 0u;
    assert(pr_u3bd_0211_art_join(&state, &report) == PR_U3BD_0211_ART_OK);
    assert(report.cells_seen == 2u && report.tile_calls == 1u);
    assert(report.plane_bytes_changed == 1u);
    state.origin_y = 10u;
    state.pieces_size = FIRST_PIECE + 1u;
    assert(pr_u3bd_0211_art_join(&state, &report) == PR_U3BD_0211_ART_OK);
    assert(report.cells_seen == 2u && report.tile_calls == 1u);
    state.origin_y = 11u;
    state.pieces = NULL;
    assert(pr_u3bd_0211_art_join(&state, &report) == PR_U3BD_0211_ART_OK);
    assert(report.cells_seen == 2u && report.tile_calls == 0u);

    /* A failure on the second reached callback retains the first effect. */
    state.origin_y = 4u;
    state.pieces = pieces;
    state.pieces_size = sizeof pieces;
    pieces[FIRST_PIECE + 1u] = 46u;
    memset(destination + HEADER, 0, DEST_DATA);
    assert(pr_u3bd_0211_art_join(&state, &report) ==
           PR_U3BD_0211_ART_OTHER_PATH);
    assert(report.tile_calls == 1u && report.plane_bytes_changed == 1u);
    assert(destination[HEADER + FIRST_DATA] == 0x80u);

    assert(pr_u3bd_0211_art_join(NULL, &report) == PR_U3BD_0211_ART_ARGUMENT);
    state.wall = 0u;
    assert(pr_u3bd_0211_art_join(&state, &report) ==
           PR_U3BD_0211_ART_OTHER_PATH);
    state.wall = 14u;
    state.view0_columns = 0u;
    assert(pr_u3bd_0211_art_join(&state, &report) == PR_U3BD_0211_ART_BOUNDS);
    state.view0_columns = 1u;
    state.pieces_size = FIRST_PIECE;
    assert(pr_u3bd_0211_art_join(&state, &report) == PR_U3BD_0211_ART_BOUNDS);

    puts("U3BD +0211 placement to tile-data join tests passed");
    return 0;
}
