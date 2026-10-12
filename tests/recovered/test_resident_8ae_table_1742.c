#include "poolrad/recovered/resident_8ae_table_1742.h"

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

static void expect_record(uint8_t row, uint16_t value)
{
    uint16_t base = (uint16_t)(0x022du + (uint16_t)row * 0x30u);
    uint16_t i;
    assert(observed_word(base) == 1u);
    assert(observed_word((uint16_t)(base + 2u)) == value);
    for (i = 4u; i < 0x30u; ++i)
        assert(segment[(uint16_t)(base + i)] == 0u);
}

static void test_zero_rows_and_short_view(void)
{
    uint8_t before[0x30];
    memset(segment, 0, sizeof segment);
    memset(segment + 0x022du, 0xa5, sizeof before);
    memcpy(before, segment + 0x022du, sizeof before);
    segment[0x0209u] = 0x77u;
    segment[0x01d8u] = 0x80u;
    segment[0x01d9u] = 0x80u;
    assert(pr_rec_res_08ae_1742(segment, sizeof segment, 5u) ==
           PR_REC_8AE_TABLE_OK);
    assert(observed_word(0x01ffu) == 5u);
    assert(segment[0x01d8u] == 0u && segment[0x01d9u] == 0u);
    assert(segment[0x0209u] == 0x77u);
    assert(memcmp(before, segment + 0x022du, sizeof before) == 0);

    segment[0x01ffu] = 0x5au;
    assert(pr_rec_res_08ae_1742(NULL, sizeof segment, 1u) ==
           PR_REC_8AE_TABLE_ARGUMENT);
    assert(pr_rec_res_08ae_1742(segment, sizeof segment - 1u, 1u) ==
           PR_REC_8AE_TABLE_ARGUMENT);
    assert(segment[0x01ffu] == 0x5au);
}

static void test_separated_hits_and_misses(void)
{
    uint16_t i;
    memset(segment, 0, sizeof segment);
    for (i = 0; i < 4u * 0x30u; ++i)
        segment[0x022du + i] = 0xa5u;
    /* For argument 5, SI=40 and source row 0 starts at 0x0587. */
    source_word(0x0587u, 0x1234u);
    source_word(0x058bu, 0x5678u);
    memset(segment + 0x0209u, 0x77, 4u);
    assert(pr_rec_res_08ae_1742(segment, sizeof segment, 5u) ==
           PR_REC_8AE_TABLE_OK);
    expect_record(0u, 0x1234u);
    expect_record(2u, 0x5678u);
    for (i = 0; i < 0x30u; ++i) {
        assert(segment[0x025du + i] == 0xa5u);
        assert(segment[0x02bdu + i] == 0xa5u);
    }
    assert(segment[0x0209u] == 5u && segment[0x020au] == 0x77u);
    assert(segment[0x020bu] == 5u && segment[0x020cu] == 0x77u);
    assert(observed_word(0x01ffu) == 5u);
    assert(observed_word(0x0587u) == 0x1234u);
    assert(observed_word(0x058bu) == 0x5678u);
}

static void test_wrapped_source_word(void)
{
    /* 0x055F + 8 * 0x1F54 wraps to 0xFFFF, so row 0 spans
     * the last and first bytes of the segment. */
    memset(segment, 0, sizeof segment);
    segment[0xffffu] = 0x34u;
    segment[0x0000u] = 0x12u;
    assert(pr_rec_res_08ae_1742(segment, sizeof segment, 0x1f54u) ==
           PR_REC_8AE_TABLE_OK);
    expect_record(0u, 0x1234u);
    assert(observed_word(0x01ffu) == 0x1f54u);
    assert(segment[0x0209u] == 0x54u);
    assert(observed_word(0x025du) == 0u);
}

static void test_later_source_aliases_prior_record(void)
{
    /* For argument 0x1F99, source row 0 is at 0x0227 and row 3
     * is at record 0's base, 0x022D. Row 0 changes that later source
     * to WORD 1 before row 3 reads it. */
    memset(segment, 0, sizeof segment);
    source_word(0x0227u, 0x3333u);
    assert(pr_rec_res_08ae_1742(segment, sizeof segment, 0x1f99u) ==
           PR_REC_8AE_TABLE_OK);
    expect_record(0u, 0x3333u);
    expect_record(3u, 1u);
    assert(observed_word(0x025du) == 0u);
    assert(observed_word(0x028du) == 0u);
    assert(segment[0x0209u] == 0x99u && segment[0x020cu] == 0x99u);
}

int main(void)
{
    test_zero_rows_and_short_view();
    test_separated_hits_and_misses();
    test_wrapped_source_word();
    test_later_source_aliases_prior_record();
    return 0;
}
