/* Synthetic branch and side-effect tests for the sixteen unit 0x38 dispatch
 * entries in overlay_unit38_effects.c. No DOS bytes: records and objects are
 * fabricated buffers; far pointers resolve through a two-segment fixture. */
#include "poolrad/recovered/overlay_unit38_effects.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* Two synthetic far segments; anything else is unresolvable. */
typedef struct {
    uint8_t seg_a[0x200];     /* segment 0x2000: the object behind slot 0x108 */
    uint8_t seg_b[0x200];     /* segment 0x3000: the object behind P[0xA..0xD] */
    unsigned calls;
    pr_u38_far_ptr last;
    size_t last_need;
} fixture;

static uint8_t *resolve(void *context, pr_u38_far_ptr p, size_t need)
{
    fixture *f = (fixture *)context;
    uint8_t *base = NULL;
    f->calls++;
    f->last = p;
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

static int state_equal_except(const pr_u38_state *a, const pr_u38_state *b,
                              const char *except)
{
    /* Compare every field whose name is not listed in `except`. */
#define FIELD(name) if (!strstr(except, #name) && a->name != b->name) return 0;
    FIELD(b6816) FIELD(b6818) FIELD(b6819) FIELD(b681a) FIELD(b681b)
    FIELD(b681d) FIELD(b681e) FIELD(b6822) FIELD(b6825) FIELD(b6de3)
#undef FIELD
    if (!strstr(except, "scratch") &&
        (a->scratch_6826.offset != b->scratch_6826.offset ||
         a->scratch_6826.segment != b->scratch_6826.segment)) return 0;
    return 1;
}

static pr_u38_state seeded(unsigned seed)
{
    pr_u38_state s;
    s.b6816 = (uint8_t)(seed * 7u + 1u); s.b6818 = (uint8_t)(seed * 11u + 2u);
    s.b6819 = (uint8_t)(seed * 13u + 3u); s.b681a = (uint8_t)(seed * 17u + 4u);
    s.b681b = (uint8_t)(seed * 19u + 5u); s.b681d = (uint8_t)(seed * 23u + 6u);
    s.b681e = (uint8_t)(seed * 29u + 7u); s.b6822 = (uint8_t)(seed * 31u + 8u);
    s.b6825 = (uint8_t)(seed * 37u + 9u); s.b6de3 = (uint8_t)(seed * 41u + 10u);
    s.scratch_6826.offset = (uint16_t)(seed * 0x111u);
    s.scratch_6826.segment = (uint16_t)(seed * 0x222u + 1u);
    return s;
}

static int test_entry2(void)
{
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38_state s = seeded(v), before;
        s.b6825 = (uint8_t)v; s.b6822 = (uint8_t)(255u - v);
        before = s;
        CHECK(pr_u38_entry2(&s) == PR_U38_OK);
        CHECK(s.b6825 == (uint8_t)(v + 5u));
        CHECK(s.b6822 == (uint8_t)(255u - v + 1u));
        CHECK(state_equal_except(&s, &before, "b6825 b6822"));
    }
    CHECK(pr_u38_entry2(NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry3(void)
{
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38_state s = seeded(v), before;
        unsigned paths = 99u;
        s.b6825 = (uint8_t)v; s.b6822 = (uint8_t)v;
        before = s;
        CHECK(pr_u38_entry3(&s, &paths) == PR_U38_OK);
        if (v >= 5u) {
            CHECK(s.b6825 == (uint8_t)(v - 5u));
            CHECK(paths == PR_U38_PATH_EFFECT);
        } else {
            CHECK(s.b6825 == 0);
            CHECK(paths == 0);
        }
        CHECK(s.b6822 == (uint8_t)(v - 1u));     /* 0 wraps to 0xFF */
        CHECK(state_equal_except(&s, &before, "b6825 b6822"));
    }
    /* entry 2 then entry 3 restores [0x6825] when the subtract path runs */
    {
        pr_u38_state s = seeded(1); s.b6825 = 0xFC; s.b6822 = 0x10;
        CHECK(pr_u38_entry2(&s) == PR_U38_OK && s.b6825 == 0x01);
        CHECK(pr_u38_entry3(&s, NULL) == PR_U38_OK && s.b6825 == 0 && s.b6822 == 0x10);
    }
    CHECK(pr_u38_entry3(NULL, NULL) == PR_U38_ARGUMENT);
    return 0;
}

/* Shared sweep for entries 18, 77, 98, 102: tested bit, halving, extra add. */
typedef pr_u38_status (*bit_fn)(pr_u38_state *, unsigned *);
static int sweep_bit_halver(bit_fn fn, uint8_t mask, unsigned add_6816)
{
    unsigned flags, value;
    for (flags = 0; flags < 256u; ++flags) {
        for (value = 0; value < 256u; value += 17u) {
            pr_u38_state s = seeded(flags + value), before;
            unsigned paths = 99u;
            s.b6819 = (uint8_t)flags; s.b6818 = (uint8_t)value;
            before = s;
            CHECK(fn(&s, &paths) == PR_U38_OK);
            if (flags & mask) {
                CHECK(paths == PR_U38_PATH_EFFECT);
                CHECK(s.b6818 == (uint8_t)(value >> 1));
                CHECK(s.b6816 == (uint8_t)(before.b6816 + add_6816));
            } else {
                CHECK(paths == 0);
                CHECK(s.b6818 == before.b6818 && s.b6816 == before.b6816);
            }
            CHECK(state_equal_except(&s, &before, "b6818 b6816"));
        }
    }
    CHECK(fn(NULL, NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_bit_halvers(void)
{
    pr_u38_state s;
    unsigned paths;
    CHECK(sweep_bit_halver(pr_u38_entry18, 0x01u, 3u) == 0);
    CHECK(sweep_bit_halver(pr_u38_entry77, 0x01u, 0u) == 0);
    CHECK(sweep_bit_halver(pr_u38_entry98, 0x04u, 0u) == 0);
    CHECK(sweep_bit_halver(pr_u38_entry102, 0x02u, 0u) == 0);
    /* each tested bit alone selects exactly its own routines */
    s = seeded(3); s.b6819 = 0x01; s.b6818 = 0x80; s.b6816 = 0xFE;
    CHECK(pr_u38_entry98(&s, &paths) == PR_U38_OK && paths == 0 && s.b6818 == 0x80);
    CHECK(pr_u38_entry102(&s, &paths) == PR_U38_OK && paths == 0 && s.b6818 == 0x80);
    CHECK(pr_u38_entry77(&s, &paths) == PR_U38_OK && paths == 1 && s.b6818 == 0x40);
    CHECK(pr_u38_entry18(&s, &paths) == PR_U38_OK && paths == 1 && s.b6818 == 0x20);
    CHECK(s.b6816 == 0x01);                     /* 0xFE + 3 wraps */
    /* repeated halving reaches zero and stays there */
    s.b6819 = 0x02; s.b6818 = 0xFF;
    { int i; for (i = 0; i < 8; ++i) CHECK(pr_u38_entry102(&s, NULL) == PR_U38_OK); }
    CHECK(s.b6818 == 0);
    CHECK(pr_u38_entry102(&s, NULL) == PR_U38_OK && s.b6818 == 0);
    return 0;
}

static int test_entry25(void)
{
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38_state s = seeded(v), before;
        s.b6818 = (uint8_t)v; before = s;
        CHECK(pr_u38_entry25(&s) == PR_U38_OK);
        CHECK(s.b6818 == (uint8_t)(v - (v >> 2)));
        CHECK(s.b6818 <= v);                      /* never wraps */
        CHECK(state_equal_except(&s, &before, "b6818"));
    }
    { pr_u38_state s = seeded(0); s.b6818 = 0xFF; CHECK(pr_u38_entry25(&s) == PR_U38_OK && s.b6818 == 0xC0); }
    { pr_u38_state s = seeded(0); s.b6818 = 3; CHECK(pr_u38_entry25(&s) == PR_U38_OK && s.b6818 == 3); }
    { pr_u38_state s = seeded(0); s.b6818 = 4; CHECK(pr_u38_entry25(&s) == PR_U38_OK && s.b6818 == 3); }
    CHECK(pr_u38_entry25(NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry36(void)
{
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38_state s = seeded(v), before;
        s.b681a = (uint8_t)v; before = s;
        CHECK(pr_u38_entry36(&s) == PR_U38_OK);
        CHECK(s.b681a == (uint8_t)(v >> 1));
        CHECK(state_equal_except(&s, &before, "b681a"));
    }
    CHECK(pr_u38_entry36(NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry59(void)
{
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38_state s = seeded(v), before;
        s.b6822 = (uint8_t)v; s.b681e = (uint8_t)(v ^ 0x5A); before = s;
        CHECK(pr_u38_entry59(&s) == PR_U38_OK);
        CHECK(s.b681e == 1);
        CHECK(s.b6822 == (uint8_t)(v - 4u));
        CHECK(state_equal_except(&s, &before, "b681e b6822"));
    }
    CHECK(pr_u38_entry59(NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry13(void)
{
    uint8_t record[PR_U38_RECORD_SIZE], copy[PR_U38_RECORD_SIZE];
    uint8_t source[PR_U38_SOURCE_MIN_SIZE];
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        size_t i;
        for (i = 0; i < sizeof record; ++i) record[i] = (uint8_t)(i * 3u + v);
        memcpy(copy, record, sizeof record);
        source[0] = (uint8_t)~v; source[1] = 0x11; source[2] = 0x22; source[3] = (uint8_t)v;
        CHECK(pr_u38_entry13(record, source) == PR_U38_OK);
        CHECK(record[0x15] == (uint8_t)v);
        copy[0x15] = (uint8_t)v;
        CHECK(memcmp(record, copy, sizeof record) == 0);   /* nothing else written */
        CHECK(source[3] == (uint8_t)v);                     /* source untouched */
    }
    CHECK(pr_u38_entry13(NULL, source) == PR_U38_ARGUMENT);
    CHECK(pr_u38_entry13(record, NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry16(void)
{
    uint8_t record[PR_U38_RECORD_SIZE], copy[PR_U38_RECORD_SIZE];
    unsigned v, b;
    for (v = 0; v < 256u; ++v) {
        for (b = 0; b < 256u; b += 15u) {       /* includes 0x0F */
            pr_u38_state s = seeded(v + b), before;
            unsigned paths = 99u, expect = 0;
            memset(record, (int)(v ^ 0x33u), sizeof record);
            record[0x111] = (uint8_t)v;
            memcpy(copy, record, sizeof record);
            s.b681b = (uint8_t)b; before = s;
            CHECK(pr_u38_entry16(&s, record, &paths) == PR_U38_OK);
            if (v < 0x39u) { expect |= PR_U38_PATH_EFFECT; copy[0x111] = 0x39; }
            CHECK(record[0x111] == (v < 0x39u ? 0x39u : v));
            CHECK(memcmp(record, copy, sizeof record) == 0);
            CHECK(s.b6816 == (uint8_t)(before.b6816 + 1u));
            if (b == 0x0Fu) { expect |= PR_U38_PATH_SECOND; CHECK(s.b6818 == 0); }
            else CHECK(s.b6818 == before.b6818);
            CHECK(paths == expect);
            CHECK(state_equal_except(&s, &before, "b6816 b6818"));
        }
    }
    { pr_u38_state s = seeded(1); CHECK(pr_u38_entry16(&s, NULL, NULL) == PR_U38_ARGUMENT); }
    CHECK(pr_u38_entry16(NULL, record, NULL) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry28(void)
{
    uint8_t record[PR_U38_RECORD_SIZE], copy[PR_U38_RECORD_SIZE];
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38_state s = seeded(v), before;
        memset(record, (int)v, sizeof record);
        record[0x111] = (uint8_t)v; record[0x112] = (uint8_t)(v + 2u);
        memcpy(copy, record, sizeof record);
        s.b6822 = (uint8_t)(v + 1u); s.b6816 = (uint8_t)(v + 3u); before = s;
        CHECK(pr_u38_entry28(&s, record) == PR_U38_OK);
        CHECK(s.b6822 == (uint8_t)(v + 1u - 4u));
        CHECK(record[0x111] == (uint8_t)(v - 4u));
        CHECK(record[0x112] == (uint8_t)(v + 2u - 4u));
        CHECK(s.b6816 == (uint8_t)(v + 3u - 4u));
        copy[0x111] = (uint8_t)(v - 4u); copy[0x112] = (uint8_t)(v + 2u - 4u);
        CHECK(memcmp(record, copy, sizeof record) == 0);
        CHECK(state_equal_except(&s, &before, "b6822 b6816"));
    }
    { pr_u38_state s = seeded(1); CHECK(pr_u38_entry28(&s, NULL) == PR_U38_ARGUMENT); }
    CHECK(pr_u38_entry28(NULL, record) == PR_U38_ARGUMENT);
    return 0;
}

static int test_entry32(void)
{
    fixture f;
    uint8_t record[PR_U38_RECORD_SIZE];
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38_state s = seeded(v), before;
        unsigned paths = 99u;
        memset(&f, 0, sizeof f);
        memset(record, 0x5A, sizeof record);
        put_far(record + 0x108, 0x40, 0x2000);
        f.seg_a[0x40 + 3] = (uint8_t)v;
        before = s;
        CHECK(pr_u38_entry32(&s, record, resolve, &f, &paths) == PR_U38_OK);
        CHECK(f.calls == 1 && f.last.offset == 0x40 && f.last.segment == 0x2000 && f.last_need == 4);
        if (v >= 1u && v <= 0x7Fu) {            /* signed > 0 */
            CHECK(paths == PR_U38_PATH_EFFECT && s.b681e == 1 && s.b6822 == 0xFF);
        } else {
            CHECK(paths == 0);
            CHECK(s.b681e == before.b681e && s.b6822 == before.b6822);
        }
        CHECK(state_equal_except(&s, &before, "b681e b6822"));
    }
    /* unresolvable slot: nothing changes */
    {
        pr_u38_state s = seeded(2), before = s;
        unsigned paths = 99u;
        memset(&f, 0, sizeof f);
        put_far(record + 0x108, 0x10, 0x4000);
        CHECK(pr_u38_entry32(&s, record, resolve, &f, &paths) == PR_U38_ARGUMENT);
        CHECK(paths == 0 && state_equal_except(&s, &before, ""));
        CHECK(pr_u38_entry32(NULL, record, resolve, &f, NULL) == PR_U38_ARGUMENT);
        CHECK(pr_u38_entry32(&s, NULL, resolve, &f, NULL) == PR_U38_ARGUMENT);
        CHECK(pr_u38_entry32(&s, record, NULL, &f, NULL) == PR_U38_ARGUMENT);
    }
    return 0;
}

static int test_entry48(void)
{
    fixture f;
    uint8_t record[PR_U38_RECORD_SIZE];
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38_state s = seeded(v), before;
        unsigned paths = 99u;
        memset(&f, 0, sizeof f);
        memset(f.seg_a, 0xEE, sizeof f.seg_a);
        memset(record, 0x5A, sizeof record);
        put_far(record + 0x108, 0x80, 0x2000);
        s.b681d = (uint8_t)v; before = s;
        CHECK(pr_u38_entry48(&s, record, resolve, &f, &paths) == PR_U38_OK);
        CHECK(f.calls == 1 && f.last_need == 7);
        CHECK(f.seg_a[0x80 + 6] == 0);
        CHECK(f.seg_a[0x80 + 5] == 0xEE && f.seg_a[0x80 + 7] == 0xEE);
        if (v != 0) CHECK(paths == PR_U38_PATH_EFFECT && s.b681a == 0);
        else CHECK(paths == 0 && s.b681a == before.b681a);
        CHECK(state_equal_except(&s, &before, "b681a"));
    }
    {
        pr_u38_state s = seeded(2), before = s;
        memset(&f, 0, sizeof f);
        put_far(record + 0x108, 0x1FC, 0x2000);   /* 7 bytes do not fit */
        CHECK(pr_u38_entry48(&s, record, resolve, &f, NULL) == PR_U38_ARGUMENT);
        CHECK(state_equal_except(&s, &before, ""));
        CHECK(pr_u38_entry48(NULL, record, resolve, &f, NULL) == PR_U38_ARGUMENT);
        CHECK(pr_u38_entry48(&s, NULL, resolve, &f, NULL) == PR_U38_ARGUMENT);
        CHECK(pr_u38_entry48(&s, record, NULL, &f, NULL) == PR_U38_ARGUMENT);
    }
    return 0;
}

static int test_entry74(void)
{
    unsigned de3, b22, src;
    uint8_t source[PR_U38_SOURCE_MIN_SIZE];
    for (de3 = 0; de3 < 256u; de3 += 85u) {       /* 0, 85, 170, 255 */
        for (b22 = 0; b22 < 256u; b22 += 51u) {   /* 0, 51, ..., 255 */
            for (src = 0; src < 256u; ++src) {
                pr_u38_state s = seeded(src), before;
                unsigned paths = 99u;
                s.b6de3 = (uint8_t)de3; s.b6822 = (uint8_t)b22; before = s;
                source[0] = 1; source[1] = 2; source[2] = 3; source[3] = (uint8_t)src;
                CHECK(pr_u38_entry74(&s, source, &paths) == PR_U38_OK);
                CHECK(source[0] == 1 && source[1] == 2 && source[2] == 3);
                if (de3 == 0 && b22 == 0) {
                    CHECK(paths == PR_U38_PATH_EFFECT);
                    CHECK(source[3] == (src & 0x0Fu));
                    CHECK(state_equal_except(&s, &before, ""));
                } else if ((src & 0x10u) == 0) {
                    CHECK(paths == PR_U38_PATH_SECOND);
                    CHECK(source[3] == (src | 0x10u));
                    CHECK(s.b6822 == 0xFF);
                    CHECK(state_equal_except(&s, &before, "b6822"));
                } else {
                    CHECK(paths == 0);
                    CHECK(source[3] == src);
                    CHECK(state_equal_except(&s, &before, ""));
                }
            }
        }
    }
    /* once marked, a second call changes nothing until [0x6822] and [0x6DE3] are both zero */
    {
        pr_u38_state s = seeded(4); unsigned paths;
        s.b6de3 = 0; s.b6822 = 1; source[3] = 0x0A;
        CHECK(pr_u38_entry74(&s, source, &paths) == PR_U38_OK && paths == PR_U38_PATH_SECOND && source[3] == 0x1A && s.b6822 == 0xFF);
        CHECK(pr_u38_entry74(&s, source, &paths) == PR_U38_OK && paths == 0 && source[3] == 0x1A);
        s.b6822 = 0;
        CHECK(pr_u38_entry74(&s, source, &paths) == PR_U38_OK && paths == PR_U38_PATH_EFFECT && source[3] == 0x0A);
        CHECK(pr_u38_entry74(NULL, source, NULL) == PR_U38_ARGUMENT);
        CHECK(pr_u38_entry74(&s, NULL, NULL) == PR_U38_ARGUMENT);
    }
    return 0;
}

static int test_entry4(void)
{
    fixture f;
    uint8_t record[PR_U38_RECORD_SIZE];
    unsigned v;
    for (v = 0; v < 256u; ++v) {
        pr_u38_state s = seeded(v), before;
        unsigned paths = 99u;
        memset(&f, 0, sizeof f);
        memset(record, 0x5A, sizeof record);
        put_far(record + 0x108, 0x20, 0x2000);           /* P */
        put_far(f.seg_a + 0x20 + 0x0A, 0x100, 0x3000);   /* Q = P[0xA..0xD] */
        memset(f.seg_b + 0x100, (int)(v ^ 0xA5u), 0xA0);
        f.seg_b[0x100 + 0x9F] = (uint8_t)v;
        before = s;
        CHECK(pr_u38_entry4(&s, record, resolve, &f, &paths) == PR_U38_OK);
        CHECK(f.calls == 2 && f.last.offset == 0x100 && f.last.segment == 0x3000 && f.last_need == 0xA0);
        CHECK(s.scratch_6826.offset == 0x100 && s.scratch_6826.segment == 0x3000);
        if (v == 4u) {
            CHECK(paths == PR_U38_PATH_EFFECT);
            CHECK(s.b6822 == (uint8_t)(before.b6822 + 2u));
            CHECK(s.b6818 == (uint8_t)(before.b6818 + 2u));
        } else {
            CHECK(paths == 0);
            CHECK(s.b6822 == before.b6822 && s.b6818 == before.b6818);
        }
        CHECK(state_equal_except(&s, &before, "b6822 b6818 scratch"));
    }
    /* wraparound of both accumulators */
    {
        pr_u38_state s = seeded(0); unsigned paths;
        memset(&f, 0, sizeof f);
        put_far(record + 0x108, 0x20, 0x2000);
        put_far(f.seg_a + 0x20 + 0x0A, 0x100, 0x3000);
        f.seg_b[0x100 + 0x9F] = 4; s.b6822 = 0xFF; s.b6818 = 0xFE;
        CHECK(pr_u38_entry4(&s, record, resolve, &f, &paths) == PR_U38_OK && paths == 1);
        CHECK(s.b6822 == 0x01 && s.b6818 == 0x00);
    }
    /* P unresolvable: scratch untouched; Q unresolvable: scratch already written */
    {
        pr_u38_state s = seeded(5), before = s; unsigned paths = 99u;
        memset(&f, 0, sizeof f);
        put_far(record + 0x108, 0x20, 0x7000);
        CHECK(pr_u38_entry4(&s, record, resolve, &f, &paths) == PR_U38_ARGUMENT);
        CHECK(paths == 0 && f.calls == 1 && state_equal_except(&s, &before, ""));
        put_far(record + 0x108, 0x20, 0x2000);
        put_far(f.seg_a + 0x20 + 0x0A, 0x1F0, 0x3000);   /* 0xA0 bytes do not fit */
        CHECK(pr_u38_entry4(&s, record, resolve, &f, &paths) == PR_U38_ARGUMENT);
        CHECK(f.calls == 3 && s.scratch_6826.offset == 0x1F0 && s.scratch_6826.segment == 0x3000);
        CHECK(state_equal_except(&s, &before, "scratch"));
        CHECK(pr_u38_entry4(NULL, record, resolve, &f, NULL) == PR_U38_ARGUMENT);
        CHECK(pr_u38_entry4(&s, NULL, resolve, &f, NULL) == PR_U38_ARGUMENT);
        CHECK(pr_u38_entry4(&s, record, NULL, &f, NULL) == PR_U38_ARGUMENT);
    }
    return 0;
}

/* Composition across the family on the shared accumulators. */
static int test_family_composition(void)
{
    pr_u38_state s = seeded(9);
    s.b6822 = 0x10; s.b6816 = 0x20; s.b6825 = 0x00; s.b681e = 0;
    CHECK(pr_u38_entry2(&s) == PR_U38_OK);              /* 6825=5, 6822=0x11 */
    CHECK(pr_u38_entry59(&s) == PR_U38_OK);             /* 681e=1, 6822=0x0D */
    CHECK(pr_u38_entry3(&s, NULL) == PR_U38_OK);        /* 6825=0, 6822=0x0C */
    CHECK(s.b6825 == 0 && s.b6822 == 0x0C && s.b681e == 1 && s.b6816 == 0x20);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_entry2();
    rc |= test_entry3();
    rc |= test_bit_halvers();
    rc |= test_entry25();
    rc |= test_entry36();
    rc |= test_entry59();
    rc |= test_entry13();
    rc |= test_entry16();
    rc |= test_entry28();
    rc |= test_entry32();
    rc |= test_entry48();
    rc |= test_entry74();
    rc |= test_entry4();
    rc |= test_family_composition();
    if (rc == 0) puts("overlay unit 0x38 effect-entry tests passed");
    return rc;
}
