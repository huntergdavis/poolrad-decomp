#include "poolrad/recovered/overlay_u77_weighted_28be.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put_word(uint8_t *record, unsigned index, uint16_t value)
{
    size_t at = 0x88u + 2u * index;
    record[at] = (uint8_t)value;
    record[at + 1u] = (uint8_t)(value >> 8);
}

static void case_value(const uint16_t values[5], uint32_t expected)
{
    uint8_t record[0x120];
    const uint32_t weights[5] = {1u, 10u, 100u, 200u, 1000u};
    uint32_t result = 0xDEADBEEFu;
    memset(record, 0xA5, sizeof record);
    for (unsigned i = 0; i < 5u; ++i) put_word(record, i, values[i]);
    assert(pr_u77_weighted_28be(record, sizeof record, weights, &result) ==
           PR_U77_28BE_OK);
    assert(result == expected);
    for (unsigned i = 0; i < 5u; ++i)
        assert(record[0x88u + 2u * i] == (uint8_t)values[i]);
}

int main(void)
{
    const uint16_t zero[5] = {0, 0, 0, 0, 0};
    const uint16_t one[5] = {1, 1, 1, 1, 1};
    const uint16_t max[5] = {65535, 65535, 65535, 65535, 65535};
    const uint16_t fifth[5] = {0, 0, 0, 0, 1};
    uint8_t record[0x92] = {0};
    uint32_t weights[5] = {1, 10, 100, 200, 1000};
    uint32_t result = 0xDEADBEEFu;
    case_value(zero, 0);
    case_value(one, 7);
    case_value(max, 429582);
    case_value(fifth, 5);
    assert(pr_u77_weighted_28be(record, sizeof record, weights, &result) ==
           PR_U77_28BE_OK);
    assert(result == 0);
    result = 0xDEADBEEFu;
    assert(pr_u77_weighted_28be(record, sizeof record - 1u, weights, &result) ==
           PR_U77_28BE_ARGUMENT);
    assert(result == 0xDEADBEEFu);
    assert(pr_u77_weighted_28be(NULL, sizeof record, weights, &result) ==
           PR_U77_28BE_ARGUMENT);
    assert(pr_u77_weighted_28be(record, sizeof record, NULL, &result) ==
           PR_U77_28BE_ARGUMENT);
    assert(pr_u77_weighted_28be(record, sizeof record, weights, NULL) ==
           PR_U77_28BE_ARGUMENT);
    weights[3] = 0;
    assert(pr_u77_weighted_28be(record, sizeof record, weights, &result) ==
           PR_U77_28BE_DIVIDE_ERROR);
    assert(result == 0xDEADBEEFu);
    puts("unit 0x77 +28BE value tests passed");
    return 0;
}
