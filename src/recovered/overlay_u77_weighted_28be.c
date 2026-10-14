/* GAME.OVR unit 0x77:+28BE..+2938; +2939..+299B is string data. */
#include "poolrad/recovered/overlay_u77_weighted_28be.h"

#include <limits.h>

pr_u77_28be_status pr_u77_weighted_28be(
    const uint8_t *record, size_t record_size, const uint32_t weights[5],
    uint32_t *result)
{
    uint32_t sum = 0;
    int64_t quotient;
    if (record == NULL || weights == NULL || result == NULL ||
        record_size < 0x92u)
        return PR_U77_28BE_ARGUMENT;
    if (weights[3] == 0u)
        return PR_U77_28BE_DIVIDE_ERROR;
    for (unsigned i = 0; i < 5u; ++i) {
        size_t offset = 0x88u + 2u * i;
        uint32_t word = (uint32_t)record[offset] |
                        ((uint32_t)record[offset + 1u] << 8);
        sum += word * weights[i];
    }
    sum += 100u;
    if (sum == (uint32_t)INT32_MIN && weights[3] == UINT32_MAX)
        return PR_U77_28BE_DIVIDE_ERROR;
    /* The runtime helpers operate on signed Turbo Pascal LongInt values. */
    quotient = (int64_t)(int32_t)sum / (int64_t)(int32_t)weights[3];
    if (quotient > INT32_MAX || quotient < INT32_MIN)
        return PR_U77_28BE_DIVIDE_ERROR;
    *result = (uint32_t)(int32_t)quotient;
    return PR_U77_28BE_OK;
}
