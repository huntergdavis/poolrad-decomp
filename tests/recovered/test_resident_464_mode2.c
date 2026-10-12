#include "poolrad/recovered/resident_464_mode2.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(uint8_t *bytes, size_t size, uint8_t count,
                  uint16_t stride, int auxiliary) {
    memset(bytes, 0, size);
    bytes[8] = count;
    bytes[0x11] = (uint8_t)stride;
    bytes[0x12] = (uint8_t)(stride >> 8);
    if (auxiliary) bytes[0x13] = 1; /* synthetic nonzero far-pointer token */
}

static int test_zero_and_copy_only(void) {
    uint8_t primary[64], block[0x15] = {0};
    pr_r464_block_object object = {primary, sizeof primary};
    pr_r464_mode2_report r;

    setup(primary, sizeof primary, 0, 2, 0);
    CHECK(pr_r464_postprocess_mode2(NULL, 0, 0x11, 0xA5, 0x0A,
                                    &object, NULL, &r) == PR_R464_MODE2_OK);
    CHECK(r.move_calls == 0 && r.layer == 0 &&
          r.source_offset == 0x11 && r.data_offset == 0);

    object.capacity = 9;
    CHECK(pr_r464_postprocess_mode2(NULL, 0, 0x11, 0, 0,
                                    &object, NULL, &r) == PR_R464_MODE2_OK);
    CHECK(r.move_calls == 0);
    object.capacity = sizeof primary;

    setup(primary, sizeof primary, 2, 2, 0);
    block[0x11] = 0x12; block[0x12] = 0x34;
    block[0x13] = 0x56; block[0x14] = 0x78;
    CHECK(pr_r464_postprocess_mode2(block, sizeof block, 0x11, 0, 0x0A,
                                    &object, NULL, &r) == PR_R464_MODE2_OK);
    CHECK(r.move_calls == 2 && r.layer == 2 && r.mask_steps == 0 &&
          r.source_offset == 0x15 && r.data_offset == 4 &&
          r.stopped_at == PR_R464_MODE2_STOP_NONE);
    CHECK(primary[0x17] == 0x12 && primary[0x18] == 0x34 &&
          primary[0x19] == 0x56 && primary[0x1A] == 0x78);

    setup(primary, sizeof primary, 1, 0, 0);
    CHECK(pr_r464_postprocess_mode2(NULL, 0, 0x11, 0, 0,
                                    &object, NULL, &r) == PR_R464_MODE2_OK);
    CHECK(r.move_calls == 1 && r.source_offset == 0x11 &&
          r.data_offset == 0);
    return 0;
}

static int test_auxiliary_mask_paths(void) {
    uint8_t primary[64], aux_bytes[4] = {0x12, 0x01, 0x80, 0xF0};
    uint8_t block[0x15] = {0};
    pr_r464_block_object object = {primary, sizeof primary};
    pr_r464_block_object auxiliary = {aux_bytes, sizeof aux_bytes};
    pr_r464_mode2_report r;

    setup(primary, sizeof primary, 1, 4, 1);
    block[0x11] = 0xA1; block[0x12] = 0x2A;
    block[0x13] = 0xAA; block[0x14] = 0x33;
    CHECK(pr_r464_postprocess_mode2(block, sizeof block, 0x11, 0xFF, 0x0A,
                                    &object, &auxiliary, &r) ==
          PR_R464_MODE2_OK);
    CHECK(r.move_calls == 1 && r.mask_steps == 4 &&
          r.aux_writes == 4 && r.primary_writes == 4 &&
          r.source_offset == 0x15 && r.data_offset == 4);
    CHECK(aux_bytes[0] == 0xF0 && aux_bytes[1] == 0x10 &&
          aux_bytes[2] == 0xFF && aux_bytes[3] == 0xF0);
    CHECK(primary[0x17] == 0x01 && primary[0x18] == 0x2A &&
          primary[0x19] == 0 && primary[0x1A] == 0x03);

    setup(primary, sizeof primary, 1, 1, 1);
    memset(aux_bytes, 0, sizeof aux_bytes);
    aux_bytes[0] = 0xF0;
    CHECK(pr_r464_postprocess_mode2(block, sizeof block, 0x11, 0,
                                    0x1A, &object, &auxiliary, &r) ==
          PR_R464_MODE2_OK);
    CHECK(r.aux_writes == 0 && r.primary_writes == 1 &&
          aux_bytes[0] == 0xF0 && primary[0x17] == 0x01);
    return 0;
}

static int test_source_offset_wrap_between_layers(void) {
    static uint8_t block[0x10000];
    uint8_t primary[64];
    pr_r464_block_object object = {primary, sizeof primary};
    pr_r464_mode2_report r;

    memset(block, 0, sizeof block);
    block[0xFFFF] = 0xC1;
    block[0] = 0xD2;
    setup(primary, sizeof primary, 2, 1, 0);
    CHECK(pr_r464_postprocess_mode2(block, sizeof block, 0xFFFF, 0, 0,
                                    &object, NULL, &r) == PR_R464_MODE2_OK);
    CHECK(r.move_calls == 2 && r.source_offset == 1 &&
          r.data_offset == 2 && primary[0x17] == 0xC1 &&
          primary[0x18] == 0xD2);
    return 0;
}

static int test_reached_stops(void) {
    uint8_t primary[64], aux_bytes[2] = {0};
    uint8_t block[0x14] = {0};
    pr_r464_block_object object = {primary, sizeof primary};
    pr_r464_block_object auxiliary = {aux_bytes, sizeof aux_bytes};
    pr_r464_mode2_report r;

    setup(primary, sizeof primary, 1, 1, 1);
    block[0x11] = 0xA1;
    CHECK(pr_r464_postprocess_mode2(block, sizeof block, 0x11, 0, 0x0A,
                                    &object, NULL, &r) ==
          PR_R464_MODE2_AUX_MAPPING);
    CHECK(r.stopped_at == PR_R464_MODE2_STOP_AUX_WRITE &&
          r.move_calls == 1 && r.source_offset == 0x11 &&
          r.data_offset == 0 && primary[0x17] == 0xA1);

    setup(primary, sizeof primary, 2, 2, 0);
    block[0x11] = 0x12; block[0x12] = 0x34;
    CHECK(pr_r464_postprocess_mode2(block, 0x13, 0x11, 0, 0,
                                    &object, NULL, &r) ==
          PR_R464_MODE2_BOUNDS);
    CHECK(r.stopped_at == PR_R464_MODE2_STOP_MOVE && r.layer == 2 &&
          r.move_calls == 1 && r.source_offset == 0x13 &&
          r.data_offset == 2 && primary[0x17] == 0x12 &&
          primary[0x18] == 0x34 && primary[0x19] == 0);

    setup(primary, sizeof primary, 1, 3, 1);
    block[0x11] = 0xA1; block[0x12] = 0x2A; block[0x13] = 0xAA;
    CHECK(pr_r464_postprocess_mode2(block, sizeof block, 0x11, 0, 0x0A,
                                    &object, &auxiliary, &r) ==
          PR_R464_MODE2_BOUNDS);
    CHECK(r.stopped_at == PR_R464_MODE2_STOP_AUX_WRITE &&
          r.mask_index == 2 && r.move_calls == 1 &&
          r.mask_steps == 2 && r.aux_writes == 2 &&
          r.source_offset == 0x11 && r.data_offset == 0);
    CHECK(primary[0x17] == 0x01 && primary[0x18] == 0x20 &&
          primary[0x19] == 0xAA);

    object.capacity = 24;
    setup(primary, sizeof primary, 1, 2, 0);
    CHECK(pr_r464_postprocess_mode2(block, sizeof block, 0x11, 0, 0,
                                    &object, NULL, &r) ==
          PR_R464_MODE2_BOUNDS);
    CHECK(r.stopped_at == PR_R464_MODE2_STOP_MOVE &&
          r.move_calls == 0 && primary[0x17] == 0);
    return 0;
}

int main(void) {
    if (test_zero_and_copy_only()) return 1;
    if (test_auxiliary_mask_paths()) return 1;
    if (test_source_offset_wrap_between_layers()) return 1;
    if (test_reached_stops()) return 1;
    return 0;
}
