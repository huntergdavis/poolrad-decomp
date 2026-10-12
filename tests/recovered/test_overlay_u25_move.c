#include "poolrad/recovered/overlay_u25_move.h"

#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static void fill(uint8_t *buffer)
{
    unsigned i;
    for (i = 0; i < 0x400u; ++i) buffer[i] = (uint8_t)(i * 37u + 11u);
}

static int test_slot_a_step(void)
{
    uint8_t buffer[0x400];
    pr_u25_move_state s = {15, 4, 6, 0xA5, 0xA5, 1, buffer, 0x5A};
    unsigned index = 4u * 16u + 14u;
    fill(buffer);
    CHECK(pr_u25_entry19(&s) == PR_U25_MOVE_OK);
    CHECK(s.x == 14 && s.y == 4 && s.facing == 6);
    CHECK(s.cell == buffer[0x200u + index]);
    CHECK(s.nibble == (buffer[0x100u + index] & 0x0Fu));
    return 0;
}

static int test_direction_boundaries(void)
{
    static const struct { uint8_t x, y, facing, expected_x, expected_y; } cases[] = {
        {7, 0, 0, 7, 15}, {7, 5, 0, 7, 4},
        {15, 7, 2, 0, 7}, {5, 7, 2, 6, 7},
        {7, 15, 4, 7, 0}, {7, 5, 4, 7, 6},
        {0, 7, 6, 15, 7}, {5, 7, 6, 4, 7},
        {0xFF, 7, 6, 15, 7}, {0x80, 7, 2, 0x81, 7},
        {7, 0xFF, 0, 7, 15}, {7, 0x80, 4, 7, 0x81}
    };
    uint8_t buffer[0x400];
    unsigned i;
    fill(buffer);
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        pr_u25_move_state s = {cases[i].x, cases[i].y, cases[i].facing,
                                0xA5, 0xA5, 0, buffer, 0};
        CHECK(pr_u25_entry19(&s) == PR_U25_MOVE_OK);
        CHECK(s.x == cases[i].expected_x && s.y == cases[i].expected_y);
        CHECK(s.facing == cases[i].facing);
    }
    return 0;
}

static int test_lookup_and_error_order(void)
{
    uint8_t buffer[0x400];
    pr_u25_move_state s = {15, 4, 6, 0xA5, 0xA5, 0, buffer, 0x67};
    fill(buffer);
    /* A null buffer is first touched after the x write, so the x write remains. */
    s.map_buffer = 0;
    CHECK(pr_u25_entry19(&s) == PR_U25_MOVE_ARGUMENT);
    CHECK(s.x == 14 && s.y == 4 && s.cell == 0xA5 && s.nibble == 0xA5);

    /* Both nested lookups gate out-of-grid coordinates on maps 0 and 10. */
    s.x = 7; s.y = 0x7F; s.map_buffer = 0; s.cell = 0xA5; s.nibble = 0xA5;
    CHECK(pr_u25_entry19(&s) == PR_U25_MOVE_OK);
    CHECK(s.x == 6 && s.y == 0x7F && s.cell == 0 && s.nibble == 0);
    s.x = 7; s.y = 0x7F; s.map_id = 10;
    CHECK(pr_u25_entry19(&s) == PR_U25_MOVE_OK);
    CHECK(s.x == 6 && s.y == 0x7F && s.cell == 0 && s.nibble == 0);

    /* An unmatched facing does not move; entry 2 returns its frame residue. */
    s.x = 2; s.y = 3; s.facing = 1; s.map_id = 1; s.map_buffer = buffer;
    CHECK(pr_u25_entry19(&s) == PR_U25_MOVE_UNSPECIFIED);
    CHECK(s.x == 2 && s.y == 3);
    CHECK(s.cell == buffer[0x200u + 3u * 16u + 2u]);
    CHECK(s.nibble == 0x67);
    CHECK(pr_u25_entry19(0) == PR_U25_MOVE_ARGUMENT);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_slot_a_step();
    rc |= test_direction_boundaries();
    rc |= test_lookup_and_error_order();
    if (!rc) puts("unit 0x25 entry 19 movement tests passed");
    return rc;
}
