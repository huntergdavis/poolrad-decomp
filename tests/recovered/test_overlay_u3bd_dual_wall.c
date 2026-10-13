#include "poolrad/recovered/overlay_u3bd_dual_wall.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned count;
    uint16_t x[4], y[4];
    uint8_t item[4];
} draws;

static int record_draw(void *ctx, uint16_t x, uint16_t y, uint8_t item,
                       uint16_t one_a, uint16_t one_b)
{
    draws *out = ctx;
    assert(out->count < 4);
    assert(one_a == 1u && one_b == 1u);
    out->x[out->count] = x;
    out->y[out->count] = y;
    out->item[out->count] = item;
    ++out->count;
    return 0;
}

int main(void)
{
    uint8_t pieces[2340] = {0};
    pr_u3bd_dual_wall_state state = {0};
    pr_u3bd_dual_wall_report report;
    draws out = {0};
    const size_t view9_first = 13u * 156u + 154u;
    const size_t view0_first = 13u * 156u;

    state.current_wall = 14;
    state.previous_wall = 14;
    state.lateral_offset = 2;
    state.view9.offset = 154;
    state.view9.columns = 1;
    state.view9.rows = 2;
    state.view9.base_x = 5;
    state.view9.y = 4;
    state.view0.columns = 1;
    state.view0.rows = 2;
    state.view0.base_x = 5;
    state.view0.y = 4;
    state.pieces = pieces;
    state.pieces_size = sizeof pieces;
    pieces[view9_first] = 1;
    pieces[view9_first + 1] = 13;
    pieces[view0_first] = 1;
    pieces[view0_first + 1] = 13;

    assert(pr_u3bd_entry5_dual_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_DUAL_WALL_OK);
    assert(report.view9_entered && report.view0_entered &&
           report.previous_wall_after == 14);
    assert(report.cells_seen == 4 && report.calls_done == 4);
    assert(out.count == 4);
    assert(out.x[0] == 8 && out.y[0] == 6 && out.item[0] == 1);
    assert(out.x[1] == 8 && out.y[1] == 7 && out.item[1] == 13);
    assert(out.x[2] == 9 && out.y[2] == 6 && out.item[2] == 1);
    assert(out.x[3] == 9 && out.y[3] == 7 && out.item[3] == 13);

    /* With a different current wall, view 9 still selects old wall 14,
     * while +0x084A commits 9 before the empty wall-9 view-zero walk. */
    state.current_wall = 9;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_dual_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_DUAL_WALL_OK);
    assert(report.previous_wall_after == 9 && report.cells_seen == 4 &&
           report.calls_done == 2 && out.count == 2);
    assert(out.x[0] == 8 && out.item[0] == 1 &&
           out.x[1] == 8 && out.item[1] == 13);
    state.current_wall = 14;

    /* +0x0824 zero-current branch leaves this bounded arm. */
    state.current_wall = 0;
    state.pieces = NULL;
    assert(pr_u3bd_entry5_dual_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_DUAL_WALL_OTHER_PATH);
    assert(!report.cells_seen && report.previous_wall_after == 14);
    state.current_wall = 14;
    state.pieces = pieces;

    /* Old wall zero skips view 9, then commits/draws current wall. */
    state.previous_wall = 0;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_dual_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_DUAL_WALL_OK);
    assert(!report.view9_entered && report.view0_entered &&
           report.previous_wall_after == 14);
    assert(out.count == 2 && out.x[0] == 9 && out.x[1] == 9);
    state.previous_wall = 14;

    /* All cells clipped: the cursor advances before any piece read. */
    state.view9.base_x = 14;
    state.view0.base_x = 14;
    state.pieces = NULL;
    assert(pr_u3bd_entry5_dual_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_DUAL_WALL_OK);
    assert(report.cells_seen == 4 && report.calls_done == 0);
    state.view9.base_x = 5;
    state.view0.base_x = 5;
    state.pieces = pieces;

    /* Zero piece bytes read without reaching the callback. */
    pieces[view9_first] = pieces[view9_first + 1] = 0;
    pieces[view0_first] = pieces[view0_first + 1] = 0;
    assert(pr_u3bd_entry5_dual_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_DUAL_WALL_OK);
    assert(report.calls_done == 0 && report.cells_seen == 4);
    pieces[view9_first] = pieces[view0_first] = 1;
    pieces[view9_first + 1] = pieces[view0_first + 1] = 13;

    /* Missing view-nine hook stops before previous-wall update. */
    assert(pr_u3bd_entry5_dual_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_DUAL_WALL_DRAW_HOOK);
    assert(report.view9_entered && !report.view0_entered &&
           report.previous_wall_after == 14 && report.cells_seen == 1);

    /* With old wall zero, a short view-zero buffer fails after one draw. */
    state.previous_wall = 0;
    state.pieces_size = view0_first + 1;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_dual_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_DUAL_WALL_BOUNDS);
    assert(out.count == 1 && report.cells_seen == 2 &&
           report.previous_wall_after == 14);
    state.previous_wall = 14;
    state.pieces_size = sizeof pieces;

    /* View-nine table fails before the commit; view zero fails after it. */
    state.view9.offset = 155;
    assert(pr_u3bd_entry5_dual_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_DUAL_WALL_BOUNDS);
    assert(report.previous_wall_after == 14 && !report.view0_entered);
    state.view9.offset = 154;
    state.view0.offset = 155;
    assert(pr_u3bd_entry5_dual_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_DUAL_WALL_BOUNDS);
    assert(report.previous_wall_after == 14 && report.view0_entered);
    state.view0.offset = 0;

    /* The local helper uses only the low byte of the origin word. */
    state.view0.base_x = 0x105;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_dual_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_DUAL_WALL_OK);
    assert(out.count == 4 && out.x[2] == 9);

    assert(pr_u3bd_entry5_dual_wall(NULL, NULL, NULL, &report) ==
           PR_U3BD_DUAL_WALL_ARGUMENT);
    puts("unit 0x3bd dual-wall bounded branch tests passed");
    return 0;
}
