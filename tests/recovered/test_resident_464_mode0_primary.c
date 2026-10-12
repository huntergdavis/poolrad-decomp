#include "poolrad/recovered/resident_464_mode0_primary.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(uint8_t *object_bytes, size_t capacity, int has_aux,
                  uint8_t *source, size_t source_size, uint8_t *lookup) {
    memset(object_bytes, 0, capacity);
    memset(source, 0, source_size);
    memset(lookup, 0, 64);
    object_bytes[8] = 1;
    object_bytes[9] = 0x11; /* Helper selector 1 returns low nibble 1. */
    if (has_aux) object_bytes[0x13] = 1;
    memset(source + 0x11, 0x11, 4);
    lookup[4] = 1; lookup[5] = 2;
    lookup[6] = 4; lookup[7] = 8;
}

static int test_primary_or_without_aux(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_report prefix;
    pr_r464_mode0_primary_report r;

    setup(object_bytes, sizeof object_bytes, 0, source,
          sizeof source, lookup);
    object_bytes[0x17] = 0x40;
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, &prefix) == PR_R464_MODE0_READY_PRIMARY);
    CHECK(pr_r464_mode0_first_primary_byte(
              &prefix, &object, NULL, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_PRIMARY_READY_SECOND);
    CHECK(object_bytes[0x17] == 0x4F &&
          r.helper_calls == 4 && r.table_reads == 4 &&
          r.primary_or_writes == 4 && r.primary_and_writes == 0 &&
          r.data_offset == 1 && r.source_offset == 0x11 &&
          r.primary_index == 4 && r.next_offset == 0x04CE &&
          r.stopped_at == PR_R464_MODE0_PRIMARY_STOP_SECOND_GROUP);
    return 0;
}

static int test_primary_and_after_aux_or(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    uint8_t aux_bytes[2] = {0, 0};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {aux_bytes, sizeof aux_bytes};
    pr_r464_mode0_report prefix;
    pr_r464_mode0_primary_report r;

    setup(object_bytes, sizeof object_bytes, 1, source,
          sizeof source, lookup);
    object_bytes[0x17] = 0x80;
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 1, &object, &auxiliary,
              pattern, sizeof pattern, &prefix) ==
          PR_R464_MODE0_READY_PRIMARY);
    CHECK(prefix.bitmask == 0xFF && prefix.auxiliary_index == 2 &&
          aux_bytes[0] == 0x0F && aux_bytes[1] == 0x0F);
    CHECK(pr_r464_mode0_first_primary_byte(
              &prefix, &object, &auxiliary, lookup, sizeof lookup,
              &r) == PR_R464_MODE0_PRIMARY_READY_SECOND);
    CHECK(object_bytes[0x17] == 0x80 &&
          r.primary_or_writes == 4 && r.primary_and_writes == 1 &&
          r.auxiliary_index == 2 && r.data_offset == 1);
    return 0;
}

static int test_partial_effect_stops(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_report prefix;
    pr_r464_mode0_primary_report r;

    setup(object_bytes, sizeof object_bytes, 0, source,
          sizeof source, lookup);
    object_bytes[0x17] = 0x40;
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, &prefix) == PR_R464_MODE0_READY_PRIMARY);
    CHECK(pr_r464_mode0_first_primary_byte(
              &prefix, &object, NULL, lookup, 6, &r) ==
          PR_R464_MODE0_PRIMARY_TABLE);
    CHECK(object_bytes[0x17] == 0x43 &&
          r.stopped_at == PR_R464_MODE0_PRIMARY_STOP_TABLE &&
          r.primary_index == 2 && r.helper_calls == 3 &&
          r.table_reads == 2 && r.primary_or_writes == 2 &&
          r.data_offset == 0 && r.next_offset == 0);

    setup(object_bytes, sizeof object_bytes, 0, source,
          sizeof source, lookup);
    object.capacity = 0x17;
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, &prefix) == PR_R464_MODE0_READY_PRIMARY);
    CHECK(pr_r464_mode0_first_primary_byte(
              &prefix, &object, NULL, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_PRIMARY_BOUNDS);
    CHECK(r.stopped_at == PR_R464_MODE0_PRIMARY_STOP_OBJECT_OR &&
          r.helper_calls == 1 && r.table_reads == 1 &&
          r.primary_or_writes == 0);

    object.capacity = sizeof object_bytes;
    setup(object_bytes, sizeof object_bytes, 1, source,
          sizeof source, lookup);
    object_bytes[0x17] = 0x40;
    /* Selector zero yields no aux OR sites, so prefix can complete
     * without a mapped auxiliary. The next slice reaches its read. */
    CHECK(pr_r464_mode0_first_pixel_prefix(
              source, sizeof source, 0x11, 0, 0, &object, NULL,
              NULL, 0, &prefix) == PR_R464_MODE0_READY_PRIMARY);
    CHECK(pr_r464_mode0_first_primary_byte(
              &prefix, &object, NULL, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_PRIMARY_AUX_MAPPING);
    CHECK(object_bytes[0x17] == 0x4F &&
          r.stopped_at == PR_R464_MODE0_PRIMARY_STOP_AUX_READ &&
          r.primary_or_writes == 4 && r.primary_and_writes == 0 &&
          r.data_offset == 0 && r.next_offset == 0);
    return 0;
}

int main(void) {
    if (test_primary_or_without_aux()) return 1;
    if (test_primary_and_after_aux_or()) return 1;
    if (test_partial_effect_stops()) return 1;
    return 0;
}
