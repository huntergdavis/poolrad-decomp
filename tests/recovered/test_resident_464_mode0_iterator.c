#include "poolrad/recovered/resident_464_mode0_iterator.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(uint8_t object_bytes[32], uint8_t source[0x21],
                  uint8_t lookup[64], uint8_t width, uint8_t height,
                  uint8_t count, int has_aux) {
    memset(object_bytes, 0, 32);
    memset(source, 0, 0x21);
    memset(lookup, 0, 64);
    object_bytes[0] = width;
    object_bytes[2] = height;
    object_bytes[8] = count;
    object_bytes[9] = 0x01;
    object_bytes[10] = 0x20;
    if (has_aux) object_bytes[0x13] = 1;
    memset(source + 0x11, 0x11, 4);
    memset(source + 0x15, 0x22, 4);
    memset(source + 0x19, 0x11, 4);
    memset(source + 0x1D, 0x22, 4);
    lookup[4] = 1; lookup[5] = 2;
    lookup[6] = 4; lookup[7] = 8;
    lookup[8] = 0x10; lookup[9] = 0x20;
    lookup[10] = 0x40; lookup[11] = 0x80;
}

static int test_backedge(uint8_t width, uint8_t height, uint8_t count,
                         pr_r464_mode0_loop_branch branch) {
    uint8_t object_bytes[32], source[0x21], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_pixel_report pixels[2];
    pr_r464_mode0_iter_report r;

    setup(object_bytes, source, lookup, width, height, count, 0);
    CHECK(pr_r464_mode0_iterate(
              source, sizeof source, 0x11, 0xA5, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, 2, pixels, 2, &r) ==
          PR_R464_MODE0_ITER_RETURN);
    CHECK(r.stage == PR_R464_MODE0_ITER_STAGE_RETURN &&
          r.pixel_index == 1 && r.records_used == 2 &&
          r.completed_pixels == 2 && r.next_offset == 0x08CE);
    CHECK(pixels[0].loop.branch == branch &&
          pixels[0].loop.next_offset == 0x0328 &&
          pixels[0].loop.source_offset == 0x15 &&
          pixels[0].loop.data_offset == 2 &&
          pixels[1].prefix.source_offset == 0x15 &&
          pixels[1].prefix.data_offset == 2 &&
          pixels[1].prefix.copied[0] == 0x22 &&
          pixels[1].second.source_offset == 0x19 &&
          pixels[1].second.data_offset == 4 &&
          pixels[1].loop.branch == PR_R464_MODE0_LOOP_BRANCH_RETURN);
    if (branch == PR_R464_MODE0_LOOP_BRANCH_Y)
        CHECK(pixels[1].prefix.x == 0 && pixels[1].prefix.y == 1 &&
              pixels[1].prefix.layer == 1);
    if (branch == PR_R464_MODE0_LOOP_BRANCH_X)
        CHECK(pixels[1].prefix.x == 1 && pixels[1].prefix.y == 0 &&
              pixels[1].prefix.layer == 1 &&
              pixels[1].prefix.height_end == 0);
    if (branch == PR_R464_MODE0_LOOP_BRANCH_LAYER)
        CHECK(pixels[1].prefix.x == 0 && pixels[1].prefix.y == 0 &&
              pixels[1].prefix.layer == 2 &&
              pixels[1].prefix.width_end == 0 &&
              pixels[1].prefix.height_end == 0);
    CHECK(object_bytes[0x17] == 0x0F && object_bytes[0x18] == 0x0F &&
          object_bytes[0x19] == 0xF0 && object_bytes[0x1A] == 0xF0);
    return 0;
}

static int test_nested_y_and_x(void) {
    uint8_t object_bytes[32], source[0x21], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_pixel_report pixels[4];
    pr_r464_mode0_iter_report r;

    setup(object_bytes, source, lookup, 2, 2, 1, 0);
    CHECK(pr_r464_mode0_iterate(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, 4, pixels, 4, &r) ==
          PR_R464_MODE0_ITER_RETURN);
    CHECK(r.completed_pixels == 4 && r.next_offset == 0x08CE &&
          pixels[0].loop.branch == PR_R464_MODE0_LOOP_BRANCH_Y &&
          pixels[1].loop.branch == PR_R464_MODE0_LOOP_BRANCH_X &&
          pixels[2].loop.branch == PR_R464_MODE0_LOOP_BRANCH_Y &&
          pixels[3].loop.branch == PR_R464_MODE0_LOOP_BRANCH_RETURN &&
          pixels[3].loop.data_offset == 8 &&
          pixels[3].loop.source_offset == 0x21);
    CHECK(object_bytes[0x17] == 0x0F && object_bytes[0x18] == 0x0F &&
          object_bytes[0x19] == 0xF0 && object_bytes[0x1A] == 0xF0 &&
          object_bytes[0x1B] == 0x0F && object_bytes[0x1C] == 0x0F &&
          object_bytes[0x1D] == 0xF0 && object_bytes[0x1E] == 0xF0);
    return 0;
}

static int test_budget_and_no_work(void) {
    uint8_t object_bytes[32], source[0x21], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_pixel_report pixels[1];
    pr_r464_mode0_iter_report r;

    setup(object_bytes, source, lookup, 2, 1, 1, 0);
    CHECK(pr_r464_mode0_iterate(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, 0, NULL, 0, &r) ==
          PR_R464_MODE0_ITER_BUDGET);
    CHECK(r.stage == PR_R464_MODE0_ITER_STAGE_BUDGET &&
          r.records_used == 0 && r.completed_pixels == 0 &&
          r.next_offset == 0x02D3 && object_bytes[0x17] == 0);
    CHECK(pr_r464_mode0_iterate(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, 2, pixels, 1, &r) ==
          PR_R464_MODE0_ITER_ARGUMENT);
    CHECK(object_bytes[0x17] == 0);

    CHECK(pr_r464_mode0_iterate(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, 1, pixels, 1, &r) ==
          PR_R464_MODE0_ITER_BUDGET);
    CHECK(r.stage == PR_R464_MODE0_ITER_STAGE_BUDGET &&
          r.completed_pixels == 1 && r.records_used == 1 &&
          r.next_offset == 0x0328 &&
          pixels[0].loop.branch == PR_R464_MODE0_LOOP_BRANCH_X &&
          object_bytes[0x17] == 0x0F && object_bytes[0x18] == 0x0F &&
          object_bytes[0x19] == 0);

    setup(object_bytes, source, lookup, 1, 1, 1, 0);
    CHECK(pr_r464_mode0_iterate(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, 1, pixels, 1, &r) ==
          PR_R464_MODE0_ITER_RETURN);
    CHECK(r.completed_pixels == 1 && r.next_offset == 0x08CE &&
          pixels[0].loop.branch == PR_R464_MODE0_LOOP_BRANCH_RETURN);

    setup(object_bytes, source, lookup, 1, 1, 0, 0);
    CHECK(pr_r464_mode0_iterate(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, 1, pixels, 1, &r) ==
          PR_R464_MODE0_ITER_NO_WORK);
    CHECK(r.stage == PR_R464_MODE0_ITER_STAGE_RETURN &&
          r.records_used == 1 && r.completed_pixels == 0 &&
          r.next_offset == 0x08CE &&
          pixels[0].prefix.move_calls == 0 &&
          object_bytes[0x17] == 0);
    return 0;
}

static int test_auxiliary_carry_and_reached_stop(void) {
    uint8_t object_bytes[32], source[0x21], lookup[64];
    uint8_t aux_bytes[4] = {0, 0, 0, 0};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {aux_bytes, sizeof aux_bytes};
    pr_r464_mode0_pixel_report pixels[2];
    pr_r464_mode0_iter_report r;

    setup(object_bytes, source, lookup, 1, 1, 2, 1);
    CHECK(pr_r464_mode0_iterate(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, lookup, sizeof lookup,
              2, pixels, 2, &r) == PR_R464_MODE0_ITER_RETURN);
    CHECK(pixels[0].loop.branch == PR_R464_MODE0_LOOP_BRANCH_LAYER &&
          pixels[0].loop.auxiliary_index == 2 &&
          pixels[1].prefix.auxiliary_index == 4 &&
          pixels[1].loop.auxiliary_index == 4 &&
          aux_bytes[0] == 0x0F && aux_bytes[1] == 0x0F &&
          aux_bytes[2] == 0 && aux_bytes[3] == 0 &&
          object_bytes[0x17] == 0 && object_bytes[0x18] == 0 &&
          object_bytes[0x19] == 0xF0 && object_bytes[0x1A] == 0xF0);

    setup(object_bytes, source, lookup, 2, 1, 1, 0);
    CHECK(pr_r464_mode0_iterate(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, 10, 2, pixels, 2, &r) ==
          PR_R464_MODE0_ITER_FIRST_STOP);
    CHECK(r.pixel_index == 1 && r.records_used == 2 &&
          r.completed_pixels == 1 &&
          r.stage == PR_R464_MODE0_ITER_STAGE_FIRST &&
          r.component_status == PR_R464_MODE0_PRIMARY_TABLE &&
          pixels[1].first.stopped_at == PR_R464_MODE0_PRIMARY_STOP_TABLE &&
          pixels[1].first.primary_or_writes == 2 &&
          object_bytes[0x17] == 0x0F && object_bytes[0x18] == 0x0F &&
          object_bytes[0x19] == 0x30 && object_bytes[0x1A] == 0);
    return 0;
}

int main(void) {
    if (test_backedge(1, 2, 1, PR_R464_MODE0_LOOP_BRANCH_Y)) return 1;
    if (test_backedge(2, 1, 1, PR_R464_MODE0_LOOP_BRANCH_X)) return 1;
    if (test_backedge(1, 1, 2, PR_R464_MODE0_LOOP_BRANCH_LAYER)) return 1;
    if (test_nested_y_and_x()) return 1;
    if (test_budget_and_no_work()) return 1;
    if (test_auxiliary_carry_and_reached_stop()) return 1;
    return 0;
}
