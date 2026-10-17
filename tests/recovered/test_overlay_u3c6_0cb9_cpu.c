/* U3C6 +0CB9 CPU/RAM model: frame, write-set and alias controls on synthetic
 * state (no game bytes). The reference below is an independent high-level
 * reading of the four bodies, valid only when P6/P10 alias nothing. */
#include "poolrad/recovered/overlay_u3c6_0cb9_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSEG 0x2000u
#define DSEG 0x1000u
#define SSEG 0x3000u
#define OSEG 0x4000u
#define MSEG 0x5000u
#define RET_CS 0x0BADu
#define RET_IP 0x1234u
#define SP0 0x8000u    /* SP before the caller's argument pushes */
#define ENTRY_SP 0x7FEEu /* SP0 - 14 argument bytes - 4 return bytes */
#define FRAME_BP 0x7FECu

static uint8_t ram[PR_U3C6_0CB9_CPU_RAM_SIZE], before[PR_U3C6_0CB9_CPU_RAM_SIZE];
static unsigned failures, checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (sizeof ram - 1u); }
static uint8_t r8(uint16_t seg, uint16_t off) { return ram[at(seg, off)]; }
static uint16_t r16(uint16_t seg, uint16_t off)
{
    return (uint16_t)(r8(seg, off) | ((uint16_t)ram[(at(seg, off) + 1u) & (sizeof ram - 1u)] << 8));
}
static void w8(uint16_t seg, uint16_t off, uint8_t v) { ram[at(seg, off)] = v; }
static void w16(uint16_t seg, uint16_t off, uint16_t v)
{
    ram[at(seg, off)] = (uint8_t)v;
    ram[(at(seg, off) + 1u) & (sizeof ram - 1u)] = (uint8_t)(v >> 8);
}
static void check(int ok, const char *what, unsigned line, unsigned tag)
{
    ++checks;
    if (!ok) {
        fprintf(stderr, "line %u (case %u): %s\n", line, tag, what);
        ++failures;
    }
}
#define CHECK(tag, e) check((e) != 0, #e, __LINE__, (tag))

typedef struct {
    uint16_t p6_seg, p6_off, p10_seg, p10_off, q_seg, q_off;
    uint8_t k;
} args_t;

static void enter(pr_u3c6_0cb9_cpu *c, const args_t *a)
{
    const uint16_t words[9] = {a->q_seg, a->q_off, a->k, a->p10_seg, a->p10_off,
                               a->p6_seg, a->p6_off, RET_CS, RET_IP};
    uint16_t sp = SP0;
    unsigned i;
    for (i = 0; i < 9; i++) {
        sp = (uint16_t)(sp - 2u);
        w16(SSEG, sp, words[i]);
    }
    memset(c, 0, sizeof *c);
    c->ax = 0x1111u; c->bx = 0x2222u; c->cx = 0x3333u; c->dx = 0x4444u;
    c->si = 0x5555u; c->di = 0x6666u; c->bp = 0x7777u; c->sp = sp;
    c->cs = CSEG; c->ip = 0x0CB9u; c->ds = DSEG; c->es = 0x9999u; c->ss = SSEG;
    c->flags = 0x0002u;
}

/* entry 15 */
static uint8_t ref_slot(const args_t *a)
{
    uint8_t s = 0;
    for (;;) {
        s = (uint8_t)(s + 1u);
        if (r16(DSEG, (uint16_t)(0x65BBu + 4u * s)) == a->q_seg &&
            r16(DSEG, (uint16_t)(0x65B9u + 4u * s)) == a->q_off)
            return s;
        if (s == r8(DSEG, 0x5F2Au)) return 0;
    }
}
static void reference(const args_t *a, uint8_t *e6, uint8_t *e10, uint8_t *ebest, uint8_t *eslot)
{
    uint8_t s = ref_slot(a), x0, y0, cls, best = 1, v6 = 0x17, v10 = 0;
    unsigned i;
    x0 = r8(DSEG, (uint16_t)(0x5F27u + 4u * s));
    y0 = r8(DSEG, (uint16_t)(0x5F28u + 4u * s));
    cls = r8(DSEG, (uint16_t)(0x5F2Au + 4u * s));
    for (i = 0; i < 4; i++) {
        int8_t dx, dy, col, row;
        uint8_t cell_a = 0, cell_b = 0;
        if (cls == 0) continue;                                           /* entry 1 returns 0 */
        dx = (int8_t)r8(DSEG, (uint16_t)(0x2844u + 8u * cls + 2u * i));
        dy = (int8_t)r8(DSEG, (uint16_t)(0x2845u + 8u * cls + 2u * i));
        if (dx < 0) continue;                                             /* entry 1 returns 0 */
        col = (int8_t)(uint8_t)((int8_t)r8(DSEG, (uint16_t)(0x2736u + a->k)) + dx + (int8_t)x0);
        row = (int8_t)(uint8_t)((int8_t)r8(DSEG, (uint16_t)(0x273Fu + a->k)) + dy + (int8_t)y0);
        if (col >= 0 && col <= 0x31 && row >= 0 && row <= 0x18) {        /* entry 6 */
            uint16_t cell = (uint16_t)(row * 50 + col);
            cell_a = r8(r16(DSEG, 0x6718u), (uint16_t)(r16(DSEG, 0x6716u) + cell + 7u));
            cell_b = r8(DSEG, (uint16_t)(0x60DBu + cell));
        }
        if (cell_b == s) cell_b = 0;
        if (cell_b != 0) v10 = cell_b;
        if (cell_a == 0 || v6 == 0) v6 = 0;
        else if (cell_a == 0x1E || v6 == 0x1E) v6 = 0x1E;
        else if (r8(DSEG, (uint16_t)(0x2744u + 4u * cell_a)) >= best) {
            best = r8(DSEG, (uint16_t)(0x2744u + 4u * cell_a));
            v6 = cell_a;
        }
    }
    *e6 = v6; *e10 = v10; *ebest = best; *eslot = s;
}

static uint32_t lcg;
static uint8_t rnd8(void) { lcg = lcg * 1103515245u + 12345u; return (uint8_t)(lcg >> 16); }
static uint8_t pick(const uint8_t *v, unsigned n) { return v[rnd8() % n]; }

static void tables(unsigned seed, args_t *a)
{
    static const uint8_t dxs[] = {0x00, 0x01, 0x02, 0xFF, 0x80, 0x7F};
    static const uint8_t ks[] = {0x00, 0x01, 0xFF, 0x20, 0x10};
    static const uint8_t cells[] = {0x00, 0x1E, 0x02, 0x05, 0x07, 0x80};
    unsigned j;
    uint8_t s = (uint8_t)(seed % 6u), count = (uint8_t)((seed / 6u) % 5u * 2u);
    memset(ram, 0, sizeof ram);
    lcg = seed * 2654435761u + 1u;
    for (j = 0; j < 0x10000u; j++) w8(DSEG, (uint16_t)j, rnd8());
    for (j = 0; j < 50u * 25u; j++) {
        w8(MSEG, (uint16_t)(7u + j), pick(cells, sizeof cells));
        w8(DSEG, (uint16_t)(0x60DBu + j), (uint8_t)((rnd8() & 3u) == 0 ? s : pick(cells, sizeof cells)));
    }
    for (j = 0; j < 256u; j++) {
        w16(DSEG, (uint16_t)(0x65B9u + 4u * j), (uint16_t)(0x1100u + j));
        w16(DSEG, (uint16_t)(0x65BBu + 4u * j), 0x2222u);
        w8(DSEG, (uint16_t)(0x2744u + 4u * j), (uint8_t)(rnd8() % 5u));
    }
    for (j = 0; j < 64u; j++) {
        w8(DSEG, (uint16_t)(0x5F27u + 4u * j), (uint8_t)(rnd8() % 0x32u));
        w8(DSEG, (uint16_t)(0x5F28u + 4u * j), (uint8_t)(rnd8() % 0x19u));
        w8(DSEG, (uint16_t)(0x5F2Au + 4u * j), (uint8_t)(rnd8() % 4u));
    }
    for (j = 0; j < 0x40u; j++) w8(DSEG, (uint16_t)(0x2844u + j), pick(dxs, sizeof dxs));
    for (j = 0; j < 16u; j++) {
        w8(DSEG, (uint16_t)(0x2736u + j), pick(ks, sizeof ks));
        w8(DSEG, (uint16_t)(0x273Fu + j), pick(ks, sizeof ks));
    }
    a->q_seg = 0x5555u; a->q_off = 0x1234u;
    if (s != 0) {
        w16(DSEG, (uint16_t)(0x65B9u + 4u * s), a->q_off);
        w16(DSEG, (uint16_t)(0x65BBu + 4u * s), a->q_seg);
    }
    w8(DSEG, 0x5F2Au, count);
    w16(DSEG, 0x6716u, 0x0000u);
    w16(DSEG, 0x6718u, MSEG);
    a->p6_seg = OSEG; a->p6_off = 0x0010u;
    a->p10_seg = OSEG; a->p10_off = 0x0020u;
    a->k = (uint8_t)(rnd8() % 9u);
    w8(OSEG, 0x0010u, 0x55u);
    w8(OSEG, 0x0020u, 0x55u);
}

/* every byte that changed must lie in A's frame/callee scratch or be *P6 / *P10 */
static int write_set_ok(void)
{
    size_t i, lo = at(SSEG, 0x7FC0u), hi = at(SSEG, ENTRY_SP);
    for (i = 0; i < sizeof ram; i++) {
        if (ram[i] == before[i]) continue;
        if (i >= lo && i < hi) continue;
        if (i == at(OSEG, 0x0010u) || i == at(OSEG, 0x0020u)) continue;
        return 0;
    }
    return 1;
}

static void reference_runs(void)
{
    unsigned seed;
    for (seed = 1; seed <= 240u; seed++) {
        pr_u3c6_0cb9_cpu c;
        args_t a;
        uint8_t e6, e10, ebest, eslot;
        pr_u3c6_0cb9_cpu_status st;
        tables(seed, &a);
        reference(&a, &e6, &e10, &ebest, &eslot);
        enter(&c, &a);
        memcpy(before, ram, sizeof ram);
        st = pr_u3c6_0cb9_cpu_run(ram, sizeof ram, &c);
        CHECK(seed, st == PR_U3C6_0CB9_CPU_RETURNED);
        CHECK(seed, c.cs == RET_CS && c.ip == RET_IP);           /* retf 0xE frame */
        CHECK(seed, c.sp == SP0);
        CHECK(seed, c.bp == 0x7777u);
        CHECK(seed, c.ss == SSEG && c.ds == DSEG);
        CHECK(seed, c.flags == 0x0046u);                        /* cmp byte [bp-1],3: PF ZF */
        CHECK(seed, r8(OSEG, 0x0010u) == e6);
        CHECK(seed, r8(OSEG, 0x0020u) == e10);
        CHECK(seed, r16(SSEG, FRAME_BP) == 0x7777u);             /* saved BP */
        CHECK(seed, r8(SSEG, FRAME_BP - 1u) == 3u);              /* i */
        CHECK(seed, r8(SSEG, FRAME_BP - 2u) == eslot);           /* s */
        CHECK(seed, r8(SSEG, FRAME_BP - 5u) == ebest);           /* best rank */
        CHECK(seed, write_set_ok());
    }
}

/* writes into modelled code stop before the instruction; neighbours do not */
static void code_alias_controls(void)
{
    static const struct { uint16_t off; int p10; uint16_t stop_ip; } cases[] = {
        {0x0D00u, 0, 0x0CC9u}, {0x0CB9u, 0, 0x0CC9u}, {0x0E10u, 0, 0x0CC9u}, {0x0E11u, 0, 0},
        {0x0C5Fu, 1, 0x0CC2u}, {0x0C5Eu, 1, 0}, {0x0000u, 1, 0x0CC2u}, {0x0067u, 1, 0x0CC2u},
        {0x0068u, 1, 0}, {0x04BEu, 0, 0x0CC9u}, {0x052Au, 1, 0x0CC2u}, {0x052Bu, 0, 0},
    };
    unsigned n;
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        pr_u3c6_0cb9_cpu c;
        args_t a;
        pr_u3c6_0cb9_cpu_status st;
        unsigned tag = 1000u + n;
        tables(7u, &a);
        if (cases[n].p10) { a.p10_seg = CSEG; a.p10_off = cases[n].off; }
        else { a.p6_seg = CSEG; a.p6_off = cases[n].off; }
        w8(CSEG, cases[n].off, 0xCCu);
        enter(&c, &a);
        st = pr_u3c6_0cb9_cpu_run(ram, sizeof ram, &c);
        if (cases[n].stop_ip == 0) {
            CHECK(tag, st == PR_U3C6_0CB9_CPU_RETURNED);
            continue;
        }
        CHECK(tag, st == PR_U3C6_0CB9_CPU_UNSCREENED_ALIAS);
        CHECK(tag, c.ip == cases[n].stop_ip && c.cs == CSEG);
        CHECK(tag, r8(CSEG, cases[n].off) == 0xCCu);             /* not written */
        CHECK(tag, c.bp == FRAME_BP && c.sp == FRAME_BP - 0x0Cu);
        if (cases[n].p10) CHECK(tag, r8(OSEG, 0x0010u) == 0x55u); /* +0CC9 not reached */
        else CHECK(tag, r8(OSEG, 0x0020u) == 0x00u);              /* +0CC2 done */
    }
}

/* negative control: P6 points at the low byte of its own frame word [bp+6].
 * +0CC9 turns the pointer into SS:0x7F17; every later P6 access must follow
 * it (a cached-pointer model would write 0 back over the frame instead). */
static void self_alias_control(void)
{
    pr_u3c6_0cb9_cpu c;
    args_t a = {SSEG, (uint16_t)(FRAME_BP + 6u), OSEG, 0x0020u, 0x5555u, 0x1234u, 0};
    pr_u3c6_0cb9_cpu_status st;
    memset(ram, 0, sizeof ram);
    w16(DSEG, (uint16_t)(0x65B9u + 4u), a.q_off);                /* slot 1 = Q */
    w16(DSEG, (uint16_t)(0x65BBu + 4u), a.q_seg);
    w8(DSEG, 0x5F2Au, 1u);                                       /* count 1 */
    w8(DSEG, 0x5F2Eu, 1u);                                       /* class of slot 1 */
    w8(SSEG, 0x7F17u, 0x55u);
    enter(&c, &a);
    st = pr_u3c6_0cb9_cpu_run(ram, sizeof ram, &c);
    CHECK(2000u, st == PR_U3C6_0CB9_CPU_RETURNED && c.sp == SP0);
    CHECK(2000u, r8(SSEG, (uint16_t)(FRAME_BP + 6u)) == 0x17u);    /* the +0CC9 store */
    CHECK(2000u, r8(SSEG, (uint16_t)(FRAME_BP + 7u)) == 0x7Fu);
    CHECK(2000u, r8(SSEG, 0x7F17u) == 0x00u);                     /* +0DB5 via the live pointer */
}

static void argument_controls(void)
{
    pr_u3c6_0cb9_cpu c, keep;
    args_t a;
    tables(3u, &a);
    enter(&c, &a);
    keep = c;
    CHECK(3000u, pr_u3c6_0cb9_cpu_run(NULL, sizeof ram, &c) == PR_U3C6_0CB9_CPU_ARGUMENT);
    CHECK(3001u, pr_u3c6_0cb9_cpu_run(ram, sizeof ram - 1u, &c) == PR_U3C6_0CB9_CPU_ARGUMENT);
    CHECK(3002u, pr_u3c6_0cb9_cpu_run(ram, sizeof ram, NULL) == PR_U3C6_0CB9_CPU_ARGUMENT);
    c.ip = 0x0CBAu;
    CHECK(3003u, pr_u3c6_0cb9_cpu_run(ram, sizeof ram, &c) == PR_U3C6_0CB9_CPU_ARGUMENT);
    c.ip = keep.ip;
    CHECK(3004u, memcmp(&c, &keep, sizeof c) == 0);
}

int main(void)
{
    reference_runs();
    code_alias_controls();
    self_alias_control();
    argument_controls();
    if (failures != 0u) {
        fprintf(stderr, "%u of %u checks failed\n", failures, checks);
        return 1;
    }
    printf("u3c6 +0CB9 cpu: %u checks passed\n", checks);
    return 0;
}
