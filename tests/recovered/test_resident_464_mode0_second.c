#include "poolrad/recovered/resident_464_mode0_second.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(uint8_t *object_bytes, size_t size, int has_aux,
                  uint8_t source[0x15], uint8_t lookup[64]) {
    memset(object_bytes, 0, size);
    memset(source, 0, 0x15);
    memset(lookup, 0, 64);
    object_bytes[8] = 1;
    object_bytes[9] = 0x11; /* Selector nibble 1 returns AL=1. */
    if (has_aux) object_bytes[0x13] = 1;
    memset(source + 0x11, 0x11, 4);
    lookup[4] = 1; lookup[5] = 2;
    lookup[6] = 4; lookup[7] = 8;
}

static int first_byte(
    uint8_t source[0x15], pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, uint8_t selector,
    const uint8_t *pattern, size_t pattern_size,
    const uint8_t lookup[64], pr_r464_mode0_report *prefix,
    pr_r464_mode0_primary_report *first) {
    if (pr_r464_mode0_first_pixel_prefix(
            source, 0x15, 0x11, 0, selector, object, auxiliary,
            pattern, pattern_size, prefix) != PR_R464_MODE0_READY_PRIMARY)
        return 1;
    return pr_r464_mode0_first_primary_byte(
        prefix, object, auxiliary, lookup, 64, first) !=
        PR_R464_MODE0_PRIMARY_READY_SECOND;
}

static int test_second_byte_without_aux(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_mode0_report prefix;
    pr_r464_mode0_primary_report first;
    pr_r464_mode0_second_report r;

    setup(object_bytes, sizeof object_bytes, 0, source, lookup);
    object_bytes[0x18] = 0x40;
    CHECK(!first_byte(source, &object, NULL, 0, NULL, 0,
                      lookup, &prefix, &first));
    CHECK(pr_r464_mode0_second_primary_byte(
              &prefix, &first, &object, NULL, lookup, sizeof lookup, &r) ==
          PR_R464_MODE0_SECOND_READY_LOOPS);
    CHECK(object_bytes[0x18] == 0x4F &&
          r.helper_calls == 4 && r.table_reads == 4 &&
          r.primary_or_writes == 4 && r.primary_and_writes == 0 &&
          r.data_offset == 2 && r.source_offset == 0x15 &&
          r.primary_index == 7 && r.next_offset == 0x05CB &&
          r.stopped_at == PR_R464_MODE0_SECOND_STOP_LOOP_CONTINUATION);
    return 0;
}

static int test_second_byte_and_after_aux(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    uint8_t auxiliary_bytes[2] = {0, 0};
    const uint8_t pattern[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {
        auxiliary_bytes, sizeof auxiliary_bytes
    };
    pr_r464_mode0_report prefix;
    pr_r464_mode0_primary_report first;
    pr_r464_mode0_second_report r;

    setup(object_bytes, sizeof object_bytes, 1, source, lookup);
    object_bytes[0x18] = 0x80;
    CHECK(!first_byte(source, &object, &auxiliary, 1, pattern,
                      sizeof pattern, lookup, &prefix, &first));
    CHECK(auxiliary_bytes[0] == 0x0F &&
          auxiliary_bytes[1] == 0x0F && first.data_offset == 1);
    CHECK(pr_r464_mode0_second_primary_byte(
              &prefix, &first, &object, &auxiliary, lookup,
              sizeof lookup, &r) == PR_R464_MODE0_SECOND_READY_LOOPS);
    CHECK(object_bytes[0x18] == 0x80 &&
          r.primary_or_writes == 4 && r.primary_and_writes == 1 &&
          r.data_offset == 2 && r.source_offset == 0x15 &&
          r.auxiliary_index == 2);
    return 0;
}

static int test_late_host_stops(void) {
    uint8_t object_bytes[32], source[0x15], lookup[64];
    uint8_t auxiliary_byte[1] = {1};
    pr_r464_block_object object = {object_bytes, sizeof object_bytes};
    pr_r464_block_object auxiliary = {
        auxiliary_byte, sizeof auxiliary_byte
    };
    pr_r464_mode0_report prefix;
    pr_r464_mode0_primary_report first;
    pr_r464_mode0_second_report r;

    setup(object_bytes, sizeof object_bytes, 0, source, lookup);
    object_bytes[0x18] = 0x40;
    CHECK(!first_byte(source, &object, NULL, 0, NULL, 0,
                      lookup, &prefix, &first));
    CHECK(pr_r464_mode0_second_primary_byte(
              &prefix, &first, &object, NULL, lookup, 6, &r) ==
          PR_R464_MODE0_SECOND_TABLE);
    CHECK(object_bytes[0x18] == 0x43 &&
          r.stopped_at == PR_R464_MODE0_SECOND_STOP_TABLE &&
          r.primary_index == 6 && r.helper_calls == 3 &&
          r.table_reads == 2 && r.primary_or_writes == 2 &&
          r.data_offset == 1 && r.source_offset == 0x11);

    setup(object_bytes, sizeof object_bytes, 0, source, lookup);
    object.capacity = 0x18;
    CHECK(!first_byte(source, &object, NULL, 0, NULL, 0,
                      lookup, &prefix, &first));
    CHECK(pr_r464_mode0_second_primary_byte(
              &prefix, &first, &object, NULL, lookup,
              sizeof lookup, &r) == PR_R464_MODE0_SECOND_BOUNDS);
    CHECK(r.stopped_at == PR_R464_MODE0_SECOND_STOP_OBJECT_OR &&
          r.helper_calls == 1 && r.table_reads == 1 &&
          r.primary_or_writes == 0);
    object.capacity = sizeof object_bytes;

    setup(object_bytes, sizeof object_bytes, 1, source, lookup);
    object_bytes[0x18] = 0x40;
    /* Bitmask zero skips prefix auxiliary ORs; byte 0 is sufficient
     * for the first primary group, but the second needs byte 1. */
    CHECK(!first_byte(source, &object, &auxiliary, 0, NULL, 0,
                      lookup, &prefix, &first));
    CHECK(pr_r464_mode0_second_primary_byte(
              &prefix, &first, &object, &auxiliary, lookup,
              sizeof lookup, &r) == PR_R464_MODE0_SECOND_BOUNDS);
    CHECK(object_bytes[0x18] == 0x4F &&
          r.stopped_at == PR_R464_MODE0_SECOND_STOP_AUX_READ &&
          r.primary_or_writes == 4 && r.primary_and_writes == 0 &&
          r.data_offset == 1 && r.source_offset == 0x11);
    return 0;
}

int main(void) {
    if (test_second_byte_without_aux()) return 1;
    if (test_second_byte_and_after_aux()) return 1;
    if (test_late_host_stops()) return 1;
    return 0;
}
