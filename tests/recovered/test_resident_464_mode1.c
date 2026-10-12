#include "poolrad/recovered/resident_464_mode1.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(uint8_t *bytes, size_t size, uint16_t width,
                  uint16_t height, uint8_t count, int auxiliary) {
    memset(bytes, 0, size);
    bytes[0] = (uint8_t)width; bytes[1] = (uint8_t)(width >> 8);
    bytes[2] = (uint8_t)height; bytes[3] = (uint8_t)(height >> 8);
    bytes[8] = count;
    if (auxiliary) bytes[0x13] = 1; /* synthetic nonzero far-pointer token */
}

static int test_zero_count_and_table_or(void) {
    uint8_t primary[64], block[0x19] = {0};
    const uint8_t table[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {primary, sizeof primary};
    pr_r464_mode1_report r;

    setup(primary, sizeof primary, 2, 1, 0, 0);
    object.capacity = 9;
    CHECK(pr_r464_postprocess_mode1(NULL, 0, 0x11, 0, 1, &object,
                                    NULL, NULL, 0, 0, &r) == PR_R464_MODE1_OK);
    CHECK(r.move_calls == 0 && r.pixels_completed == 0 &&
          r.source_offset == 0x11 && r.data_offset == 0);

    object.capacity = sizeof primary;
    setup(primary, sizeof primary, 1, 1, 1, 0);
    block[0x11] = 0x12; block[0x12] = 0x34;
    block[0x13] = 0x56; block[0x14] = 0x78;
    CHECK(pr_r464_postprocess_mode1(block, sizeof block, 0x11, 0xA5, 1,
                                    &object, NULL, table, sizeof table, 1,
                                    &r) == PR_R464_MODE1_OK);
    CHECK(r.move_calls == 1 && r.pixels_completed == 1 &&
          r.aux_writes == 0 && r.primary_or_writes == 13 &&
          r.primary_and_writes == 0 && r.source_offset == 0x15 &&
          r.data_offset == 4);
    CHECK(primary[0x17] == 0xAA && primary[0x18] == 0x66 &&
          primary[0x19] == 0x1E && primary[0x1A] == 0x01);
    return 0;
}

static int test_auxiliary_and_nested_loops(void) {
    uint8_t primary[80], aux_bytes[3] = {0}, block[0x19] = {0};
    const uint8_t table[4] = {1, 2, 4, 8};
    pr_r464_block_object object = {primary, sizeof primary};
    pr_r464_block_object auxiliary = {aux_bytes, sizeof aux_bytes};
    pr_r464_mode1_report r;

    setup(primary, sizeof primary, 2, 1, 1, 1);
    block[0x11] = 0x12; block[0x12] = 0x34;
    block[0x13] = 0x56; block[0x14] = 0x78;
    block[0x15] = 0x11; block[0x16] = 0x11;
    block[0x17] = 0x11; block[0x18] = 0x11;
    CHECK(pr_r464_postprocess_mode1(block, sizeof block, 0x11, 0xFF, 1,
                                    &object, &auxiliary, table, sizeof table,
                                    2, &r) == PR_R464_MODE1_OK);
    CHECK(r.move_calls == 2 && r.pixels_completed == 2 &&
          r.aux_writes == 2 && r.auxiliary_index == 2 &&
          r.primary_or_writes == 21 && r.primary_and_writes == 8 &&
          r.data_offset == 8 && r.source_offset == 0x19 &&
          r.layer == 1 && r.x == 1 && r.y == 0);
    CHECK(aux_bytes[0] == 0x80 && aux_bytes[1] == 0xFF);
    CHECK(primary[0x17] == 0x2A && primary[0x18] == 0x66 &&
          primary[0x19] == 0x1E && primary[0x1A] == 0x01);
    CHECK(primary[0x1B] == 0 && primary[0x1C] == 0 &&
          primary[0x1D] == 0 && primary[0x1E] == 0);
    return 0;
}

static int test_reached_host_stops(void) {
    uint8_t primary[64], block[0x19] = {0};
    const uint8_t table[4] = {1, 2, 4, 8};
    const uint8_t short_table[1] = {1};
    pr_r464_block_object object = {primary, sizeof primary};
    pr_r464_mode1_report r;

    setup(primary, sizeof primary, 1, 1, 1, 1);
    block[0x11] = 0x12; block[0x12] = 0x34;
    block[0x13] = 0x56; block[0x14] = 0x78;
    CHECK(pr_r464_postprocess_mode1(block, sizeof block, 0x11, 0, 1,
                                    &object, NULL, table, sizeof table, 1,
                                    &r) == PR_R464_MODE1_AUX_MAPPING);
    CHECK(r.stopped_at == PR_R464_MODE1_STOP_AUX_WRITE &&
          r.move_calls == 1 && r.pixels_completed == 0 &&
          r.source_offset == 0x11 && primary[0x17] == 0);

    setup(primary, sizeof primary, 1, 1, 1, 0);
    CHECK(pr_r464_postprocess_mode1(block, sizeof block, 0x11, 0, 1,
                                    &object, NULL, short_table, 1, 1,
                                    &r) == PR_R464_MODE1_TABLE);
    CHECK(r.stopped_at == PR_R464_MODE1_STOP_TABLE &&
          r.move_calls == 1 && r.primary_or_writes == 1 &&
          r.source_offset == 0x11 && r.data_offset == 0 &&
          primary[0x17] == 0x80);

    setup(primary, sizeof primary, 2, 1, 1, 0);
    CHECK(pr_r464_postprocess_mode1(block, 0x15, 0x11, 0, 1,
                                    &object, NULL, table, sizeof table, 2,
                                    &r) == PR_R464_MODE1_BOUNDS);
    CHECK(r.stopped_at == PR_R464_MODE1_STOP_MOVE && r.x == 1 &&
          r.move_calls == 1 && r.pixels_completed == 1 &&
          r.source_offset == 0x15 && r.data_offset == 4 &&
          primary[0x17] == 0xAA);

    setup(primary, sizeof primary, 0, 1, 1, 0);
    CHECK(pr_r464_postprocess_mode1(block, sizeof block, 0x11, 0, 1,
                                    &object, NULL, table, sizeof table, 1,
                                    &r) == PR_R464_MODE1_LIMIT);
    CHECK(r.stopped_at == PR_R464_MODE1_STOP_LIMIT &&
          r.x == 1 && r.y == 0 && r.pixels_completed == 1 &&
          r.source_offset == 0x15 && r.data_offset == 4);
    return 0;
}

static int test_y_and_layer_rollover(void) {
    uint8_t primary[64], block[0x21] = {0};
    const uint8_t table[4] = {0};
    pr_r464_block_object object = {primary, sizeof primary};
    pr_r464_mode1_report r;

    setup(primary, sizeof primary, 1, 2, 2, 0);
    CHECK(pr_r464_postprocess_mode1(block, sizeof block, 0x11, 0, 0,
                                    &object, NULL, table, sizeof table, 4,
                                    &r) == PR_R464_MODE1_OK);
    CHECK(r.move_calls == 4 && r.pixels_completed == 4 &&
          r.layer == 2 && r.x == 0 && r.y == 1 &&
          r.source_offset == 0x21 && r.data_offset == 16 &&
          r.primary_or_writes == 0);
    return 0;
}

int main(void) {
    if (test_zero_count_and_table_or()) return 1;
    if (test_auxiliary_and_nested_loops()) return 1;
    if (test_reached_host_stops()) return 1;
    if (test_y_and_layer_rollover()) return 1;
    return 0;
}
