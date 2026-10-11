/* Synthetic tests for unit 0x38 private helper +0x0000 and entries 93, 94,
 * 96: every key against several [0x6817] values, every flag byte for the
 * two bit-gated entries, unchanged-state checks, and composition. */
#include "poolrad/recovered/overlay_unit38_helper.h"

#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static int test_helper(void)
{
    unsigned key, stored;
    for (key = 0; key < 256u; ++key) {
        for (stored = 0; stored < 256u; stored += 3u) {          /* includes 0 and 0xFF */
            pr_u38h_state s;
            unsigned paths = 99u;
            s.b6817 = (uint8_t)stored; s.b6818 = (uint8_t)(key ^ 0x5Au); s.b6819 = (uint8_t)(stored + 7u);
            CHECK(pr_u38_helper_0000(&s, (uint8_t)key, &paths) == PR_U38_OK);
            if (key == 0 || key == stored) {
                CHECK(paths == PR_U38_PATH_EFFECT && s.b6817 == 0 && s.b6818 == 0);
            } else {
                CHECK(paths == 0 && s.b6817 == stored && s.b6818 == (uint8_t)(key ^ 0x5Au));
            }
            CHECK(s.b6819 == (uint8_t)(stored + 7u));
        }
    }
    /* key 0 clears even when [0x6817] is nonzero and [0x6818] already zero */
    {
        pr_u38h_state s = {0x77, 0x00, 0xFF};
        unsigned paths;
        CHECK(pr_u38_helper_0000(&s, 0, &paths) == PR_U38_OK && paths == PR_U38_PATH_EFFECT);
        CHECK(s.b6817 == 0 && s.b6818 == 0 && s.b6819 == 0xFF);
        /* a second call with a nonzero key no longer matches: nothing changes */
        s.b6818 = 0x42;
        CHECK(pr_u38_helper_0000(&s, 0x77, &paths) == PR_U38_OK && paths == 0 && s.b6818 == 0x42);
    }
    CHECK(pr_u38_helper_0000(NULL, 0, NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry93(void)
{
    unsigned stored;
    for (stored = 0; stored < 256u; ++stored) {
        pr_u38h_state s;
        unsigned paths = 99u;
        s.b6817 = (uint8_t)stored; s.b6818 = 0xA5; s.b6819 = (uint8_t)stored;
        CHECK(pr_u38_entry93(&s, &paths) == PR_U38_OK);
        if (stored == 0x34u) CHECK(paths == PR_U38_PATH_EFFECT && s.b6817 == 0 && s.b6818 == 0);
        else CHECK(paths == 0 && s.b6817 == stored && s.b6818 == 0xA5);
        CHECK(s.b6819 == (uint8_t)stored);     /* entry 93 never looks at the flag byte */
    }
    CHECK(pr_u38_entry93(NULL, NULL) == PR_U38_ARGUMENT);
    return 0;
}

typedef pr_u38_status (*entry_fn)(pr_u38h_state *, unsigned *);

static int sweep_bit_entry(entry_fn fn, uint8_t mask)
{
    unsigned flags, stored;
    for (flags = 0; flags < 256u; ++flags) {
        for (stored = 0; stored < 256u; stored += 51u) {        /* 0, 51, ..., 255 */
            pr_u38h_state s;
            unsigned paths = 99u;
            s.b6817 = (uint8_t)stored; s.b6818 = (uint8_t)(flags + 1u); s.b6819 = (uint8_t)flags;
            CHECK(fn(&s, &paths) == PR_U38_OK);
            if (flags & mask) {
                /* key 0: clears regardless of [0x6817] */
                CHECK(paths == (PR_U38_PATH_EFFECT | PR_U38_PATH_SECOND));
                CHECK(s.b6817 == 0 && s.b6818 == 0);
            } else {
                CHECK(paths == 0 && s.b6817 == stored && s.b6818 == (uint8_t)(flags + 1u));
            }
            CHECK(s.b6819 == (uint8_t)flags);
        }
    }
    CHECK(fn(NULL, NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entries_94_96(void)
{
    pr_u38h_state s;
    unsigned paths;
    CHECK(sweep_bit_entry(pr_u38_entry94, 0x02u) == 0);
    CHECK(sweep_bit_entry(pr_u38_entry96, 0x01u) == 0);
    /* bit isolation: bit 1 alone fires 94 only, bit 0 alone fires 96 only */
    s.b6817 = 0x10; s.b6818 = 0x20; s.b6819 = 0x02;
    CHECK(pr_u38_entry96(&s, &paths) == PR_U38_OK && paths == 0 && s.b6818 == 0x20);
    CHECK(pr_u38_entry94(&s, &paths) == PR_U38_OK && paths == 3 && s.b6818 == 0 && s.b6817 == 0);
    s.b6817 = 0x10; s.b6818 = 0x20; s.b6819 = 0x01;
    CHECK(pr_u38_entry94(&s, &paths) == PR_U38_OK && paths == 0 && s.b6818 == 0x20);
    CHECK(pr_u38_entry96(&s, &paths) == PR_U38_OK && paths == 3 && s.b6818 == 0 && s.b6817 == 0);
    return 0;
}

/* Composition: a key set elsewhere ([0x6817] = 0x34 as unit 0x9e entry 20
 * would store from its argument) makes entry 93 clear; afterwards 93 is
 * inert until the key is set again, while 94/96 still clear via key 0. */
static int test_composition(void)
{
    pr_u38h_state s = {0x34, 0x09, 0x00};
    unsigned paths;
    CHECK(pr_u38_entry93(&s, &paths) == PR_U38_OK && paths == 1 && s.b6818 == 0 && s.b6817 == 0);
    s.b6818 = 0x09;
    CHECK(pr_u38_entry93(&s, &paths) == PR_U38_OK && paths == 0 && s.b6818 == 0x09);
    s.b6819 = 0x03;
    CHECK(pr_u38_entry94(&s, &paths) == PR_U38_OK && paths == 3 && s.b6818 == 0);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_helper();
    rc |= test_entry93();
    rc |= test_entries_94_96();
    rc |= test_composition();
    if (rc == 0) puts("overlay unit 0x38 helper tests passed");
    return rc;
}
