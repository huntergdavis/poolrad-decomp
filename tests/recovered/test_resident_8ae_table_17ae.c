#include "poolrad/recovered/resident_8ae_table_17ae.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static uint8_t segment[PR_REC_8AE_SEGMENT_BYTES];

static void source_word(uint16_t offset, uint16_t value)
{
    segment[offset] = (uint8_t)value;
    segment[(uint16_t)(offset + 1u)] = (uint8_t)(value >> 8);
}

static uint16_t observed_word(uint16_t offset)
{
    return (uint16_t)(segment[offset] |
                      ((uint16_t)segment[(uint16_t)(offset + 1u)] << 8));
}

static void expect_record(uint8_t row, uint16_t word)
{
    uint16_t base = (uint16_t)(0x03c7u + (uint16_t)row * 0x30u);
    uint16_t j;
    assert(observed_word(base) == 1u);
    assert(observed_word((uint16_t)(base + 2u)) == word);
    for (j = 4u; j < 0x30u; ++j)
        assert(segment[(uint16_t)(base + j)] == 0u);
}

static void test_zero_rows_and_arguments(void)
{
    uint8_t before[0x30];
    memset(segment, 0, sizeof segment);
    memset(segment + 0x03c7u, 0xa5, 0x30u);
    memcpy(before, segment + 0x03c7u, sizeof before);
    segment[0x0209u] = 0x77u;
    segment[0x01d8u] = 0x80u;
    segment[0x01d9u] = 0x80u;
    assert(pr_rec_res_08ae_17ae(segment, sizeof segment, 3u) ==
           PR_REC_8AE_TABLE_OK);
    assert(observed_word(0x01ffu) == 3u);
    assert(segment[0x01d8u] == 0u && segment[0x01d9u] == 0u);
    assert(segment[0x0209u] == 0x77u);
    assert(memcmp(before, segment + 0x03c7u, sizeof before) == 0);

    segment[0x01ffu] = 0x5au;
    assert(pr_rec_res_08ae_17ae(NULL, sizeof segment, 1u) ==
           PR_REC_8AE_TABLE_ARGUMENT);
    assert(pr_rec_res_08ae_17ae(segment, sizeof segment - 1u, 1u) ==
           PR_REC_8AE_TABLE_ARGUMENT);
    assert(segment[0x01ffu] == 0x5au);
}

static void test_mixed_rows(void)
{
    uint16_t j;
    memset(segment, 0, sizeof segment);
    for (j = 0; j < 0x30u; ++j) {
        segment[0x03c7u + j] = 0xa5u;
        segment[0x03f7u + j] = 0xa5u;
        segment[0x0427u + j] = 0xa5u;
        segment[0x0457u + j] = 0xa5u;
    }
    source_word(0x061fu, 0x1234u); /* argument 3, row 0 */
    source_word(0x0625u, 0x5678u); /* argument 3, row 3 */
    memset(segment + 0x0209u, 0x77, 4u);
    assert(pr_rec_res_08ae_17ae(segment, sizeof segment, 3u) ==
           PR_REC_8AE_TABLE_OK);
    expect_record(0u, 0x1234u);
    expect_record(3u, 0x5678u);
    for (j = 0; j < 0x30u; ++j) {
        assert(segment[0x03f7u + j] == 0xa5u);
        assert(segment[0x0427u + j] == 0xa5u);
    }
    assert(segment[0x0209u] == 3u && segment[0x020au] == 0x77u);
    assert(segment[0x020bu] == 0x77u && segment[0x020cu] == 3u);
    assert(observed_word(0x01ffu) == 3u);
    assert(observed_word(0x061fu) == 0x1234u);
    assert(observed_word(0x0625u) == 0x5678u);
}

static void test_effective_offset_wrap(void)
{
    /* 0x0607 + 8 * 0x1F3F wraps to 0xFFFF. Its WORD crosses the
     * 64 KiB boundary. */
    memset(segment, 0, sizeof segment);
    segment[0xffffu] = 0x34u;
    segment[0x0000u] = 0x12u;
    assert(pr_rec_res_08ae_17ae(segment, sizeof segment, 0x1f3fu) ==
           PR_REC_8AE_TABLE_OK);
    expect_record(0u, 0x1234u);
    assert(observed_word(0x01ffu) == 0x1f3fu);
    assert(segment[0x0209u] == 0x3fu);
    assert(observed_word(0x03f7u) == 0u);
}

static void test_in_place_source_alias(void)
{
    /* 0x0607 + 8 * 0x1FB8 wraps to record 0's base, 0x03C7.
     * Row 0 writes its value at +2, becoming row 1's source WORD. */
    memset(segment, 0, sizeof segment);
    source_word(0x03c7u, 0x4444u);
    assert(pr_rec_res_08ae_17ae(segment, sizeof segment, 0x1fb8u) ==
           PR_REC_8AE_TABLE_OK);
    expect_record(0u, 0x4444u);
    expect_record(1u, 0x4444u);
    assert(observed_word(0x0427u) == 0u);
    assert(segment[0x0209u] == 0xb8u && segment[0x020au] == 0xb8u);
}

int main(void)
{
    test_zero_rows_and_arguments();
    test_mixed_rows();
    test_effective_offset_wrap();
    test_in_place_source_alias();
    return 0;
}
