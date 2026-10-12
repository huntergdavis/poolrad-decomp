#include "poolrad/recovered/resident_464_mode0_two_y.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(uint8_t object_bytes[32], uint8_t source[0x19],
                  uint8_t lookup[64], int has_aux) {
    memset(object_bytes, 0, 32);
    memset(source, 0, 0x19);
    memset(lookup, 0, 64);
    object_bytes[0] = 1;
    object_bytes[2] = 2;
    object_bytes[8] = 1;
    object_bytes[9] = 0x01; /* Selector 1 reads this low nibble. */
    object_bytes[10] = 0x20; /* Selector 2 reads this high nibble. */
    if (has_aux) object_bytes[0x13] = 1;
    memset(source + 0x11, 0x11, 4);
    memset(source + 0x15, 0x22, 4);
    lookup[4] = 1; lookup[5] = 2;
    lookup[6] = 4; lookup[7] = 8;
    lookup[8] = 0x10; lookup[9] = 0x20;
    lookup[10] = 0x40; lookup[11] = 0x80;
}

static int test_two_rows_without_aux(void) {
    uint8_t object_bytes[32], source[0x19], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_two_y_report r;

    setup(object_bytes, source, lookup, 0);
    CHECK(pr_r464_mode0_two_y_pixels(
              source, sizeof source, 0x11, 0xA5, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_TWO_Y_RETURN);
    CHECK(r.pixel == 1 && r.stage == PR_R464_MODE0_TWO_Y_STAGE_RETURN);
    CHECK(r.loop[0].branch == PR_R464_MODE0_LOOP_BRANCH_Y &&
          r.loop[0].y == 1 && r.loop[0].next_offset == 0x0328 &&
          r.loop[0].source_offset == 0x15 &&
          r.loop[0].data_offset == 2);
    CHECK(r.prefix[1].source_offset == 0x15 &&
          r.prefix[1].data_offset == 2 && r.prefix[1].y == 1 &&
          r.prefix[1].move_calls == 1 &&
          r.prefix[1].copied[0] == 0x22);
    CHECK(r.first[1].data_offset == 3 &&
          r.second[1].data_offset == 4 &&
          r.second[1].source_offset == 0x19 &&
          r.loop[1].source_offset == 0x19 &&
          r.loop[1].branch == PR_R464_MODE0_LOOP_BRANCH_RETURN &&
          r.loop[1].next_offset == 0x08CE);
    CHECK(r.first[0].helper_calls == 4 &&
          r.second[0].helper_calls == 4 &&
          r.first[1].helper_calls == 4 &&
          r.second[1].helper_calls == 4);
    CHECK(object_bytes[0x17] == 0x0F && object_bytes[0x18] == 0x0F &&
          object_bytes[0x19] == 0xF0 && object_bytes[0x1A] == 0xF0);
    return 0;
}

static int test_two_rows_with_aux(void) {
    uint8_t object_bytes[32], source[0x19], lookup[64];
    uint8_t aux_bytes[4] = {0, 0, 0, 0};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {aux_bytes, sizeof aux_bytes};
    pr_r464_mode0_two_y_report r;

    setup(object_bytes, source, lookup, 1);
    CHECK(pr_r464_mode0_two_y_pixels(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_TWO_Y_RETURN);
    CHECK(r.prefix[0].bitmask == 0xFF &&
          r.prefix[0].auxiliary_index == 2 &&
          r.prefix[0].aux_or_writes == 8 &&
          r.prefix[1].bitmask == 0 &&
          r.prefix[1].auxiliary_index == 4 &&
          r.prefix[1].aux_or_writes == 0 &&
          r.loop[1].auxiliary_index == 4);
    CHECK(aux_bytes[0] == 0x0F && aux_bytes[1] == 0x0F &&
          aux_bytes[2] == 0 && aux_bytes[3] == 0);
    CHECK(object_bytes[0x17] == 0 && object_bytes[0x18] == 0 &&
          object_bytes[0x19] == 0xF0 && object_bytes[0x1A] == 0xF0);
    return 0;
}

static int test_shape_and_second_move_stop(void) {
    uint8_t object_bytes[32], source[0x19], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_two_y_report r;

    setup(object_bytes, source, lookup, 0);
    object_bytes[2] = 1;
    CHECK(pr_r464_mode0_two_y_pixels(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_TWO_Y_SHAPE);
    CHECK(r.stage == PR_R464_MODE0_TWO_Y_STAGE_SHAPE &&
          r.prefix[0].move_calls == 0 && object_bytes[0x17] == 0);

    setup(object_bytes, source, lookup, 0);
    CHECK(pr_r464_mode0_two_y_pixels(
              source, 0x18, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_TWO_Y_PREFIX_STOP);
    CHECK(r.pixel == 1 && r.stage == PR_R464_MODE0_TWO_Y_STAGE_PREFIX &&
          r.component_status == PR_R464_MODE0_BOUNDS &&
          r.prefix[1].stopped_at == PR_R464_MODE0_STOP_MOVE &&
          r.prefix[1].source_offset == 0x15 &&
          r.prefix[1].data_offset == 2 &&
          r.prefix[1].move_calls == 0 &&
          object_bytes[0x17] == 0x0F && object_bytes[0x18] == 0x0F &&
          object_bytes[0x19] == 0);
    return 0;
}

static int test_second_row_late_stops(void) {
    uint8_t object_bytes[32], source[0x19], lookup[64];
    uint8_t aux_bytes[3] = {0, 0, 0};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {aux_bytes, sizeof aux_bytes};
    pr_r464_mode0_two_y_report r;

    setup(object_bytes, source, lookup, 0);
    CHECK(pr_r464_mode0_two_y_pixels(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, 10, &r) ==
          PR_R464_MODE0_TWO_Y_FIRST_STOP);
    CHECK(r.pixel == 1 && r.first[1].stopped_at ==
          PR_R464_MODE0_PRIMARY_STOP_TABLE &&
          r.first[1].primary_or_writes == 2 &&
          r.first[1].data_offset == 2 &&
          object_bytes[0x17] == 0x0F && object_bytes[0x18] == 0x0F &&
          object_bytes[0x19] == 0x30 && object_bytes[0x1A] == 0);

    setup(object_bytes, source, lookup, 0);
    object.capacity = 0x1A;
    CHECK(pr_r464_mode0_two_y_pixels(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_TWO_Y_SECOND_STOP);
    CHECK(r.pixel == 1 && r.second[1].stopped_at ==
          PR_R464_MODE0_SECOND_STOP_OBJECT_OR &&
          r.second[1].primary_or_writes == 0 &&
          object_bytes[0x17] == 0x0F && object_bytes[0x18] == 0x0F &&
          object_bytes[0x19] == 0xF0 && object_bytes[0x1A] == 0);
    object.capacity = sizeof object_bytes;

    setup(object_bytes, source, lookup, 1);
    CHECK(pr_r464_mode0_two_y_pixels(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_TWO_Y_SECOND_STOP);
    CHECK(r.pixel == 1 && r.second[1].stopped_at ==
          PR_R464_MODE0_SECOND_STOP_AUX_READ &&
          r.second[1].primary_or_writes == 4 &&
          r.second[1].data_offset == 3 &&
          r.prefix[1].auxiliary_index == 4 &&
          aux_bytes[0] == 0x0F && aux_bytes[1] == 0x0F &&
          object_bytes[0x17] == 0 && object_bytes[0x18] == 0 &&
          object_bytes[0x19] == 0xF0 && object_bytes[0x1A] == 0xF0);
    return 0;
}

static int test_second_row_auxiliary_stop(void) {
    uint8_t object_bytes[32], source[0x19], lookup[64];
    uint8_t aux_bytes[2] = {0, 0};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {aux_bytes, sizeof aux_bytes};
    pr_r464_mode0_two_y_report r;

    setup(object_bytes, source, lookup, 1);
    source[0x15] = 0x11; /* Second mask reaches auxiliary index two. */
    CHECK(pr_r464_mode0_two_y_pixels(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_TWO_Y_PREFIX_STOP);
    CHECK(r.pixel == 1 && r.stage == PR_R464_MODE0_TWO_Y_STAGE_PREFIX &&
          r.prefix[1].stopped_at == PR_R464_MODE0_STOP_AUX_READ &&
          r.prefix[1].move_calls == 1 &&
          r.prefix[1].auxiliary_index == 2 &&
          r.prefix[1].aux_or_writes == 0 &&
          aux_bytes[0] == 0x0F && aux_bytes[1] == 0x0F &&
          object_bytes[0x17] == 0 && object_bytes[0x18] == 0 &&
          object_bytes[0x19] == 0);
    return 0;
}

int main(void) {
    if (test_two_rows_without_aux()) return 1;
    if (test_two_rows_with_aux()) return 1;
    if (test_shape_and_second_move_stop()) return 1;
    if (test_second_row_late_stops()) return 1;
    if (test_second_row_auxiliary_stop()) return 1;
    return 0;
}
