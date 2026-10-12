#include "poolrad/recovered/resident_464_mode0_loops.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(uint8_t *object_bytes, size_t size,
                  uint16_t width, uint16_t height, uint8_t count,
                  uint8_t source[0x15], uint8_t lookup[64]) {
    memset(object_bytes, 0, size);
    memset(source, 0, 0x15);
    memset(lookup, 0, 64);
    object_bytes[0] = (uint8_t)width;
    object_bytes[1] = (uint8_t)(width >> 8);
    object_bytes[2] = (uint8_t)height;
    object_bytes[3] = (uint8_t)(height >> 8);
    object_bytes[8] = count;
    object_bytes[9] = 0x11; /* Nibble helper returns AL=1. */
    memset(source + 0x11, 0x11, 4);
    lookup[4] = 1; lookup[5] = 2;
    lookup[6] = 4; lookup[7] = 8;
}

static int complete_first_pixel(
    uint8_t source[0x15], const uint8_t lookup[64],
    pr_r464_block_object *object, pr_r464_mode0_report *prefix,
    pr_r464_mode0_second_report *second) {
    pr_r464_mode0_primary_report first;
    if (pr_r464_mode0_first_pixel_prefix(
            source, 0x15, 0x11, 0, 0, object, NULL, NULL, 0,
            prefix) != PR_R464_MODE0_READY_PRIMARY)
        return 1;
    if (pr_r464_mode0_first_primary_byte(
            prefix, object, NULL, lookup, 64,
            &first) != PR_R464_MODE0_PRIMARY_READY_SECOND)
        return 1;
    return pr_r464_mode0_second_primary_byte(
        prefix, &first, object, NULL, lookup, 64, second) !=
        PR_R464_MODE0_SECOND_READY_LOOPS;
}

static int test_four_branch_outcomes(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_report prefix;
    pr_r464_mode0_second_report second;
    pr_r464_mode0_loop_report r;

    setup(object_bytes, sizeof object_bytes, 1, 2, 1, source, lookup);
    CHECK(!complete_first_pixel(source, lookup, &object,
                                &prefix, &second));
    CHECK(pr_r464_mode0_after_first_pixel(
              &prefix, &second, NULL, &r) ==
          PR_R464_MODE0_LOOP_NEXT_PIXEL);
    CHECK(r.branch == PR_R464_MODE0_LOOP_BRANCH_Y &&
          r.x == 0 && r.y == 1 && r.layer == 1 &&
          r.width_end == 0 && r.height_end == 1 &&
          r.source_offset == 0x15 && r.data_offset == 2 &&
          r.next_offset == 0x0328 &&
          r.stopped_at == PR_R464_MODE0_LOOP_STOP_NEXT_MOVE);

    setup(object_bytes, sizeof object_bytes, 2, 1, 1, source, lookup);
    CHECK(!complete_first_pixel(source, lookup, &object,
                                &prefix, &second));
    CHECK(pr_r464_mode0_after_first_pixel(
              &prefix, &second, &object, &r) ==
          PR_R464_MODE0_LOOP_NEXT_PIXEL);
    CHECK(r.branch == PR_R464_MODE0_LOOP_BRANCH_X &&
          r.x == 1 && r.y == 0 && r.layer == 1 &&
          r.width_end == 1 && r.height_end == 0 &&
          r.next_offset == 0x0328);

    setup(object_bytes, sizeof object_bytes, 1, 1, 2, source, lookup);
    CHECK(!complete_first_pixel(source, lookup, &object,
                                &prefix, &second));
    object_bytes[8] = 1; /* Gate uses the saved entry count, still two. */
    CHECK(pr_r464_mode0_after_first_pixel(
              &prefix, &second, &object, &r) ==
          PR_R464_MODE0_LOOP_NEXT_PIXEL);
    CHECK(r.branch == PR_R464_MODE0_LOOP_BRANCH_LAYER &&
          r.x == 0 && r.y == 0 && r.layer == 2 && r.count == 2 &&
          r.next_offset == 0x0328);

    setup(object_bytes, sizeof object_bytes, 1, 1, 1, source, lookup);
    CHECK(!complete_first_pixel(source, lookup, &object,
                                &prefix, &second));
    CHECK(pr_r464_mode0_after_first_pixel(
              &prefix, &second, NULL, &r) ==
          PR_R464_MODE0_LOOP_RETURN);
    CHECK(r.branch == PR_R464_MODE0_LOOP_BRANCH_RETURN &&
          r.next_offset == 0x08CE &&
          r.stopped_at == PR_R464_MODE0_LOOP_STOP_COMMON_RETURN &&
          r.source_offset == 0x15 && r.data_offset == 2);

    setup(object_bytes, sizeof object_bytes, 0, 0, 1, source, lookup);
    CHECK(!complete_first_pixel(source, lookup, &object,
                                &prefix, &second));
    CHECK(pr_r464_mode0_after_first_pixel(
              &prefix, &second, NULL, &r) ==
          PR_R464_MODE0_LOOP_NEXT_PIXEL);
    CHECK(r.branch == PR_R464_MODE0_LOOP_BRANCH_Y &&
          r.width_end == 0xFFFF && r.height_end == 0xFFFF &&
          r.y == 1 && r.next_offset == 0x0328);
    return 0;
}

static int test_reached_header_stops(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_report prefix;
    pr_r464_mode0_second_report second;
    pr_r464_mode0_loop_report r;

    setup(object_bytes, sizeof object_bytes, 2, 1, 1, source, lookup);
    CHECK(!complete_first_pixel(source, lookup, &object,
                                &prefix, &second));
    object.capacity = 3; /* Host mapping ends before height WORD. */
    CHECK(pr_r464_mode0_after_first_pixel(
              &prefix, &second, &object, &r) ==
          PR_R464_MODE0_LOOP_BOUNDS);
    CHECK(r.branch == PR_R464_MODE0_LOOP_BRANCH_X &&
          r.x == 1 && r.y == 0 &&
          r.stopped_at == PR_R464_MODE0_LOOP_STOP_HEIGHT_RELOAD &&
          r.next_offset == 0 && r.source_offset == 0x15);
    object.capacity = sizeof object_bytes;

    setup(object_bytes, sizeof object_bytes, 1, 1, 2, source, lookup);
    CHECK(!complete_first_pixel(source, lookup, &object,
                                &prefix, &second));
    object.capacity = 1; /* Layer increments before width is reached. */
    CHECK(pr_r464_mode0_after_first_pixel(
              &prefix, &second, &object, &r) ==
          PR_R464_MODE0_LOOP_BOUNDS);
    CHECK(r.branch == PR_R464_MODE0_LOOP_BRANCH_LAYER &&
          r.layer == 2 && r.width_end == 0 &&
          r.stopped_at == PR_R464_MODE0_LOOP_STOP_WIDTH_RELOAD);

    object.capacity = 2;
    object_bytes[0] = 3; /* Reached width reread precedes height. */
    CHECK(pr_r464_mode0_after_first_pixel(
              &prefix, &second, &object, &r) ==
          PR_R464_MODE0_LOOP_BOUNDS);
    CHECK(r.layer == 2 && r.width_end == 2 && r.x == 0 &&
          r.stopped_at == PR_R464_MODE0_LOOP_STOP_HEIGHT_RELOAD);
    return 0;
}

int main(void) {
    if (test_four_branch_outcomes()) return 1;
    if (test_reached_header_stops()) return 1;
    return 0;
}
