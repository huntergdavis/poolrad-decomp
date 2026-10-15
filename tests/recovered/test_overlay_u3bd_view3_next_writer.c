#include "poolrad/recovered/overlay_u3bd_view3_next_writer.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define PIECES_SIZE (15u * 156u)

typedef struct {
    unsigned count;
    unsigned fail_at;
    uint16_t x[4], y[4];
    uint8_t item[4];
} draw_log;

static int draw(void *ctx, uint16_t x, uint16_t y, uint8_t item,
                uint16_t one_a, uint16_t one_b)
{
    draw_log *log = ctx;
    unsigned at = log->count++;
    assert(one_a == 1u && one_b == 1u);
    assert(at < 4u);
    log->x[at] = x;
    log->y[at] = y;
    log->item[at] = item;
    return log->fail_at != 0u && log->count == log->fail_at;
}

static pr_u3bd_view3_next_writer_state setup(uint8_t *geo, uint8_t *pieces,
                                             const uint8_t *x_shift,
                                             const uint8_t *y_shift)
{
    pr_u3bd_view3_next_writer_state state;
    memset(&state, 0, sizeof state);
    state.sample_x = 14u;
    state.sample_y = 6u;
    state.lateral_offset = (uint16_t)-6;
    state.outer_selector = 6u;
    state.facing = 4u;
    state.map_id = 20u;
    state.geo = geo;
    state.geo_size = 1024u;
    state.pieces = pieces;
    state.pieces_size = PIECES_SIZE;
    state.x_shift = x_shift;
    state.y_shift = y_shift;
    state.shift_count = 1u;
    state.view3_offset = 10u;
    state.view3_columns = 3u;
    state.view3_rows = 4u;
    state.view3_base_x = 4u;
    state.view3_y = 3u;
    return state;
}

int main(void)
{
    uint8_t geo[1024] = {0};
    uint8_t pieces[PIECES_SIZE] = {0};
    const uint8_t x_shift[1] = {0};
    const uint8_t y_shift[1] = {255};
    pr_u3bd_view3_next_writer_state state =
        setup(geo, pieces, x_shift, y_shift);
    pr_u3bd_view3_next_writer_report report;
    draw_log log;
    size_t wall9 = 8u * 156u + 10u;
    size_t wall14 = 13u * 156u + 10u;

    /* Invented walls: high nibble is the view-four gate, low nibble
     * selects the view-three piece slice on each sample row. */
    geo[0x100u + 16u * 6u + 14u] = 0x09u;
    geo[0x100u + 16u * 5u + 14u] = 0x09u;
    geo[0x100u + 16u * 4u + 14u] = 0xFEu;
    pieces[wall9] = 7u;
    pieces[wall9 + 3u] = 128u;
    pieces[wall14] = 7u;
    pieces[wall14 + 3u] = 40u;

    memset(&log, 0, sizeof log);
    assert(pr_u3bd_entry5_view3_next_writer(&state, draw, &log, &report)
           == PR_U3BD_VIEW3_NEXT_OK);
    assert(log.count == 4u && report.calls_done == 4u);
    assert(report.scan_index == 2u && report.sample_x == 14u &&
           report.sample_y == 4u && report.lateral_offset == 0u);
    assert(report.view3_wall[0] == 9u && report.view3_wall[1] == 14u);
    assert(report.view4_wall[0] == 0u && report.view4_wall[1] == 15u);
    assert(log.x[0] == 3u && log.y[0] == 5u && log.item[0] == 7u);
    assert(log.x[1] == 3u && log.y[1] == 6u && log.item[1] == 128u);
    assert(log.x[2] == 6u && log.y[2] == 5u && log.item[2] == 7u);
    assert(log.x[3] == 6u && log.y[3] == 6u && log.item[3] == 40u);

    /* Initial nonzero view four takes a path outside this prefix. */
    geo[0x100u + 16u * 6u + 14u] = 0x19u;
    memset(&log, 0, sizeof log);
    assert(pr_u3bd_entry5_view3_next_writer(&state, draw, &log, &report)
           == PR_U3BD_VIEW3_NEXT_OTHER_PATH);
    assert(log.count == 0u && report.scan_index == 0u);
    geo[0x100u + 16u * 6u + 14u] = 0x09u;

    /* A changed middle branch preserves the two already completed
     * callback effects and stops before the next scan update. */
    geo[0x100u + 16u * 5u + 14u] = 0x19u;
    memset(&log, 0, sizeof log);
    assert(pr_u3bd_entry5_view3_next_writer(&state, draw, &log, &report)
           == PR_U3BD_VIEW3_NEXT_OTHER_PATH);
    assert(log.count == 2u && report.calls_done == 2u &&
           report.scan_index == 1u && report.sample_y == 5u);
    geo[0x100u + 16u * 5u + 14u] = 0x09u;

    geo[0x100u + 16u * 4u + 14u] = 0x0Eu;
    memset(&log, 0, sizeof log);
    assert(pr_u3bd_entry5_view3_next_writer(&state, draw, &log, &report)
           == PR_U3BD_VIEW3_NEXT_OTHER_PATH);
    assert(log.count == 4u && report.calls_done == 4u &&
           report.scan_index == 2u);
    geo[0x100u + 16u * 4u + 14u] = 0xFEu;

    memset(&log, 0, sizeof log);
    log.fail_at = 2u;
    assert(pr_u3bd_entry5_view3_next_writer(&state, draw, &log, &report)
           == PR_U3BD_VIEW3_NEXT_DRAW_HOOK);
    assert(log.count == 2u && report.calls_done == 1u &&
           report.scan_index == 1u);

    state.direction_index = 1u;
    memset(&log, 0, sizeof log);
    assert(pr_u3bd_entry5_view3_next_writer(&state, draw, &log, &report)
           == PR_U3BD_VIEW3_NEXT_BOUNDS);
    assert(log.count == 0u);
    return 0;
}
