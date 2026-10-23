/* U23 +0239 inventory append: contract of the C composition on synthetic memory (no original bytes).
 * The callees that read only data (275B, GetMem, Move, StrLoad, 0BB8) run for real; 1693 reads its own call sites from code
 * bytes that this test does not hold, so the display path is checked as a callee refusal carried through (CALLEE/DISPLAY).
 * The differential against the original bytes is tools/overlay/u23_append_0239_cpu_audit.py. */
#include "poolrad/recovered/overlay_u23_append_0239_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define OVL 0x3000u
#define STUB 0x3200u
#define U77 0x3400u
#define SYS 0x6000u
#define RES 0x00BAu
#define DSEG 0x5000u
#define SSEG 0x2000u
#define CHAR_SEG 0x1000u
#define CHAR_OFF 0x0100u
#define NODE_SEG 0x1400u
#define ITEM_SEG 0x1C00u
#define ITEM_OFF 0x0200u
#define RESULT_SEG 0x1D00u
#define HEAP_SEG 0x9000u
#define SP0 0x8000u

static size_t at(unsigned s, unsigned o) { return (((size_t)s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { size_t p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }
static unsigned rb(const uint8_t *r, unsigned s, unsigned o) { return r[at(s, o)]; }

static const struct { unsigned site, off, seg; } SITES[] = {
    {0x24D, 0x4D, STUB}, {0x260, 0xB0B, SYS}, {0x265, 0x1693, 0x8200}, {0x29B, 0x23F, SYS}, {0x2B4, 0xA3F, SYS},
    {0x300, 0x23F, SYS}, {0x319, 0xA3F, SYS}, {0x344, 0xBB8, RES}};

static pr_ega ega;
static pr_bus bus;
static pr_u23_0239_limits limits = {65536, 65536, 65536, 65536, 1000000, 1u << 20, 64, 1000};

/* nodes: number of existing list records; each has weight 1 and count 1 (so S = nodes and the weight test is far from the edge). */
static pr_u23_0239_cpu setup(uint8_t *r, unsigned nodes, unsigned item_weight, unsigned item_count)
{
    pr_u23_0239_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, SP0, OVL, 0x0239, DSEG, 0x8888, SSEG, 0x0202};
    unsigned i;
    memset(r, 0xA5, PR_BUS_RAM_SIZE);
    for (i = 0; i < sizeof SITES / sizeof SITES[0]; ++i) {
        r[at(OVL, SITES[i].site)] = 0x9A;
        wr(r, OVL, SITES[i].site + 1, SITES[i].off);
        wr(r, OVL, SITES[i].site + 3, SITES[i].seg);
    }
    r[at(STUB, 0x4D)] = 0xEA; wr(r, STUB, 0x4E, 0x275B); wr(r, STUB, 0x50, U77);
    wr(r, U77, 0x276A, RES); wr(r, U77, 0x27AB, RES);          /* 275B's own relocated segment words */
    r[at(OVL, 0x22E)] = 10;
    for (i = 0; i < 10; ++i) r[at(OVL, 0x22F + i)] = (uint8_t)('a' + i);   /* synthetic literal, not game text */
    for (i = 0; i < 0x140; ++i) r[at(CHAR_SEG, CHAR_OFF + i)] = 0;
    wr(r, DSEG, 0x5D92, CHAR_OFF); wr(r, DSEG, 0x5D94, CHAR_SEG);
    for (i = 0; i < nodes; ++i) {
        unsigned off = 0x2000 + i * 0x100, j;
        for (j = 0; j < 0x50; ++j) r[at(NODE_SEG, off + j)] = 0;
        wr(r, NODE_SEG, off + 0x37, 1); r[at(NODE_SEG, off + 0x39)] = 1;
        wr(r, NODE_SEG, off + 0x2A, i + 1 < nodes ? off + 0x100 : 0); wr(r, NODE_SEG, off + 0x2C, i + 1 < nodes ? NODE_SEG : 0);
    }
    wr(r, CHAR_SEG, CHAR_OFF + 0xC8, nodes ? 0x2000 : 0); wr(r, CHAR_SEG, CHAR_OFF + 0xCA, nodes ? NODE_SEG : 0);
    for (i = 0; i < 0x3F; ++i) r[at(ITEM_SEG, ITEM_OFF + i)] = (uint8_t)(0x40 + i * 7);
    wr(r, ITEM_SEG, ITEM_OFF + 0x37, item_weight); r[at(ITEM_SEG, ITEM_OFF + 0x39)] = (uint8_t)item_count;
    /* heap: HeapPtr at the top of an empty free list, default HeapError, plenty of room */
    wr(r, DSEG, 0x4400, 0); wr(r, DSEG, 0x4402, HEAP_SEG); wr(r, DSEG, 0x4404, 0); wr(r, DSEG, 0x4406, HEAP_SEG + 0x300);
    wr(r, DSEG, 0x4408, 0); wr(r, DSEG, 0x440A, HEAP_SEG + 0xFF0); wr(r, DSEG, 0x4414, 0xA9); wr(r, DSEG, 0x4416, SYS);
    wr(r, DSEG, 0x440C, 0); wr(r, DSEG, 0x440E, HEAP_SEG + 0x300);
    wr(r, HEAP_SEG + 0x300, 0, 0); wr(r, HEAP_SEG + 0x300, 2, 0); wr(r, HEAP_SEG + 0x300, 4, 0); wr(r, HEAP_SEG + 0x300, 6, 0);
    wr(r, SSEG, SP0, 0x7000); wr(r, SSEG, SP0 + 2, 0x9001); wr(r, SSEG, SP0 + 4, 0x0040); wr(r, SSEG, SP0 + 6, RESULT_SEG);
    wr(r, SSEG, SP0 + 8, ITEM_OFF); wr(r, SSEG, SP0 + 10, ITEM_SEG);
    r[at(RESULT_SEG, 0x40)] = 0x5A;
    return c;
}

static pr_u23_0239_status run(uint8_t *r, pr_u23_0239_cpu *c, pr_u23_0239_diagnostic *d)
{
    bus.ram = r; bus.ega = &ega; bus.ports = NULL; bus.on_vram_write = NULL; bus.ctx = NULL;
    return pr_u23_append_0239_cpu(&bus, PR_BUS_RAM_SIZE, c, &limits, d);
}

static unsigned chain_length(const uint8_t *r, unsigned *tail_seg, unsigned *tail_off)
{
    unsigned off = rd(r, CHAR_SEG, CHAR_OFF + 0xC8), seg = rd(r, CHAR_SEG, CHAR_OFF + 0xCA), n = 0;
    while ((off | seg) != 0 && n < 64) {
        *tail_off = off; *tail_seg = seg; ++n;
        { unsigned o2 = rd(r, seg, off + 0x2A), s2 = rd(r, seg, off + 0x2C); off = o2; seg = s2; }
    }
    return n;
}

static void appended(const uint8_t *r, unsigned before, const pr_u23_0239_cpu *c, const pr_u23_0239_diagnostic *d)
{
    unsigned seg = 0, off = 0, n = chain_length(r, &seg, &off), k;
    CHECK(n == before + 1);
    CHECK(rb(r, RESULT_SEG, 0x40) == 0);                                       /* *RESULT := 0 */
    CHECK(seg >= HEAP_SEG && seg < HEAP_SEG + 0x1000);                         /* a fresh heap block */
    for (k = 0; k < 0x3F; ++k) {
        unsigned want = (k >= 0x2A && k < 0x2E) ? 0 : (0x40 + k * 7) & 255;
        if (k == 0x37) want = 1;
        if (k == 0x38) want = 0;                                              /* the weight word is 0001h */
        if (k == 0x39) want = 1;
        CHECK(rb(r, seg, off + k) == want);                                    /* ITEM's 63 bytes, links cleared */
    }
    CHECK(c->cs == 0x9001 && c->ip == 0x7000 && c->sp == SP0 + 12 && c->bp == 0x7777 && c->ds == DSEG && c->ss == SSEG);
    CHECK(d->callee == PR_U23_0239_REDRAW && d->status == 0);                  /* the last call was the character update */
}

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *saved = malloc(PR_BUS_RAM_SIZE);
    pr_u23_0239_cpu c, before;
    pr_u23_0239_diagnostic d;
    unsigned seg, off;
    CHECK(r && saved);
    memset(&ega, 0, sizeof ega); pr_ega_reset(&ega);

    /* empty list: GetMem, Move, head := node, then 0BB8 */
    c = setup(r, 0, 1, 1);
    CHECK(run(r, &c, &d) == PR_U23_0239_OK);
    appended(r, 0, &c, &d);
    CHECK(d.walked == 0);
    /* a list of one, three and fifteen records: the tail is found and linked; fifteen is not yet over capacity */
    for (unsigned n = 1; n <= 15; n += (n == 1 ? 2 : 12)) {
        c = setup(r, n, 1, 1);
        CHECK(run(r, &c, &d) == PR_U23_0239_OK);
        appended(r, n, &c, &d);
        CHECK(d.walked == n - 1);                                              /* N records need N-1 backward jumps */
        CHECK((rd(r, NODE_SEG, 0x2000 + (n - 1) * 0x100 + 0x2A) | rd(r, NODE_SEG, 0x2000 + (n - 1) * 0x100 + 0x2C)) != 0);          /* the old tail now points at the new record */
    }

    /* the walk cap: three records need two backward jumps; a cap of one stops at 02FA before the second */
    c = setup(r, 3, 1, 1); limits.walk_limit = 1;
    CHECK(run(r, &c, &d) == PR_U23_0239_LIMIT && c.ip == 0x02FA && d.walked == 1);
    limits.walk_limit = 2;
    c = setup(r, 3, 1, 1);
    CHECK(run(r, &c, &d) == PR_U23_0239_OK && d.walked == 2);
    limits.walk_limit = 1000;

    /* over capacity by count: sixteen records. The message literal is copied to the local, then 1693 is called; this test
     * holds no 1693 bytes, so the display refuses and the stop is carried as CALLEE/DISPLAY with RESULT and the list untouched. */
    c = setup(r, 16, 1, 1);
    CHECK(run(r, &c, &d) == PR_U23_0239_CALLEE && d.callee == PR_U23_0239_DISPLAY && d.status != 0);
    CHECK(rb(r, RESULT_SEG, 0x40) == 0x5A);
    CHECK(rb(r, SSEG, (SP0 - 2 - 0x13) & 65535) == 10 && rb(r, SSEG, (SP0 - 2 - 0x13 + 1) & 65535) == 'a');
    CHECK(chain_length(r, &seg, &off) == 16);

    /* a callee stop stops the body there, whatever the callee. A cyclic list never lets 0BB8 (inside 275B) finish: 275B resets its own
     * instruction budget, so the stop is 275B's CALLEE, carried as CALLEE/CHECK. The 0BB8 call at 0344 takes this body's budget. */
    c = setup(r, 3, 1, 1);
    wr(r, NODE_SEG, 0x2000 + 2 * 0x100 + 0x2A, 0x2000); wr(r, NODE_SEG, 0x2000 + 2 * 0x100 + 0x2C, NODE_SEG);
    CHECK(run(r, &c, &d) == PR_U23_0239_CALLEE && d.callee == PR_U23_0239_CHECK && d.status != 0);
    c = setup(r, 2, 1, 1); limits.resident_budget = 1;
    CHECK(run(r, &c, &d) == PR_U23_0239_CALLEE && d.callee == PR_U23_0239_REDRAW && d.status == PR_RES00BA_LIMIT);
    limits.resident_budget = 1u << 20;
    c = setup(r, 0, 1, 1); wr(r, DSEG, 0x4408, 0); wr(r, DSEG, 0x440A, HEAP_SEG + 0x300);   /* HeapEnd = HeapPtr: no room */
    CHECK(run(r, &c, &d) == PR_U23_0239_CALLEE && d.callee == PR_U23_0239_GETMEM_HEAD && d.status == PR_SYSTEM_GETMEM_RUNERROR);
    CHECK(c.cs == SYS && c.ip == 0x00E2);                                                 /* stopped at RunError's entry */
    c = setup(r, 2, 1, 1); wr(r, DSEG, 0x4408, 0); wr(r, DSEG, 0x440A, HEAP_SEG + 0x300);
    CHECK(run(r, &c, &d) == PR_U23_0239_CALLEE && d.callee == PR_U23_0239_GETMEM_TAIL);

    /* binding guards: a changed opcode or offset at a call site stops there (CODE), and an unresolved stub (RESOLVE) */
    for (unsigned k = 0; k < sizeof SITES / sizeof SITES[0]; ++k) {
        unsigned n = (SITES[k].site == 0x260 || SITES[k].site == 0x265) ? 16 : (SITES[k].site == 0x29B || SITES[k].site == 0x2B4) ? 0 : 2;   /* a list that reaches the site */
        c = setup(r, n, 1, 1); r[at(OVL, SITES[k].site)] = 0x90;
        CHECK(run(r, &c, &d) == PR_U23_0239_CODE && c.ip == SITES[k].site);
        c = setup(r, n, 1, 1); r[at(OVL, SITES[k].site + 1)] ^= 0xFF;
        CHECK(run(r, &c, &d) == PR_U23_0239_CODE && c.ip == SITES[k].site);
    }
    c = setup(r, 2, 1, 1); r[at(STUB, 0x4D)] = 0x90;
    CHECK(run(r, &c, &d) == PR_U23_0239_RESOLVE && c.ip == 0x024D);
    c = setup(r, 2, 1, 1); wr(r, STUB, 0x4E, 0x275C);
    CHECK(run(r, &c, &d) == PR_U23_0239_RESOLVE);

    /* ARGUMENT: wrong IP, SP outside [0200,FFE0], an aperture segment, null and short objects; nothing changes */
    for (unsigned k = 0; k < 9; ++k) {
        c = setup(r, 2, 1, 1);
        if (k == 0) c.ip = 0x023A;
        if (k == 1) c.sp = 0x01FF;
        if (k == 2) c.sp = 0xFFE1;
        if (k == 3) c.cs = 0xA000;
        if (k == 4) c.ds = 0xA800;
        if (k == 5) c.ss = 0xAF00;
        before = c; memcpy(saved, r, PR_BUS_RAM_SIZE);
        bus.ram = r; bus.ega = &ega; bus.ports = NULL; bus.on_vram_write = NULL; bus.ctx = NULL;
        CHECK(pr_u23_append_0239_cpu(k == 6 ? NULL : &bus, k == 7 ? 1 : PR_BUS_RAM_SIZE, k == 8 ? NULL : &c, &limits, &d) == PR_U23_0239_ARGUMENT);
        if (k != 8) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, PR_BUS_RAM_SIZE) == 0);
        CHECK(d.callee == -1);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
