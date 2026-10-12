#include "poolrad/recovered/resident_464_mode0_prefix.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(uint8_t *bytes, size_t size, uint8_t count,
                  int has_aux) {
    memset(bytes, 0, size);
    bytes[8] = count;
    if (has_aux) bytes[0x13] = 1; /* Synthetic far-pointer token. */
}

static int test_no_work_and_no_aux(void) {
    uint8_t object_bytes[32], source[0x15] = {0};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_report r;

    setup(object_bytes, sizeof object_bytes, 0, 0);
    object.capacity = 9;
    CHECK(pr_r464_mode0_first_pixel_prefix(
              NULL, 0, 0x11, 0xB7, 1, &object, NULL, NULL, 0, &r) ==
          PR_R464_MODE0_NO_WORK);
    CHECK(r.stopped_at == PR_R464_MODE0_STOP_RETURN &&
          r.next_offset == 0x08CE && r.move_calls == 0 &&
          r.source_offset == 0x11);

    object.capacity = sizeof object_bytes;
    setup(object_bytes, sizeof object_bytes, 1, 0);
    /* Both DOS WORD decrements underflow; the first pixel still runs. */
    source[0x11] = 0x12; source[0x12] = 0x34;
    source[0x13] = 0x56; source[0x14] = 0x78;
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0xB7, 1, &object, NULL,
              NULL, 0, &r) == PR_R464_MODE0_READY_PRIMARY);
    CHECK(r.stopped_at == PR_R464_MODE0_STOP_PRIMARY_HELPERS &&
          r.next_offset == 0x03D1 && r.layer == 1 &&
          r.width_end == 0xFFFF && r.height_end == 0xFFFF &&
          r.x == 0 && r.y == 0 &&
          r.move_calls == 1 && r.bitmask_calls == 0 &&
          r.auxiliary_index == 0 && r.data_offset == 0 &&
          r.source_offset == 0x11 &&
          memcmp(r.copied, source + 0x11, 4) == 0);
    return 0;
}

static int test_auxiliary_or_and_zero_mask(void) {
    uint8_t object_bytes[32], source[0x15] = {0};
    uint8_t auxiliary_bytes[2] = {0x10, 0x20};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {
        auxiliary_bytes, sizeof auxiliary_bytes
    };
    pr_r464_mode0_report r;

    setup(object_bytes, sizeof object_bytes, 1, 1);
    source[0x11] = 0x11; source[0x12] = 0x21;
    source[0x13] = 0x31; source[0x14] = 0x41;
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, &r) ==
          PR_R464_MODE0_READY_PRIMARY);
    CHECK(r.bitmask == 0xD5 && r.bitmask_calls == 1 &&
          r.move_calls == 1 && r.table_reads == 5 &&
          r.aux_or_writes == 5 && r.auxiliary_index == 2 &&
          auxiliary_bytes[0] == 0x1B && auxiliary_bytes[1] == 0x2A);

    /* A present far-pointer gate calls the bitmask helper, but zero bits
     * do not read the table or auxiliary mapping. Both indices advance. */
    source[0x11] = source[0x12] = source[0x13] = source[0x14] = 0x11;
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, &r) == PR_R464_MODE0_READY_PRIMARY);
    CHECK(r.bitmask == 0 && r.bitmask_calls == 1 &&
          r.table_reads == 0 && r.aux_or_writes == 0 &&
          r.auxiliary_index == 2);
    return 0;
}

static int test_reached_stops(void) {
    uint8_t object_bytes[32], source[0x15] = {0};
    uint8_t auxiliary_bytes[1] = {0x10};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {
        auxiliary_bytes, sizeof auxiliary_bytes
    };
    pr_r464_mode0_report r;

    setup(object_bytes, sizeof object_bytes, 1, 1);
    memset(source + 0x11, 0x11, 4);
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, 0x14, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, &r) == PR_R464_MODE0_BOUNDS);
    CHECK(r.stopped_at == PR_R464_MODE0_STOP_MOVE &&
          r.move_calls == 0 && r.bitmask_calls == 0);

    object.capacity = 9;
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, &r) == PR_R464_MODE0_BOUNDS);
    CHECK(r.stopped_at == PR_R464_MODE0_STOP_HEADER &&
          r.move_calls == 1 && r.bitmask_calls == 0 &&
          r.copied[0] == 0x11);
    object.capacity = sizeof object_bytes;

    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              NULL, 0, &r) == PR_R464_MODE0_TABLE);
    CHECK(r.stopped_at == PR_R464_MODE0_STOP_AUX_TABLE &&
          r.move_calls == 1 && r.bitmask_calls == 1 &&
          r.table_reads == 0 && auxiliary_bytes[0] == 0x10);

    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 1, &object, NULL,
              pattern, sizeof pattern, &r) == PR_R464_MODE0_AUX_MAPPING);
    CHECK(r.stopped_at == PR_R464_MODE0_STOP_AUX_READ &&
          r.table_reads == 1 && r.aux_or_writes == 0);

    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, &r) == PR_R464_MODE0_BOUNDS);
    CHECK(r.stopped_at == PR_R464_MODE0_STOP_AUX_READ &&
          r.auxiliary_index == 1 && r.table_reads == 5 &&
          r.aux_or_writes == 4 && auxiliary_bytes[0] == 0x1F);
    return 0;
}

int main(void) {
    if (test_no_work_and_no_aux()) return 1;
    if (test_auxiliary_or_and_zero_mask()) return 1;
    if (test_reached_stops()) return 1;
    return 0;
}
