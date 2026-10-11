/* Synthetic tests for unit 0x38 entries 81 and 110. No DOS bytes: records,
 * the object behind slot 0x108, the record behind it, and list nodes are
 * fabricated in two fixture segments. Expected values are hand-derived. */
#include "poolrad/recovered/overlay_unit38_walk.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint8_t seg_a[0x400];      /* segment 0x2000: P (at 0x20) and Q (at 0x100) */
    uint8_t seg_b[0x400];      /* segment 0x3000: list nodes, 0x40 bytes apart */
    unsigned calls;
    pr_u38_far_ptr trace[16];
    size_t need[16];
} fixture;

static uint8_t *resolve(void *context, pr_u38_far_ptr p, size_t need)
{
    fixture *f = (fixture *)context;
    uint8_t *base = NULL;
    if (f->calls < 16) { f->trace[f->calls] = p; f->need[f->calls] = need; }
    f->calls++;
    if (p.segment == 0x2000u) base = f->seg_a;
    else if (p.segment == 0x3000u) base = f->seg_b;
    if (!base || (size_t)p.offset + need > 0x400u) return NULL;
    return base + p.offset;
}

static void put_far(uint8_t *at, uint16_t offset, uint16_t segment)
{
    at[0] = (uint8_t)offset; at[1] = (uint8_t)(offset >> 8);
    at[2] = (uint8_t)segment; at[3] = (uint8_t)(segment >> 8);
}

static pr_u38w_state seeded(unsigned seed)
{
    pr_u38w_state s;
    s.b6816 = (uint8_t)(seed * 7u + 1u); s.b681e = 0; s.b682a = (uint8_t)(seed * 37u + 9u);
    s.scratch_6826.offset = (uint16_t)(seed * 0x111u);
    s.scratch_6826.segment = (uint16_t)(seed * 0x222u + 1u);
    return s;
}

static int same_except(const pr_u38w_state *a, const pr_u38w_state *b, const char *except)
{
#define FIELD(name) if (!strstr(except, #name) && a->name != b->name) return 0;
    FIELD(b6816) FIELD(b681e) FIELD(b682a)
#undef FIELD
    if (!strstr(except, "scratch") &&
        (a->scratch_6826.offset != b->scratch_6826.offset ||
         a->scratch_6826.segment != b->scratch_6826.segment)) return 0;
    return 1;
}

/* ---- entry 81 ---------------------------------------------------------- */

static unsigned bracket_for(unsigned v)
{
    if (v >= 4u && v <= 6u) return 1u;
    if (v >= 7u && v <= 10u) return 2u;
    if (v >= 11u && v <= 13u) return 3u;
    if (v >= 14u && v <= 17u) return 4u;
    if (v >= 18u && v <= 20u) return 5u;
    return 0u;     /* undefined in the original */
}

static int test_entry81(void)
{
    uint8_t record[PR_U38_RECORD_SIZE], copy[PR_U38_RECORD_SIZE];
    unsigned gate, v, residue;
    /* gate: only 2 and 4 open it */
    for (gate = 0; gate < 256u; ++gate) {
        pr_u38w_state s = seeded(gate), before;
        unsigned paths = 99u, br = 99u;
        s.b682a = (uint8_t)gate; before = s;
        memset(record, 0, sizeof record); record[0x14] = 8;     /* bracket 2 if open */
        CHECK(pr_u38_entry81(&s, record, 0x77, &paths, &br) == PR_U38_OK);
        if (gate == 2u || gate == 4u) {
            CHECK(paths == PR_U38_PATH_EFFECT && br == 2 && s.b6816 == (uint8_t)(before.b6816 + 2u));
            CHECK(same_except(&s, &before, "b6816"));
        } else {
            CHECK(paths == 0 && br == 0 && same_except(&s, &before, ""));
        }
    }
    /* every record value with three residues, under each open gate value */
    for (gate = 2; gate <= 4u; gate += 2u) {
        for (v = 0; v < 256u; ++v) {
            for (residue = 0; residue < 256u; residue += 127u) {
                pr_u38w_state s = seeded(v + residue), before;
                unsigned paths = 99u, br = 99u, expect = bracket_for(v);
                s.b682a = (uint8_t)gate; before = s;
                memset(record, (int)(v ^ 0xC3u), sizeof record);
                record[0x14] = (uint8_t)v;
                memcpy(copy, record, sizeof record);
                CHECK(pr_u38_entry81(&s, record, (uint8_t)residue, &paths, &br) == PR_U38_OK);
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
    }
    /* boundaries and wraparound of the byte add */
    {
        static const struct { unsigned v, br; } edge[] = {
            {3, 0}, {4, 1}, {6, 1}, {7, 2}, {10, 2}, {11, 3}, {13, 3},
            {14, 4}, {17, 4}, {18, 5}, {20, 5}, {21, 0}, {255, 0}};
        size_t i;
        for (i = 0; i < sizeof edge / sizeof edge[0]; ++i) {
            pr_u38w_state s = seeded(1); unsigned br;
            s.b682a = 4; s.b6816 = 0xFE;
            memset(record, 0, sizeof record); record[0x14] = (uint8_t)edge[i].v;
            CHECK(pr_u38_entry81(&s, record, 9, NULL, &br) == PR_U38_OK);
            CHECK(br == (edge[i].br ? edge[i].br : 9u));
            CHECK(s.b6816 == (uint8_t)(0xFEu + br));
        }
    }
    { pr_u38w_state s = seeded(2); CHECK(pr_u38_entry81(&s, NULL, 0, NULL, NULL) == PR_U38_ARGUMENT); }
    CHECK(pr_u38_entry81(NULL, record, 0, NULL, NULL) == PR_U38_ARGUMENT);
    return 0;
}

/* ---- entry 110 --------------------------------------------------------- */

/* Lay out P at 0x2000:0x20, Q at 0x2000:0x100, head pointer in Q[0xC8]. */
static void setup_chain(fixture *f, uint8_t *record, uint16_t head_offset, uint16_t head_segment)
{
    memset(f, 0, sizeof *f);
    memset(record, 0x5A, PR_U38_RECORD_SIZE);
    put_far(record + 0x108, 0x20, 0x2000);
    put_far(f->seg_a + 0x20 + 0x0A, 0x100, 0x3000 - 0x1000);   /* Q at 0x2000:0x100 */
    put_far(f->seg_a + 0x100 + 0xC8, head_offset, head_segment);
}

/* node k lives at 0x3000:(0x40 * (k + 1)); returns its base in seg_b */
static uint8_t *node_at(fixture *f, unsigned k)
{
    return f->seg_b + 0x40u * (k + 1u);
}

static void link_node(fixture *f, unsigned k, unsigned next_k_or_null)
{
    if (next_k_or_null == 0xFFFFu) put_far(node_at(f, k) + 0x2A, 0, 0);
    else put_far(node_at(f, k) + 0x2A, (uint16_t)(0x40u * (next_k_or_null + 1u)), 0x3000);
}

static int test_entry110_exits(void)
{
    fixture f;
    uint8_t record[PR_U38_RECORD_SIZE];
    pr_u38w_state s, before;
    unsigned nodes = 99u, matched = 99u;

    /* null head: END, no node read, scratch written */
    setup_chain(&f, record, 0, 0);
    s = seeded(1); before = s;
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END);
    CHECK(nodes == 0 && matched == 0 && f.calls == 2);
    CHECK(f.need[0] == 0x0E && f.need[1] == 0xCC);
    CHECK(s.scratch_6826.offset == 0x100 && s.scratch_6826.segment == 0x2000);
    CHECK(same_except(&s, &before, "scratch"));

    /* null head with the flag preset: the null test comes first, so END */
    s = seeded(1); s.b681e = 1;
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END);
    CHECK(nodes == 0 && s.b681e == 1);

    /* non-null head with the flag preset: FLAG before any node is read */
    setup_chain(&f, record, 0x40, 0x3000);
    link_node(&f, 0, 0xFFFFu);
    s = seeded(2); s.b681e = 0x55;
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_FLAG);
    CHECK(nodes == 0 && matched == 0 && f.calls == 2 && s.b681e == 0x55);

    /* one node, [0x34] == 0: inner loop skipped, link followed, END */
    setup_chain(&f, record, 0x40, 0x3000);
    link_node(&f, 0, 0xFFFFu);
    node_at(&f, 0)[0x2F] = 0x76;                 /* would match if examined */
    s = seeded(3);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END);
    CHECK(nodes == 1 && matched == 0 && s.b681e == 0);
    CHECK(f.calls == 3 && f.need[2] == 0x35 && f.trace[2].offset == 0x40 && f.trace[2].segment == 0x3000);

    /* P unresolvable: ARGUMENT, scratch untouched; Q unresolvable: scratch written */
    setup_chain(&f, record, 0x40, 0x3000);
    put_far(record + 0x108, 0x20, 0x7000);
    s = seeded(4); before = s;
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_ARGUMENT);
    CHECK(f.calls == 1 && same_except(&s, &before, ""));
    put_far(record + 0x108, 0x20, 0x2000);
    put_far(f.seg_a + 0x20 + 0x0A, 0x3F0, 0x2000);   /* 0xCC bytes do not fit */
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_ARGUMENT);
    CHECK(s.scratch_6826.offset == 0x3F0 && same_except(&s, &before, "scratch"));

    /* unresolvable node pointer: ARGUMENT after the head was found non-null */
    setup_chain(&f, record, 0x10, 0x9000);
    s = seeded(5);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_ARGUMENT);
    CHECK(nodes == 0 && f.calls == 3);

    CHECK(pr_u38_entry110(NULL, record, resolve, &f, 8, NULL, NULL) == PR_U38_WALK_ARGUMENT);
    CHECK(pr_u38_entry110(&s, NULL, resolve, &f, 8, NULL, NULL) == PR_U38_WALK_ARGUMENT);
    CHECK(pr_u38_entry110(&s, record, NULL, &f, 8, NULL, NULL) == PR_U38_WALK_ARGUMENT);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 0, NULL, NULL) == PR_U38_WALK_ARGUMENT);
    return 0;
}

static int test_entry110_matching(void)
{
    fixture f;
    uint8_t record[PR_U38_RECORD_SIZE];
    pr_u38w_state s;
    unsigned nodes, matched, i, probe, b31;

    /* single enabled node, exhaustive probe byte at each of the three
     * examined offsets, with [0x31] both 0xFC and not: match iff 0x76, or
     * 0x98 with [0x31] == 0xFC */
    for (i = 1; i <= 3u; ++i) {
        for (b31 = 0; b31 < 256u; b31 += 85u) {           /* 0, 85, 170, 255 (never 0xFC) */
            for (probe = 0; probe < 256u; ++probe) {
                unsigned expect;
                setup_chain(&f, record, 0x40, 0x3000);
                link_node(&f, 0, 0xFFFFu);
                node_at(&f, 0)[0x34] = 1;
                memset(node_at(&f, 0) + 0x2F, 0x11, 3);     /* 0x2F..0x31 */
                node_at(&f, 0)[0x31] = (uint8_t)b31;
                node_at(&f, 0)[0x2E + i] = (uint8_t)probe;  /* i == 3 overwrites 0x31 */
                s = seeded(probe);
                CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END);
                expect = (probe == 0x76u) ||
                         (probe == 0x98u && node_at(&f, 0)[0x31] == 0xFCu);
                CHECK(matched == expect && s.b681e == (expect ? 1u : 0u));
                CHECK(nodes == 1);
            }
        }
    }
    /* 0x98 at index 1 with [0x31] == 0xFC matches; the same without FC does not */
    setup_chain(&f, record, 0x40, 0x3000); link_node(&f, 0, 0xFFFFu);
    node_at(&f, 0)[0x34] = 1; node_at(&f, 0)[0x2F] = 0x98; node_at(&f, 0)[0x31] = 0xFC;
    s = seeded(7);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END && matched == 1);
    node_at(&f, 0)[0x31] = 0xFB; s = seeded(7);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END && matched == 0);
    /* index 3 is [0x31] itself: 0x98 there cannot pair with 0xFC, so no match; 0x76 there matches */
    node_at(&f, 0)[0x2F] = 0; node_at(&f, 0)[0x31] = 0x98; s = seeded(7);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END && matched == 0);
    node_at(&f, 0)[0x31] = 0x76; s = seeded(7);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END && matched == 1);
    /* byte 0x2E (index 0) and 0x32 (index 4) are never examined */
    memset(node_at(&f, 0) + 0x2E, 0, 5); node_at(&f, 0)[0x2E] = 0x76; node_at(&f, 0)[0x32] = 0x76; s = seeded(7);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END && matched == 0);
    return 0;
}

static int test_entry110_chains(void)
{
    fixture f;
    uint8_t record[PR_U38_RECORD_SIZE];
    pr_u38w_state s;
    unsigned nodes, matched;

    /* three nodes, match on the second: the inner loop finishes the second
     * node, the link is followed, and the flag test exits before the third
     * node is read (two node reads, four resolver calls) */
    setup_chain(&f, record, 0x40, 0x3000);
    link_node(&f, 0, 1); link_node(&f, 1, 2); link_node(&f, 2, 0xFFFFu);
    node_at(&f, 0)[0x34] = 1; node_at(&f, 1)[0x34] = 1; node_at(&f, 2)[0x34] = 1;
    node_at(&f, 1)[0x30] = 0x76; node_at(&f, 2)[0x2F] = 0x76;
    s = seeded(8);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_FLAG);
    CHECK(nodes == 2 && matched == 1 && s.b681e == 1 && f.calls == 4);
    CHECK(f.trace[3].offset == 0x80 && f.trace[3].segment == 0x3000);

    /* three nodes, no match anywhere, END after all three; disabled middle node skipped */
    setup_chain(&f, record, 0x40, 0x3000);
    link_node(&f, 0, 1); link_node(&f, 1, 2); link_node(&f, 2, 0xFFFFu);
    node_at(&f, 0)[0x34] = 1; node_at(&f, 1)[0x34] = 0; node_at(&f, 2)[0x34] = 1;
    node_at(&f, 1)[0x2F] = 0x76;                     /* disabled: not examined */
    s = seeded(9);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 8, &nodes, &matched) == PR_U38_WALK_END);
    CHECK(nodes == 3 && matched == 0 && s.b681e == 0 && f.calls == 5);

    /* a two-node cycle with no match: the original never terminates; the
     * translation stops at node_limit and says so */
    setup_chain(&f, record, 0x40, 0x3000);
    link_node(&f, 0, 1); link_node(&f, 1, 0);
    node_at(&f, 0)[0x34] = 1; node_at(&f, 1)[0x34] = 1;
    s = seeded(10);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 5, &nodes, &matched) == PR_U38_WALK_LIMIT);
    CHECK(nodes == 5 && matched == 0 && s.b681e == 0);
    /* the same cycle with a match on the second node exits by the flag */
    node_at(&f, 1)[0x31] = 0x76; s = seeded(10);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 5, &nodes, &matched) == PR_U38_WALK_FLAG);
    CHECK(nodes == 2 && matched == 1);
    /* a limit of exactly the chain length still reaches END on an acyclic list */
    setup_chain(&f, record, 0x40, 0x3000);
    link_node(&f, 0, 1); link_node(&f, 1, 0xFFFFu);
    s = seeded(11);
    CHECK(pr_u38_entry110(&s, record, resolve, &f, 2, &nodes, &matched) == PR_U38_WALK_END && nodes == 2);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_entry81();
    rc |= test_entry110_exits();
    rc |= test_entry110_matching();
    rc |= test_entry110_chains();
    if (rc == 0) puts("overlay unit 0x38 walk-entry tests passed");
    return rc;
}
