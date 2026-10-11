/* Synthetic edge-byte and branch tests for unit 0x38 entries 6, 75, 82, 97.
 * No DOS bytes. Expected values are hand-derived from the decoded
 * semantics, not recomputed with the routine's own arithmetic. */
#include "poolrad/recovered/overlay_unit38_effects_next.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint8_t seg_a[0x200];   /* segment 0x2000: object behind record slot 0x108 */
    uint8_t seg_b[0x200];   /* segment 0x3000: object behind P[0xA..0xD] */
    unsigned calls;
    size_t last_need;
} fixture;

static uint8_t *resolve(void *context, pr_u38_far_ptr p, size_t need)
{
    fixture *f = (fixture *)context;
    uint8_t *base = NULL;
    f->calls++;
    f->last_need = need;
    if (p.segment == 0x2000u) base = f->seg_a;
    else if (p.segment == 0x3000u) base = f->seg_b;
    if (!base || (size_t)p.offset + need > 0x200u) return NULL;
    return base + p.offset;
}

static void put_far(uint8_t *at, uint16_t offset, uint16_t segment)
{
    at[0] = (uint8_t)offset; at[1] = (uint8_t)(offset >> 8);
    at[2] = (uint8_t)segment; at[3] = (uint8_t)(segment >> 8);
}

static pr_u38n_state seeded(unsigned seed)
{
    pr_u38n_state s;
    s.b6816 = (uint8_t)(seed * 7u + 1u); s.b6818 = (uint8_t)(seed * 11u + 2u);
    s.b6819 = (uint8_t)(seed * 13u + 3u); s.b681c = (uint8_t)(seed * 17u + 4u);
    s.b6822 = (uint8_t)(seed * 31u + 8u); s.b682a = (uint8_t)(seed * 37u + 9u);
    s.scratch_6826.offset = (uint16_t)(seed * 0x111u);
    s.scratch_6826.segment = (uint16_t)(seed * 0x222u + 1u);
    return s;
}

static int same_except(const pr_u38n_state *a, const pr_u38n_state *b, const char *except)
{
#define FIELD(name) if (!strstr(except, #name) && a->name != b->name) return 0;
    FIELD(b6816) FIELD(b6818) FIELD(b6819) FIELD(b681c) FIELD(b6822) FIELD(b682a)
#undef FIELD
    if (!strstr(except, "scratch") &&
        (a->scratch_6826.offset != b->scratch_6826.offset ||
         a->scratch_6826.segment != b->scratch_6826.segment)) return 0;
    return 1;
}

static int test_entry6(void)
{
    fixture f;
    uint8_t record[PR_U38_RECORD_SIZE];
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38n_state s = seeded(v), before;
        unsigned cls = 99u, expect;
        memset(&f, 0, sizeof f);
        memset(record, 0x5A, sizeof record);
        put_far(record + 0x108, 0x20, 0x2000);
        put_far(f.seg_a + 0x20 + 0x0A, 0x100, 0x3000);
        memset(f.seg_b + 0x100, (int)(~v & 0xFFu), 0xA0);
        f.seg_b[0x100 + 0x9F] = (uint8_t)v;
        before = s;
        CHECK(pr_u38_entry6(&s, record, resolve, &f, &cls) == PR_U38_OK);
        expect = (v == 0x0Au) ? 1u : (v == 0x09u || v == 0x0Cu) ? 2u : (v == 0x04u) ? 3u : 0u;
        CHECK(cls == expect);
        CHECK(f.calls == 2 && f.last_need == 0xA0);
        CHECK(s.scratch_6826.offset == 0x100 && s.scratch_6826.segment == 0x3000);
        CHECK(s.b6822 == (uint8_t)(before.b6822 + expect));
        CHECK(s.b6818 == (uint8_t)(before.b6818 + expect));
        CHECK(s.b6819 == 9);                       /* always, even for class 0 */
        CHECK(same_except(&s, &before, "b6822 b6818 b6819 scratch"));
    }
    /* wraparound with class 3 */
    {
        pr_u38n_state s = seeded(0); unsigned cls;
        memset(&f, 0, sizeof f);
        put_far(record + 0x108, 0x20, 0x2000);
        put_far(f.seg_a + 0x20 + 0x0A, 0x100, 0x3000);
        f.seg_b[0x100 + 0x9F] = 4; s.b6822 = 0xFE; s.b6818 = 0xFF; s.b6819 = 0;
        CHECK(pr_u38_entry6(&s, record, resolve, &f, &cls) == PR_U38_OK && cls == 3);
        CHECK(s.b6822 == 0x01 && s.b6818 == 0x02 && s.b6819 == 9);
    }
    /* P unresolvable: nothing written; Q unresolvable: only the scratch pointer */
    {
        pr_u38n_state s = seeded(3), before = s; unsigned cls = 99u;
        memset(&f, 0, sizeof f);
        put_far(record + 0x108, 0x20, 0x5000);
        CHECK(pr_u38_entry6(&s, record, resolve, &f, &cls) == PR_U38_ARGUMENT);
        CHECK(cls == 0 && f.calls == 1 && same_except(&s, &before, ""));
        put_far(record + 0x108, 0x20, 0x2000);
        put_far(f.seg_a + 0x20 + 0x0A, 0x1F0, 0x3000);
        CHECK(pr_u38_entry6(&s, record, resolve, &f, &cls) == PR_U38_ARGUMENT);
        CHECK(s.scratch_6826.offset == 0x1F0 && s.scratch_6826.segment == 0x3000);
        CHECK(same_except(&s, &before, "scratch") && s.b6819 == before.b6819);
        CHECK(pr_u38_entry6(NULL, record, resolve, &f, NULL) == PR_U38_ARGUMENT);
        CHECK(pr_u38_entry6(&s, NULL, resolve, &f, NULL) == PR_U38_ARGUMENT);
        CHECK(pr_u38_entry6(&s, record, NULL, &f, NULL) == PR_U38_ARGUMENT);
    }
    return 0;
}

static unsigned bracket_for(unsigned v)
{
    if (v >= 4u && v <= 6u) return 1u;
    if (v >= 7u && v <= 10u) return 2u;
    if (v >= 11u && v <= 13u) return 3u;
    if (v >= 14u && v <= 17u) return 4u;
    if (v >= 18u && v <= 20u) return 5u;
    return 0u;   /* undefined in the original */
}

static int test_entry75(void)
{
    uint8_t record[PR_U38_RECORD_SIZE], copy[PR_U38_RECORD_SIZE];
    unsigned v, residue;
    /* gate: any nonzero [0x682A] skips everything */
    for (v = 1; v < 256u; ++v) {
        pr_u38n_state s = seeded(v), before;
        unsigned paths = 99u, br = 99u;
        s.b682a = (uint8_t)v; before = s;
        memset(record, 10, sizeof record);
        CHECK(pr_u38_entry75(&s, record, 0x77, &paths, &br) == PR_U38_OK);
        CHECK(paths == 0 && br == 0 && same_except(&s, &before, ""));
    }
    /* every record value with three residues; defined brackets ignore the residue */
    for (v = 0; v < 256u; ++v) {
        for (residue = 0; residue < 256u; residue += 127u) {    /* 0, 127, 254 */
            pr_u38n_state s = seeded(v + residue), before;
            unsigned paths = 99u, br = 99u, expect = bracket_for(v);
            s.b682a = 0; before = s;
            memset(record, (int)(v ^ 0xC3u), sizeof record);
            record[0x14] = (uint8_t)v;
            memcpy(copy, record, sizeof record);
            CHECK(pr_u38_entry75(&s, record, (uint8_t)residue, &paths, &br) == PR_U38_OK);
            CHECK(memcmp(record, copy, sizeof record) == 0);
            if (expect) {
                CHECK(paths == PR_U38_PATH_EFFECT && br == expect);
                CHECK(s.b6816 == (uint8_t)(before.b6816 + expect));
            } else {
                CHECK(paths == (PR_U38_PATH_EFFECT | PR_U38_PATH_SECOND) && br == residue);
                CHECK(s.b6816 == (uint8_t)(before.b6816 + residue));
            }
            CHECK(same_except(&s, &before, "b6816"));
        }
    }
    /* bracket boundaries and wraparound of the byte add */
    {
        static const struct { unsigned v, br; } edge[] = {
            {3, 0}, {4, 1}, {6, 1}, {7, 2}, {10, 2}, {11, 3}, {13, 3},
            {14, 4}, {17, 4}, {18, 5}, {20, 5}, {21, 0}, {255, 0}};
        size_t i;
        for (i = 0; i < sizeof edge / sizeof edge[0]; ++i) {
            pr_u38n_state s = seeded(1); unsigned br;
            s.b682a = 0; s.b6816 = 0xFE;
            memset(record, 0, sizeof record); record[0x14] = (uint8_t)edge[i].v;
            CHECK(pr_u38_entry75(&s, record, 9, NULL, &br) == PR_U38_OK);
            CHECK(br == (edge[i].br ? edge[i].br : 9u));
            CHECK(s.b6816 == (uint8_t)(0xFEu + br));
        }
    }
    { pr_u38n_state s = seeded(2); CHECK(pr_u38_entry75(&s, NULL, 0, NULL, NULL) == PR_U38_ARGUMENT); }
    CHECK(pr_u38_entry75(NULL, record, 0, NULL, NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry82(void)
{
    uint8_t record[PR_U38_RECORD_SIZE], copy[PR_U38_RECORD_SIZE];
    unsigned v, cap;
    for (v = 0; v < 256u; ++v) {
        for (cap = 0; cap < 256u; cap += 5u) {
            unsigned paths = 99u, plus3 = (v + 3u) & 0xFFu;
            memset(record, (int)(v + cap), sizeof record);
            record[0x11B] = (uint8_t)v; record[0x32] = (uint8_t)cap;
            memcpy(copy, record, sizeof record);
            CHECK(pr_u38_entry82(record, &paths) == PR_U38_OK);
            if (plus3 > cap) { CHECK(paths == PR_U38_PATH_EFFECT && record[0x11B] == cap); copy[0x11B] = (uint8_t)cap; }
            else { CHECK(paths == 0 && record[0x11B] == plus3); copy[0x11B] = (uint8_t)plus3; }
            CHECK(memcmp(record, copy, sizeof record) == 0);
        }
    }
    /* the add wraps before the compare: 0xFE + 3 = 0x01 is not clamped by a cap of 0x10 */
    memset(record, 0, sizeof record); record[0x11B] = 0xFE; record[0x32] = 0x10;
    CHECK(pr_u38_entry82(record, NULL) == PR_U38_OK && record[0x11B] == 0x01);
    /* equal to the cap after the add: not clamped */
    record[0x11B] = 0x0D; record[0x32] = 0x10;
    CHECK(pr_u38_entry82(record, NULL) == PR_U38_OK && record[0x11B] == 0x10);
    CHECK(pr_u38_entry82(NULL, NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry97(void)
{
    static const struct { unsigned flags, limit, start, result, iterations; } cases[] = {
        {0x00, 3, 10, 10, 0},      /* bit 0 clear: untouched */
        {0xFE, 3, 10, 10, 0},      /* other bits do not enable */
        {0x01, 0, 10, 10, 0},      /* limit 0: untouched */
        {0x01, 3, 10, 7, 3},       /* 9, 8, 7 */
        {0x01, 3, 4, 3, 3},        /* 3, 3, 3 */
        {0x01, 5, 5, 5, 5},        /* 4 < 5 -> 5 each time */
        {0x01, 1, 0, 0xFF, 1},     /* 0 - 1 wraps to 0xFF, which is >= 1 */
        {0x01, 2, 0, 0xFE, 2},     /* 0xFF, 0xFE */
        {0x01, 255, 0, 0xFF, 255}, /* 0xFF then 0xFE < 0xFF -> 0xFF, repeated */
        {0x01, 255, 255, 255, 255},
        {0x01, 200, 100, 200, 200},/* 99 < 200 -> 200 immediately and stays */
        {0x01, 7, 20, 13, 7},
    };
    size_t i;
    unsigned limit;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        pr_u38n_state s = seeded((unsigned)i), before;
        unsigned iterations = 99u;
        s.b6819 = (uint8_t)cases[i].flags; s.b681c = (uint8_t)cases[i].limit;
        s.b6818 = (uint8_t)cases[i].start; before = s;
        CHECK(pr_u38_entry97(&s, &iterations) == PR_U38_OK);
        CHECK(iterations == cases[i].iterations);
        CHECK(s.b6818 == cases[i].result);
        CHECK(same_except(&s, &before, "b6818"));
    }
    /* property: for every limit, the body runs limit times and the result is
     * never below the limit once at least one iteration ran */
    for (limit = 1; limit < 256u; ++limit) {
        pr_u38n_state s = seeded(limit);
        unsigned iterations;
        s.b6819 = 1; s.b681c = (uint8_t)limit; s.b6818 = 0x80;
        CHECK(pr_u38_entry97(&s, &iterations) == PR_U38_OK);
        CHECK(iterations == limit);
        CHECK(s.b6818 >= limit);
        CHECK(s.b6818 == (0x80u > limit ? (0x80u - limit > limit ? 0x80u - limit : limit) : limit));
    }
    CHECK(pr_u38_entry97(NULL, NULL) == PR_U38_ARGUMENT);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_entry6();
    rc |= test_entry75();
    rc |= test_entry82();
    rc |= test_entry97();
    if (rc == 0) puts("overlay unit 0x38 next effect-entry tests passed");
    return rc;
}
