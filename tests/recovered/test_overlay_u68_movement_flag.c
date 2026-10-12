#include "poolrad/recovered/overlay_u68_movement_flag.h"
#include "poolrad/recovered/overlay_u68_record_scan.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t first_segment[PR_U68_RECORD_SEGMENT_SIZE];
static uint8_t second_segment[PR_U68_RECORD_SEGMENT_SIZE];

typedef struct {
    uint8_t rolls[4];
    unsigned roll_count;
    uint8_t last_range, last_count;
    uint8_t marks[2][3];
    uint8_t flag_at_mark[2];
    unsigned mark_count;
    pr_u68_move_state *state;
    int mutate_on_first_mark;
} trace;

static uint8_t roll_hook(void *ctx, uint8_t range, uint8_t count)
{
    trace *t = ctx;
    unsigned i = t->roll_count++;
    t->last_range = range;
    t->last_count = count;
    return t->rolls[i < 4u ? i : 3u];
}

static void mark_hook(void *ctx, uint8_t x, uint8_t y, uint8_t selector)
{
    trace *t = ctx;
    unsigned i = t->mark_count++;
    if (i < 2u) {
        t->marks[i][0] = x;
        t->marks[i][1] = y;
        t->marks[i][2] = selector;
        t->flag_at_mark[i] = t->state->ds_6ddf;
    }
    if (i == 0u && t->mutate_on_first_mark) {
        t->state->party_x = 8;
        t->state->party_y = 9;
        t->state->facing = 4;
    }
}

static void put_word(uint8_t *bytes, uint16_t at, uint16_t value)
{
    bytes[at] = (uint8_t)value;
    bytes[(uint16_t)(at + 1u)] = (uint8_t)(value >> 8);
}

static void put_next(uint8_t *bytes, pr_u68_far node, pr_u68_far next)
{
    put_word(bytes, (uint16_t)(node.offset + 0x104u), next.offset);
    put_word(bytes, (uint16_t)(node.offset + 0x106u), next.segment);
}

static void prepare(pr_u68_move_state *state, trace *t,
                    pr_u68_segment_view *views, pr_u68_far *visited,
                    uint8_t *direction_table)
{
    memset(state, 0, sizeof *state);
    memset(t, 0, sizeof *t);
    memset(first_segment, 0xA5, sizeof first_segment);
    memset(second_segment, 0xA5, sizeof second_segment);
    memset(direction_table, 0, 265);
    state->head = (pr_u68_far){0x0100u, 0x1111u};
    state->views = views;
    state->view_count = 2;
    state->visited = visited;
    state->visited_capacity = 4;
    state->party_x = 4;
    state->party_y = 5;
    state->facing = 2;
    state->ds_6ddf = 7;
    state->direction_table = direction_table;
    state->direction_table_size = 265;
    state->hooks.roll_9e_entry8 = roll_hook;
    state->hooks.mark_0109 = mark_hook;
    state->hooks.ctx = t;
    t->state = state;
    put_next(first_segment, state->head, (pr_u68_far){0, 0});
}

int main(void)
{
    const pr_u68_far first = {0x0100u, 0x1111u};
    const pr_u68_far second = {0x0200u, 0x2222u};
    const pr_u68_far zero = {0, 0};
    pr_u68_segment_view views[2] = {
        {0x1111u, first_segment, sizeof first_segment},
        {0x2222u, second_segment, sizeof second_segment}
    };
    pr_u68_far visited[4];
    uint8_t table[265];
    pr_u68_move_state state;
    trace t;
    uint8_t al = 0xEE;

    /* Empty head performs no calls but still clears DS:[0x6DDF]. */
    prepare(&state, &t, views, visited, table);
    state.head = zero;
    state.views = NULL;
    state.view_count = 0;
    state.visited = NULL;
    state.visited_capacity = 0;
    state.hooks.roll_9e_entry8 = NULL;
    state.hooks.mark_0109 = NULL;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_OK);
    CHECK(al == 0 && state.ds_6ddf == 0 && t.roll_count == 0 && t.mark_count == 0);

    /* Equal roll is a hit. A self next pointer is read but not traversed. */
    prepare(&state, &t, views, visited, table);
    t.rolls[0] = 20;
    first_segment[first.offset + 0x78u] = 20;
    first_segment[first.offset + 0x10Cu] = 0;
    put_next(first_segment, first, first);
    table[2] = 0xFFu;   /* -1 */
    table[11] = 1;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_OK);
    CHECK(al == 1 && state.ds_6ddf == 0);
    CHECK(t.roll_count == 1 && t.last_range == 100 && t.last_count == 1);
    CHECK(t.mark_count == 2);
    CHECK(t.marks[0][0] == 4 && t.marks[0][1] == 5 && t.marks[0][2] == 2);
    CHECK(t.marks[1][0] == 3 && t.marks[1][1] == 6 && t.marks[1][2] == 6);
    CHECK(t.flag_at_mark[0] == 0 && t.flag_at_mark[1] == 0);
    CHECK(state.party_x == 4 && state.party_y == 5 && state.facing == 2);

    /* Roll above threshold and occupied node both fall through to null. */
    prepare(&state, &t, views, visited, table);
    t.rolls[0] = 21;
    first_segment[first.offset + 0x78u] = 20;
    first_segment[first.offset + 0x10Cu] = 0;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_OK);
    CHECK(al == 0 && state.ds_6ddf == 0 && t.roll_count == 1 && t.mark_count == 0);
    prepare(&state, &t, views, visited, table);
    t.rolls[0] = 20;
    first_segment[first.offset + 0x78u] = 20;
    first_segment[first.offset + 0x10Cu] = 1;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_OK);
    CHECK(al == 0 && t.mark_count == 0);

    /* JA compares unsigned bytes: 100 is admitted by threshold 0x80. */
    prepare(&state, &t, views, visited, table);
    t.rolls[0] = 100;
    first_segment[first.offset + 0x78u] = 0x80u;
    first_segment[first.offset + 0x10Cu] = 0;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_OK);
    CHECK(al == 1 && t.mark_count == 2);

    /* First node blocked, second selected; the first map call may update
     * DS x/y/facing, and the second must use those newly read bytes. */
    prepare(&state, &t, views, visited, table);
    t.rolls[0] = 10;
    t.rolls[1] = 20;
    t.mutate_on_first_mark = 1;
    first_segment[first.offset + 0x78u] = 10;
    first_segment[first.offset + 0x10Cu] = 1;
    put_next(first_segment, first, second);
    put_next(second_segment, second, zero);
    second_segment[second.offset + 0x78u] = 20;
    second_segment[second.offset + 0x10Cu] = 0;
    table[4] = 2;
    table[13] = 0xFEu; /* -2 */
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_OK);
    CHECK(al == 1 && t.roll_count == 2 && t.mark_count == 2);
    CHECK(t.marks[0][0] == 4 && t.marks[0][1] == 5 && t.marks[0][2] == 2);
    CHECK(t.marks[1][0] == 10 && t.marks[1][1] == 7 && t.marks[1][2] == 0);
    CHECK(state.party_x == 8 && state.party_y == 9 && state.facing == 4);

    /* Nonzero segment with offset zero is a valid far node. */
    prepare(&state, &t, views, visited, table);
    state.head.offset = 0;
    first_segment[0x78u] = 1;
    first_segment[0x10Cu] = 0;
    put_next(first_segment, state.head, zero);
    t.rolls[0] = 1;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_OK);
    CHECK(al == 1 && t.mark_count == 2);

    /* The next far word is read even after a hit. A host word beginning
     * at FFFF is guarded after the roll and before DS flag clearing. */
    prepare(&state, &t, views, visited, table);
    state.head.offset = 0xFEFBu;
    first_segment[(uint16_t)(state.head.offset + 0x78u)] = 1;
    first_segment[(uint16_t)(state.head.offset + 0x10Cu)] = 0;
    t.rolls[0] = 1;
    al = 0xEE;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_ARGUMENT);
    CHECK(t.roll_count == 1 && t.mark_count == 0);
    CHECK(state.ds_6ddf == 7 && al == 0xEE);

    /* Missing hooks are checked only when reached. A selected node has
     * already executed the unconditional DS flag clear before map calls. */
    prepare(&state, &t, views, visited, table);
    state.hooks.roll_9e_entry8 = NULL;
    al = 0xEE;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_HOOK);
    CHECK(al == 0xEE && state.ds_6ddf == 7);
    prepare(&state, &t, views, visited, table);
    first_segment[first.offset + 0x78u] = 1;
    first_segment[first.offset + 0x10Cu] = 0;
    t.rolls[0] = 1;
    state.hooks.mark_0109 = NULL;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_HOOK);
    CHECK(al == 0xEE && state.ds_6ddf == 0 && t.roll_count == 1);

    /* Table guard comes after the first mark, matching the read boundary. */
    prepare(&state, &t, views, visited, table);
    first_segment[first.offset + 0x78u] = 1;
    first_segment[first.offset + 0x10Cu] = 0;
    t.rolls[0] = 1;
    state.direction_table_size = 11;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_ARGUMENT);
    CHECK(state.ds_6ddf == 0 && t.mark_count == 1 && al == 0xEE);

    /* Host cycle, capacity, and absent segment terminate without a flag
     * clear or result write. The DOS loop has no corresponding exits. */
    prepare(&state, &t, views, visited, table);
    t.rolls[0] = 2;
    first_segment[first.offset + 0x78u] = 1;
    put_next(first_segment, first, first);
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_CYCLE);
    CHECK(state.ds_6ddf == 7 && al == 0xEE && t.roll_count == 1);
    prepare(&state, &t, views, visited, table);
    t.rolls[0] = 2;
    first_segment[first.offset + 0x78u] = 1;
    put_next(first_segment, first, second);
    state.visited_capacity = 1;
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_CAPACITY);
    CHECK(state.ds_6ddf == 7 && al == 0xEE);
    prepare(&state, &t, views, visited, table);
    state.view_count = 1;
    put_next(first_segment, first, second);
    CHECK(pr_u68_movement_flag(&state, &al) == PR_U68_MOVE_ARGUMENT);
    CHECK(state.ds_6ddf == 7 && al == 0xEE);
    CHECK(pr_u68_movement_flag(NULL, &al) == PR_U68_MOVE_ARGUMENT);
    CHECK(pr_u68_movement_flag(&state, NULL) == PR_U68_MOVE_ARGUMENT);

    puts("overlay_u68_movement_flag: ok");
    return 0;
}
