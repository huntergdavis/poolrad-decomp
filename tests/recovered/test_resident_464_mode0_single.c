#include "poolrad/recovered/resident_464_mode0_single.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(uint8_t *object_bytes, size_t size, int has_aux,
                  uint8_t source[0x15], uint8_t lookup[64]) {
    memset(object_bytes, 0, size);
    memset(source, 0, 0x15);
    memset(lookup, 0, 64);
    object_bytes[0] = 1;
    object_bytes[2] = 1;
    object_bytes[8] = 1;
    object_bytes[9] = 0x11; /* Helper selector 1 returns AL=1. */
    if (has_aux) object_bytes[0x13] = 1;
    memset(source + 0x11, 0x11, 4);
    lookup[4] = 1; lookup[5] = 2;
    lookup[6] = 4; lookup[7] = 8;
}

static int test_single_pixel_return_without_aux(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_single_report r;

    setup(object_bytes, sizeof object_bytes, 0, source, lookup);
    object_bytes[0x17] = 0x40;
    object_bytes[0x18] = 0x80;
    CHECK(pr_r464_mode0_single_pixel(
              source, sizeof source, 0x11, 0xA5, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_SINGLE_RETURN);
    CHECK(r.stage == PR_R464_MODE0_SINGLE_STAGE_RETURN &&
          r.component_status == PR_R464_MODE0_LOOP_RETURN &&
          r.prefix.move_calls == 1 && r.prefix.bitmask_calls == 0 &&
          r.first.helper_calls == 4 && r.second.helper_calls == 4 &&
          r.first.primary_or_writes == 4 &&
          r.second.primary_or_writes == 4 &&
          r.first.primary_and_writes == 0 &&
          r.second.primary_and_writes == 0 &&
          r.loop.branch == PR_R464_MODE0_LOOP_BRANCH_RETURN &&
          r.loop.source_offset == 0x15 && r.loop.data_offset == 2 &&
          r.loop.next_offset == 0x08CE);
    CHECK(object_bytes[0x17] == 0x4F &&
          object_bytes[0x18] == 0x8F);
    return 0;
}

static int test_single_pixel_return_with_aux(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    uint8_t auxiliary_bytes[2] = {0, 0};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {
        auxiliary_bytes, sizeof auxiliary_bytes
    };
    pr_r464_mode0_single_report r;

    setup(object_bytes, sizeof object_bytes, 1, source, lookup);
    object_bytes[0x17] = 0x80;
    object_bytes[0x18] = 0x80;
    CHECK(pr_r464_mode0_single_pixel(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_SINGLE_RETURN);
    CHECK(r.prefix.bitmask == 0xFF && r.prefix.bitmask_calls == 1 &&
          r.prefix.aux_or_writes == 8 &&
          r.first.primary_and_writes == 1 &&
          r.second.primary_and_writes == 1 &&
          auxiliary_bytes[0] == 0x0F &&
          auxiliary_bytes[1] == 0x0F &&
          object_bytes[0x17] == 0x80 &&
          object_bytes[0x18] == 0x80 &&
          r.loop.next_offset == 0x08CE);
    return 0;
}

static int test_shape_and_early_stops(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    uint8_t auxiliary_bytes[2] = {0, 0};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {
        auxiliary_bytes, sizeof auxiliary_bytes
    };
    pr_r464_mode0_single_report r;

    setup(object_bytes, sizeof object_bytes, 0, source, lookup);
    object_bytes[8] = 2;
    object_bytes[0x17] = 0x50;
    CHECK(pr_r464_mode0_single_pixel(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_SINGLE_SHAPE);
    CHECK(r.stage == PR_R464_MODE0_SINGLE_STAGE_SHAPE &&
          r.prefix.move_calls == 0 && object_bytes[0x17] == 0x50);

    setup(object_bytes, sizeof object_bytes, 0, source, lookup);
    CHECK(pr_r464_mode0_single_pixel(
              source, 0x14, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_SINGLE_PREFIX_STOP);
    CHECK(r.stage == PR_R464_MODE0_SINGLE_STAGE_PREFIX &&
          r.component_status == PR_R464_MODE0_BOUNDS &&
          r.prefix.stopped_at == PR_R464_MODE0_STOP_MOVE &&
          r.first.helper_calls == 0);

    setup(object_bytes, sizeof object_bytes, 1, source, lookup);
    CHECK(pr_r464_mode0_single_pixel(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, 1, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_SINGLE_PREFIX_STOP);
    CHECK(r.prefix.stopped_at == PR_R464_MODE0_STOP_AUX_TABLE &&
          r.prefix.move_calls == 1 &&
          r.prefix.aux_or_writes == 1 && auxiliary_bytes[0] == 1 &&
          r.first.helper_calls == 0);
    return 0;
}

static int test_late_stops_keep_prior_effects(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    uint8_t auxiliary_byte[1] = {1};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {
        auxiliary_byte, sizeof auxiliary_byte
    };
    pr_r464_mode0_single_report r;

    setup(object_bytes, sizeof object_bytes, 0, source, lookup);
    CHECK(pr_r464_mode0_single_pixel(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, 6, &r) ==
          PR_R464_MODE0_SINGLE_FIRST_STOP);
    CHECK(r.stage == PR_R464_MODE0_SINGLE_STAGE_FIRST &&
          r.component_status == PR_R464_MODE0_PRIMARY_TABLE &&
          r.first.stopped_at == PR_R464_MODE0_PRIMARY_STOP_TABLE &&
          r.first.primary_or_writes == 2 &&
          object_bytes[0x17] == 3 &&
          r.second.helper_calls == 0 &&
          r.prefix.source_offset == 0x11);

    setup(object_bytes, sizeof object_bytes, 0, source, lookup);
    object.capacity = 0x18; /* First destination mapped; second absent. */
    CHECK(pr_r464_mode0_single_pixel(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_SINGLE_SECOND_STOP);
    CHECK(r.stage == PR_R464_MODE0_SINGLE_STAGE_SECOND &&
          r.component_status == PR_R464_MODE0_SECOND_BOUNDS &&
          r.first.primary_or_writes == 4 &&
          r.second.stopped_at == PR_R464_MODE0_SECOND_STOP_OBJECT_OR &&
          r.second.primary_or_writes == 0 &&
          object_bytes[0x17] == 0x0F &&
          r.second.source_offset == 0x11);
    object.capacity = sizeof object_bytes;

    setup(object_bytes, sizeof object_bytes, 1, source, lookup);
    CHECK(pr_r464_mode0_single_pixel(
              source, sizeof source, 0x11, 0, 0, &object, &auxiliary,
              NULL, 0, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_SINGLE_SECOND_STOP);
    CHECK(r.prefix.auxiliary_index == 2 &&
          r.first.primary_and_writes == 1 &&
          r.second.primary_or_writes == 4 &&
          r.second.stopped_at == PR_R464_MODE0_SECOND_STOP_AUX_READ &&
          object_bytes[0x17] == 0x0E &&
          object_bytes[0x18] == 0x0F &&
          r.second.source_offset == 0x11);
    return 0;
}

int main(void) {
    if (test_single_pixel_return_without_aux()) return 1;
    if (test_single_pixel_return_with_aux()) return 1;
    if (test_shape_and_early_stops()) return 1;
    if (test_late_stops_keep_prior_effects()) return 1;
    return 0;
}
