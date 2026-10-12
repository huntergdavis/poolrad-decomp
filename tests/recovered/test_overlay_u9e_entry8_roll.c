#include "poolrad/recovered/overlay_u9e_entry8_roll.h"

#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static int vectors(void)
{
    static const struct {
        uint32_t initial_seed, final_seed;
        uint16_t count, range;
        uint8_t al;
    } v[] = {
        { UINT32_C(0), UINT32_C(0), 0, 6, 0 },
        { UINT32_C(0), UINT32_C(1), 1, 6, 1 },
        { UINT32_C(0), UINT32_C(0x08088406), 2, 6, 6 },
        { UINT32_C(0x12345678), UINT32_C(0xD9A484B7), 3, 6, 7 },
        { UINT32_C(0x12345678), UINT32_C(0xD9A484B7), 3, 0, 3 },
        { UINT32_C(0x12345678), UINT32_C(0xAFE0484B), 255, 1, 255 },
        { UINT32_C(0xFFFFFFFF), UINT32_C(0x21DA862B), 4, 255, 71 },
        /* AH of both Pascal WORD arguments is ignored. */
        { UINT32_C(0), UINT32_C(0x08088406), 0x0102, 0x0206, 6 },
        { UINT32_C(0x12345678), UINT32_C(0x12345678), 0x0100, 6, 0 }
    };
    unsigned i;
    for (i = 0; i < sizeof v / sizeof v[0]; ++i) {
        uint32_t seed = v[i].initial_seed;
        uint8_t al = 0xA5;
        pr_u9e_entry8_report r;
        CHECK(pr_u9e_entry8_roll(v[i].count, v[i].range, &seed, &al, &r)
              == PR_U9E_ENTRY8_OK);
        CHECK(seed == v[i].final_seed && al == v[i].al);
        CHECK(r.count == (uint8_t)v[i].count);
        CHECK(r.range == (uint8_t)v[i].range);
        CHECK(r.random_calls == (uint8_t)v[i].count);
        CHECK(r.final_seed == seed);
    }
    return 0;
}

static int intermediate_seed(void)
{
    uint32_t seed = 0;
    uint8_t al;
    pr_u9e_entry8_report r;
    CHECK(pr_u9e_entry8_roll(1, 6, &seed, &al, &r) == PR_U9E_ENTRY8_OK);
    CHECK(seed == UINT32_C(1) && al == 1 && r.final_seed == seed);
    CHECK(pr_u9e_entry8_roll(1, 6, &seed, &al, &r) == PR_U9E_ENTRY8_OK);
    CHECK(seed == UINT32_C(0x08088406) && al == 5 && r.final_seed == seed);
    CHECK(pr_u9e_entry8_roll(1, 0, &seed, &al, &r) == PR_U9E_ENTRY8_OK);
    CHECK(seed == UINT32_C(0xDC6DAC1F) && al == 1 && r.final_seed == seed);
    return 0;
}

static int boundaries(void)
{
    uint32_t seed = UINT32_C(0x12345678);
    uint8_t al = 0xA5;
    pr_u9e_entry8_report r;
    CHECK(pr_u9e_entry8_roll(0, 6, NULL, &al, &r) == PR_U9E_ENTRY8_OK);
    CHECK(al == 0 && r.random_calls == 0);
    al = 0xA5;
    CHECK(pr_u9e_entry8_roll(1, 6, NULL, &al, &r) == PR_U9E_ENTRY8_SEED_MISSING);
    CHECK(al == 0xA5 && r.random_calls == 0);
    CHECK(pr_u9e_entry8_roll(1, 6, &seed, NULL, &r) == PR_U9E_ENTRY8_ARGUMENT);
    CHECK(seed == UINT32_C(0x12345678));
    CHECK(pr_u9e_entry8_roll(1, 6, &seed, &al, NULL) == PR_U9E_ENTRY8_OK);
    CHECK(seed == UINT32_C(0xCB5B9059));
    return 0;
}

int main(void)
{
    CHECK(vectors() == 0);
    CHECK(intermediate_seed() == 0);
    CHECK(boundaries() == 0);
    puts("overlay_u9e_entry8_roll: ok");
    return 0;
}
