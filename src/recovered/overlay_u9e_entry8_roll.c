#include "poolrad/recovered/overlay_u9e_entry8_roll.h"

#include <string.h>

pr_u9e_entry8_status pr_u9e_entry8_roll(uint16_t count_word,
                                        uint16_t range_word,
                                        uint32_t *rand_seed,
                                        uint8_t *al_out,
                                        pr_u9e_entry8_report *report)
{
    pr_u9e_entry8_report local;
    pr_u9e_entry8_report *r = report ? report : &local;
    uint8_t count = (uint8_t)count_word;
    uint8_t range = (uint8_t)range_word;
    uint8_t sum = 0;
    unsigned i;

    memset(r, 0, sizeof *r);
    r->count = count;
    r->range = range;
    if (!al_out)
        return PR_U9E_ENTRY8_ARGUMENT;
    if (count != 0 && !rand_seed)
        return PR_U9E_ENTRY8_SEED_MISSING;

    for (i = 0; i < count; ++i) {
        uint16_t random_value;
        /* SYSTEM +0x120F: 32-bit unsigned wrap, then +0x11C0 returns
         * high16(seed) mod range. The range-zero branch still advances seed. */
        *rand_seed = *rand_seed * UINT32_C(0x08088405) + UINT32_C(1);
        random_value = range ? (uint16_t)((*rand_seed >> 16) % range) : 0;
        sum = (uint8_t)(sum + (uint8_t)(random_value + 1u));
        r->random_calls++;
    }
    if (rand_seed)
        r->final_seed = *rand_seed;
    *al_out = sum;
    return PR_U9E_ENTRY8_OK;
}
