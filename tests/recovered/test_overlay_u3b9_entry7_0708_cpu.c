/* U3B9 +0708 CPU/RAM model: the hook-free null-slot path under the declared
 * RETF policy, the UNMEASURED stop, the FFFD/FFFE/FFFF RETF frames, each
 * stack LES/LDS seam stop with its FFFC control, the HOOK_UNMEASURED stop,
 * HOOK_CONTRACT runs through synthetic hook stand-ins with per-callee frame
 * negatives, code/alias stops, the default CODE_PINNED rejection and
 * argument negatives, on synthetic state. Only the seven call-site bytes the
 * model verifies are planted; no other game bytes are used, so every
 * behavioural case runs under CODE_SYNTHETIC (call sites only, no parity). */
#include "poolrad/recovered/overlay_u3b9_entry7_0708_cpu.h"
#include <stdio.h>
#include <string.h>

#define CSEG 0x2000u     /* unit 0x3B9 load segment */
#define YSEG 0x0AF8u     /* SYSTEM segment of the folded copy */
#define DSEG 0x1000u
#define SSEG 0x3000u
#define RECSEG 0x5000u
#define RECOFF 0x0100u
#define RET_CS 0x0BADu
#define RET_IP 0x1234u
#define SYN PR_U3B9_0708_CODE_SYNTHETIC /* every behavioural case: call sites only, no parity */

static uint8_t ram[PR_U3B9_0708_CPU_RAM_SIZE], before[PR_U3B9_0708_CPU_RAM_SIZE];
static unsigned failures, checks;
static uint16_t sseg = SSEG;

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

/* the seven lcall sites (opcode and offset are verified, segments are live)
 * and a 0x42-byte record whose slot `nonnull` (1..3) is non-nil, or none */
static void fill(unsigned nonnull)
{
    static const struct { uint16_t site, off, seg; } calls[7] = {
        {0x071Cu, 0x0A3Fu, YSEG}, {0x072Du, 0x00FFu, 0x0464u}, {0x073Fu, 0x0919u, YSEG},
        {0x0744u, 0x084Au, YSEG}, {0x0749u, 0x0000u, 0x0899u}, {0x07ACu, 0x0D71u, 0x0464u},
        {0x07B5u, 0x1B52u, 0x0464u}};
    unsigned k;
    memset(ram, 0, sizeof ram);
    for (k = 0; k < 7u; k++) {
        w8(CSEG, calls[k].site, 0x9Au);
        w16(CSEG, (uint16_t)(calls[k].site + 1u), calls[k].off);
        w16(CSEG, (uint16_t)(calls[k].site + 3u), calls[k].seg);
    }
    for (k = 0; k < 0x42u; k++) w8(RECSEG, (uint16_t)(RECOFF + k), (uint8_t)(0xC0u + k));
    for (k = 1; k <= 3u; k++)                      /* slot k: record bytes 8k-2 .. 8k+1 */
        if (k != nonnull) {
            w16(RECSEG, (uint16_t)(RECOFF + 8u * k - 2u), 0u);
            w16(RECSEG, (uint16_t)(RECOFF + 8u * k), 0u);
        }
}
static void enter_at(pr_u3b9_0708_cpu *c, uint16_t sp, uint16_t index)
{
    w16(sseg, sp, RET_IP);
    w16(sseg, (uint16_t)(sp + 2u), RET_CS);
    w16(sseg, (uint16_t)(sp + 4u), RECOFF);
    w16(sseg, (uint16_t)(sp + 6u), RECSEG);
    w16(sseg, (uint16_t)(sp + 8u), index);
    memset(c, 0, sizeof *c);
    c->ax = 0x5A11u; c->bx = 0x2222u; c->cx = 0x3333u; c->dx = 0x4444u;
    c->si = 0x5555u; c->di = 0x6666u; c->bp = 0x7777u; c->sp = sp;
    c->cs = CSEG; c->ip = 0x0708u; c->ds = DSEG; c->es = 0x9999u; c->ss = sseg;
    c->flags = 0x0002u;
}
static pr_u3b9_0708_cpu_status run(pr_u3b9_0708_cpu *c, pr_u3b9_0708_retf_policy p)
{
    return pr_u3b9_0708_cpu_run(ram, sizeof ram, c, p, SYN, NULL);
}

/* index 2, slot 2 nil: the copy, the compares, the OR test and retf 6 */
static void null_slot(void)
{
    static const pr_u3b9_0708_retf_policy pols[2] = {PR_U3B9_0708_RETF_WRAPPED_SS,
                                                       PR_U3B9_0708_RETF_CONTIGUOUS_PHYSICAL};
    const uint16_t sp = 0x7FF0u, bp = (uint16_t)(sp - 2u);
    unsigned p, k;
    for (p = 0; p < 2u; p++) {
        pr_u3b9_0708_cpu c;
        unsigned tag = 100u + p;
        fill(0u);
        enter_at(&c, sp, 2u);
        CHECK(tag, run(&c, pols[p]) == PR_U3B9_0708_CPU_RETURNED);
        CHECK(tag, c.cs == RET_CS && c.ip == RET_IP && c.sp == (uint16_t)(sp + 10u) && c.bp == 0x7777u);
        CHECK(tag, c.ds == DSEG && c.es == SSEG && c.di == 0x0010u && c.ax == 0u && c.cx == 0x0003u);
        CHECK(tag, c.si == (uint16_t)(RECOFF + 0x42u) && c.dx == DSEG && c.bx == (uint16_t)(sp - 0x56u));
        CHECK(tag, c.flags == 0x0046u);                      /* OR result 0: ZF PF; DF clear */
        for (k = 0; k < 0x42u; k++)
            CHECK(tag, r8(SSEG, (uint16_t)(bp - 0x42u + k)) == r8(RECSEG, (uint16_t)(RECOFF + k)));
        CHECK(tag, r16(SSEG, (uint16_t)(sp - 0x52u)) == 0x0042u && r16(SSEG, (uint16_t)(sp - 0x56u)) == 0x0721u);
    }
    /* index 0 and 4 leave through the 0x464:00FF hook: no hooks, so HOOK_PENDING at +072D */
    for (k = 0; k < 2u; k++) {
        pr_u3b9_0708_cpu c;
        fill(0u);
        enter_at(&c, sp, k ? 4u : 0u);
        CHECK(110u + k, run(&c, PR_U3B9_0708_RETF_WRAPPED_SS) == PR_U3B9_0708_CPU_HOOK_PENDING && c.ip == 0x072Du &&
                            c.sp == (uint16_t)(sp - 0x48u));
    }
}

/* UNMEASURED stops before the folded retf 0xA, after the copy */
static void unmeasured(void)
{
    const uint16_t sp = 0x7FF0u;
    pr_u3b9_0708_cpu c;
    fill(0u);
    enter_at(&c, sp, 2u);
    CHECK(200u, run(&c, PR_U3B9_0708_RETF_UNMEASURED) == PR_U3B9_0708_CPU_RETURN_PENDING);
    CHECK(200u, c.cs == YSEG && c.ip == 0x0A54u && c.sp == (uint16_t)(sp - 0x56u) && c.ds == DSEG);
    CHECK(200u, c.cx == 0u && c.si == (uint16_t)(RECOFF + 0x42u) && c.di == (uint16_t)(sp - 2u));
    CHECK(200u, r8(SSEG, (uint16_t)(sp - 0x44u)) == 0xC0u);  /* first copied byte at BP-0x42 */
}

/* RETF policy discriminators: (a) body retf 6 with the IP word at SS:FFFD,
 * FFFE, FFFF; (b) the folded retf 0xA with its IP word at SS:FFFE (entry SP
 * 0x0054). Decoys sit at linear SS*16 + 0x10000 (FFFE) or + 0x10001 (FFFF). */
static void retf_wrap(void)
{
    static const uint16_t sps[3] = {0xFFFDu, 0xFFFEu, 0xFFFFu};
    unsigned k, p;
    for (k = 0; k < 3u; k++)
        for (p = 0; p < 2u; p++) {
            pr_u3b9_0708_cpu c;
            unsigned tag = 300u + 10u * k + p;
            fill(0u);
            enter_at(&c, sps[k], 2u);
            if (k == 1u) w16((uint16_t)(SSEG + 0x1000u), 0x0000u, 0xBEEFu);
            if (k == 2u) w16((uint16_t)(SSEG + 0x1000u), 0x0001u, 0xBEEFu);
            CHECK(tag, run(&c, p ? PR_U3B9_0708_RETF_CONTIGUOUS_PHYSICAL : PR_U3B9_0708_RETF_WRAPPED_SS) ==
                           PR_U3B9_0708_CPU_RETURNED);
            CHECK(tag, c.cs == ((p == 1u && k != 0u) ? 0xBEEFu : RET_CS) && c.ip == RET_IP &&
                           c.sp == (uint16_t)(sps[k] + 10u));
        }
    for (p = 0; p < 2u; p++) {
        pr_u3b9_0708_cpu c;
        unsigned tag = 350u + p;
        pr_u3b9_0708_cpu_status st;
        fill(0u);
        enter_at(&c, 0x0054u, 2u);
        w16((uint16_t)(SSEG + 0x1000u), 0x0000u, 0xBEEFu);
        st = run(&c, p ? PR_U3B9_0708_RETF_CONTIGUOUS_PHYSICAL : PR_U3B9_0708_RETF_WRAPPED_SS);
        if (p == 0u)
            CHECK(tag, st == PR_U3B9_0708_CPU_RETURNED && c.cs == RET_CS && c.sp == 0x005Eu);
        else
            CHECK(tag, st == PR_U3B9_0708_CPU_SYSTEM_TRANSFER && c.cs == 0xBEEFu && c.ip == 0x0721u &&
                           c.sp == 0x000Cu);
    }
}

/* each stack LES/LDS operand at FFFD/FFFE/FFFF stops before it; FFFC runs */
static void les_seams(void)
{
    static const struct {
        uint16_t base;          /* entry SP putting the operand at FFFC */
        uint16_t ip, cs;
        unsigned nonnull;       /* slot 1 non-nil to reach +076C/+0775 */
        pr_u3b9_0708_cpu_status control;
    } sites[5] = {
        {0xFFF8u, 0x070Eu, CSEG, 0u, PR_U3B9_0708_CPU_RETURNED},   /* [bp+6] = SP+4 */
        {0x0048u, 0x0A43u, YSEG, 0u, PR_U3B9_0708_CPU_RETURNED},   /* ss:[bx+0xA] = SP-0x4C */
        {0x004Cu, 0x0A47u, YSEG, 0u, PR_U3B9_0708_CPU_RETURNED},   /* ss:[bx+6] = SP-0x50 */
        {0x003Au, 0x076Cu, CSEG, 1u, PR_U3B9_0708_CPU_HOOK_PENDING},  /* [bp+di-0x44] = SP-0x3E */
        {0x0044u, 0x0775u, CSEG, 1u, PR_U3B9_0708_CPU_HOOK_PENDING},  /* [bp-0x46] = SP-0x48 */
    };
    unsigned s, d;
    for (s = 0; s < 5u; s++)
        for (d = 0; d < 4u; d++) {
            pr_u3b9_0708_cpu c;
            unsigned tag = 400u + 10u * s + d;
            uint16_t sp = (uint16_t)(sites[s].base + d);   /* d = 0: FFFC control */
            uint16_t bp = (uint16_t)(sp - 2u);
            pr_u3b9_0708_cpu_status st;
            fill(sites[s].nonnull);
            enter_at(&c, sp, sites[s].nonnull ? 1u : 2u);
            st = run(&c, PR_U3B9_0708_RETF_WRAPPED_SS);
            if (d == 0u) {
                CHECK(tag, st == sites[s].control);
                if (st == PR_U3B9_0708_CPU_HOOK_PENDING) CHECK(tag, c.ip == 0x07ACu && c.sp == (uint16_t)(sp - 0x58u));
                continue;
            }
            CHECK(tag, st == PR_U3B9_0708_CPU_UNSUPPORTED_LES_SEAM && c.ip == sites[s].ip && c.cs == sites[s].cs);
            switch (s) {
            case 0:     /* nothing after SUB SP */
                CHECK(tag, c.sp == (uint16_t)(sp - 0x48u) && c.di == 0x6666u && c.es == 0x9999u);
                break;
            case 1:     /* inside the copy: MOV BX,SP and MOV DX,DS done, DS:SI untouched */
                CHECK(tag, c.sp == (uint16_t)(sp - 0x56u) && c.bx == c.sp && c.dx == DSEG && c.ds == DSEG &&
                               c.si == 0x5555u);
                break;
            case 2:     /* the LDS done */
                CHECK(tag, c.ds == RECSEG && c.si == RECOFF && c.es == RECSEG);
                break;
            case 3:     /* DI = index << 3, ES from the copy */
                CHECK(tag, c.di == 0x0008u && c.es == SSEG && c.sp == (uint16_t)(sp - 0x48u));
                break;
            default:    /* +076F/+0772 stores done; +0781 is never reached */
                CHECK(tag, r16(SSEG, (uint16_t)(bp - 0x46u)) == c.di && c.es == r16(SSEG, (uint16_t)(bp - 0x44u)));
                break;
            }
        }
}

/* HOOK_CONTRACT stand-ins: each records its entry frame and returns per its
 * callee's contract (RETF n, SS/BP/DS kept) unless `victim` breaks one */
enum { H_00FF, H_0919, H_084A, H_0899, H_0D71, H_1B52 };
enum { M_NONE, M_SP_PLUS2, M_SP_MINUS2, M_SS, M_BP, M_DS, M_FILE, M_TRANSFER, M_FAIL, M_CODE };
typedef struct {
    int victim, mode, r0899;
    unsigned calls[6];
    uint16_t sp[6], flags[6], w[6][8];
} hk_t;
static const uint16_t NPOP[6] = {0u, 6u, 4u, 0u, 0x10u, 0u};

static int stub(hk_t *h, unsigned id, uint8_t *mem, pr_u3b9_0708_cpu *c)
{
    unsigned k;
    if (mem != ram) return 9;
    h->calls[id]++;
    h->sp[id] = c->sp;
    h->flags[id] = c->flags;
    for (k = 0; k < 8u; k++) h->w[id][k] = r16(c->ss, (uint16_t)(c->sp + 4u + 2u * k));
    if (id == H_0899 && h->r0899 != 0) return h->r0899;           /* else it pretends to return */
    if (h->victim == (int)id && h->mode == M_FAIL) return 1;
    if (h->victim == (int)id && h->mode == M_CODE) w8(CSEG, 0x06F0u, (uint8_t)(r8(CSEG, 0x06F0u) ^ 0xFFu));
    c->ip = r16(c->ss, c->sp);                                       /* RETF n */
    c->cs = r16(c->ss, (uint16_t)(c->sp + 2u));
    c->sp = (uint16_t)(c->sp + 4u + NPOP[id]);
    if (h->victim == (int)id) {
        switch (h->mode) {
        case M_SP_PLUS2: c->sp = (uint16_t)(c->sp + 2u); break;
        case M_SP_MINUS2: c->sp = (uint16_t)(c->sp - 2u); break;
        case M_SS: c->ss = (uint16_t)(c->ss + 1u); break;
        case M_BP: c->bp ^= 2u; break;
        case M_DS: c->ds ^= 1u; break;
        case M_FILE: w16(c->ss, c->sp, (uint16_t)(r16(c->ss, c->sp) ^ 1u)); break;
        case M_TRANSFER: c->ip ^= 1u; break;
        default: break;
        }
    }
    c->ax = 0xA5A5u; c->bx = 0xB6B6u; c->cx = 0xC7C7u; c->dx = 0xD8D8u; c->si = 0xE9E9u; c->di = 0xFAFAu;
    return 0;
}
static int s00ff(void *x, uint8_t *m, size_t n, pr_u3b9_0708_cpu *c) { (void)n; return stub(x, H_00FF, m, c); }
static int s0919(void *x, uint8_t *m, size_t n, pr_u3b9_0708_cpu *c) { (void)n; return stub(x, H_0919, m, c); }
static int s084a(void *x, uint8_t *m, size_t n, pr_u3b9_0708_cpu *c) { (void)n; return stub(x, H_084A, m, c); }
static int s0899(void *x, uint8_t *m, size_t n, pr_u3b9_0708_cpu *c) { (void)n; return stub(x, H_0899, m, c); }
static int s0d71(void *x, uint8_t *m, size_t n, pr_u3b9_0708_cpu *c) { (void)n; return stub(x, H_0D71, m, c); }
static int s1b52(void *x, uint8_t *m, size_t n, pr_u3b9_0708_cpu *c) { (void)n; return stub(x, H_1B52, m, c); }

static pr_u3b9_0708_cpu_hooks contract(hk_t *h, int victim, int mode)
{
    pr_u3b9_0708_cpu_hooks hooks;
    memset(h, 0, sizeof *h);
    h->victim = victim;
    h->mode = mode;
    hooks.context = h;
    hooks.hook_policy = PR_U3B9_0708_HOOK_CONTRACT;
    hooks.resident_0464_00ff = s00ff;
    hooks.system_0919 = s0919;
    hooks.system_084a = s084a;
    hooks.resident_0899_0000 = s0899;
    hooks.resident_0464_0d71 = s0d71;
    hooks.resident_0464_1b52 = s1b52;
    return hooks;
}
/* slot 1 pointing at an object 0x6000:0x0010 with x0 = 0x0100, y0 = 0x0050 */
static void good_slot(void)
{
    fill(1u);
    w16(RECSEG, (uint16_t)(RECOFF + 6u), 0x0010u);
    w16(RECSEG, (uint16_t)(RECOFF + 8u), 0x6000u);
    w16(0x6000u, 0x0014u, 0x0100u);
    w16(0x6000u, 0x0016u, 0x0050u);
    w16(DSEG, 0x4A20u, 0xAAAAu);
    w16(DSEG, 0x4A22u, 0xBBBBu);
}

static void hook_paths(void)
{
    const uint16_t sp = 0x7FF0u, bp = (uint16_t)(sp - 2u);
    pr_u3b9_0708_cpu c;
    hk_t h;
    pr_u3b9_0708_cpu_hooks hooks;
    pr_u3b9_0708_cpu_status st;
    /* default: hooks present but HOOK_UNMEASURED */
    hooks = contract(&h, -1, M_NONE);
    hooks.hook_policy = PR_U3B9_0708_HOOK_UNMEASURED;
    fill(0u);
    enter_at(&c, sp, 0u);
    st = pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, &hooks);
    CHECK(600u, st == PR_U3B9_0708_CPU_HOOK_PENDING && c.ip == 0x072Du && h.calls[H_00FF] == 0u);
    /* HOOK_CONTRACT with a missing function */
    hooks = contract(&h, -1, M_NONE);
    hooks.resident_0464_00ff = NULL;
    fill(0u);
    enter_at(&c, sp, 0u);
    CHECK(601u, pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3B9_0708_CPU_NEED_HOOK && c.ip == 0x072Du);
    /* bad index: 00FF, Write, Writeln, then 0x899:0000 must not return */
    hooks = contract(&h, -1, M_NONE);
    fill(0u);
    enter_at(&c, sp, 0u);
    st = pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, &hooks);
    CHECK(610u, st == PR_U3B9_0708_CPU_CALLEE_NONRETURNING && c.cs == CSEG && c.ip == 0x074Eu);
    CHECK(610u, h.calls[H_00FF] == 1u && h.calls[H_0919] == 1u && h.calls[H_084A] == 1u && h.calls[H_0899] == 1u);
    CHECK(611u, h.sp[H_00FF] == (uint16_t)(sp - 0x4Cu) && h.sp[H_0899] == (uint16_t)(sp - 0x4Cu));
    CHECK(612u, h.sp[H_0919] == (uint16_t)(sp - 0x56u) && h.w[H_0919][0] == 0u && h.w[H_0919][1] == 0x06E9u &&
                    h.w[H_0919][2] == CSEG && h.w[H_0919][3] == 0x8608u && h.w[H_0919][4] == DSEG);
    CHECK(613u, h.sp[H_084A] == (uint16_t)(sp - 0x50u) && h.w[H_084A][0] == 0x8608u && h.w[H_084A][1] == DSEG);
    h.r0899 = 1;
    fill(0u);
    enter_at(&c, sp, 0u);
    h.calls[H_00FF] = 0u;
    CHECK(614u, pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3B9_0708_CPU_CALLEE_FAILED);
    /* good slot: 0D71 with eight argument words, then 1B52, then retf 6 */
    hooks = contract(&h, -1, M_NONE);
    good_slot();
    enter_at(&c, sp, 1u);
    st = pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, &hooks);
    CHECK(620u, st == PR_U3B9_0708_CPU_RETURNED && c.cs == RET_CS && c.ip == RET_IP && c.sp == (uint16_t)(sp + 10u));
    CHECK(620u, c.bp == 0x7777u && h.calls[H_0D71] == 1u && h.calls[H_1B52] == 1u && h.calls[H_00FF] == 0u);
    CHECK(621u, h.sp[H_0D71] == (uint16_t)(sp - 0x5Cu) && h.sp[H_1B52] == (uint16_t)(bp - 0x46u - 4u));
    CHECK(622u, h.w[H_0D71][0] == 0xAAAAu && h.w[H_0D71][1] == 0xBBBBu && h.w[H_0D71][2] == 0x0010u &&
                    h.w[H_0D71][3] == 0x6000u && h.w[H_0D71][4] == 0x0001u && h.w[H_0D71][5] == 0x0000u &&
                    h.w[H_0D71][6] == 0x0052u && h.w[H_0D71][7] == 0x0102u);
    /* the third SHL's flags at the 0D71 entry: AF = 1, OF = 0 under the pinned lazy-flag claim */
    CHECK(623u, h.flags[H_0D71] == 0x0012u);
}

/* each contract item broken once: CALLEE_FRAME, TRANSFER, FAILED or CODE_CHANGED */
static void hook_negatives(void)
{
    static const struct { int victim, mode; unsigned index; pr_u3b9_0708_cpu_status want; } cases[] = {
        {H_00FF, M_SP_PLUS2, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_00FF, M_SP_MINUS2, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_00FF, M_SS, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_00FF, M_BP, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_00FF, M_DS, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_00FF, M_TRANSFER, 0u, PR_U3B9_0708_CPU_CALLEE_TRANSFER},
        {H_00FF, M_FAIL, 0u, PR_U3B9_0708_CPU_CALLEE_FAILED},
        {H_00FF, M_CODE, 0u, PR_U3B9_0708_CPU_CODE_CHANGED},
        {H_0919, M_FILE, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_0919, M_SP_PLUS2, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_0919, M_DS, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_084A, M_BP, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_084A, M_SP_MINUS2, 0u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_0D71, M_SP_MINUS2, 1u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_0D71, M_SP_PLUS2, 1u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_0D71, M_SS, 1u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_0D71, M_CODE, 1u, PR_U3B9_0708_CPU_CODE_CHANGED},
        {H_1B52, M_BP, 1u, PR_U3B9_0708_CPU_CALLEE_FRAME},
        {H_1B52, M_TRANSFER, 1u, PR_U3B9_0708_CPU_CALLEE_TRANSFER},
    };
    const uint16_t sp = 0x7FF0u;
    unsigned n, k;
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        pr_u3b9_0708_cpu c;
        hk_t h;
        pr_u3b9_0708_cpu_hooks hooks = contract(&h, cases[n].victim, cases[n].mode);
        unsigned tag = 700u + n;
        if (cases[n].index) good_slot(); else fill(0u);
        enter_at(&c, sp, cases[n].index);
        CHECK(tag, pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, &hooks) ==
                       cases[n].want);
        CHECK(tag, h.calls[cases[n].victim] == 1u);
        for (k = (unsigned)cases[n].victim + 1u; k < 6u; k++) CHECK(tag, h.calls[k] == 0u);
    }
}

/* after a hook, the conservative fixed 0x20-byte forward window meets the
 * entry code: SS = CS, entry SP 0x0820 clears the up-front screen [07C4,0820)
 * but after +00FF the window [07B8,07D8) meets [06E9,07C0). The real remaining
 * pushes end at 07CA, so this stop is early but safe. */
static void hook_alias(void)
{
    pr_u3b9_0708_cpu c;
    hk_t h;
    pr_u3b9_0708_cpu_hooks hooks = contract(&h, -1, M_NONE);
    sseg = CSEG;
    fill(0u);
    enter_at(&c, 0x0820u, 0u);
    CHECK(800u, pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3B9_0708_CPU_UNSCREENED_ALIAS && h.calls[H_00FF] == 1u && h.calls[H_0919] == 0u);
    sseg = SSEG;
}

/* argument negatives leave CPU and RAM unchanged */
static void arguments(void)
{
    pr_u3b9_0708_cpu c, keep;
    fill(0u);
    enter_at(&c, 0x7FF0u, 2u);
    keep = c;
    memcpy(before, ram, sizeof ram);
    CHECK(500u, pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, (pr_u3b9_0708_retf_policy)3, SYN, NULL) ==
                    PR_U3B9_0708_CPU_ARGUMENT);
    CHECK(501u, pr_u3b9_0708_cpu_run(NULL, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, NULL) ==
                    PR_U3B9_0708_CPU_ARGUMENT);
    CHECK(502u, pr_u3b9_0708_cpu_run(ram, sizeof ram - 1u, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, NULL) ==
                    PR_U3B9_0708_CPU_ARGUMENT);
    {
        hk_t h;
        pr_u3b9_0708_cpu_hooks hooks = contract(&h, -1, M_NONE);
        CHECK(506u, pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS,
                                         (pr_u3b9_0708_code_policy)2, NULL) == PR_U3B9_0708_CPU_ARGUMENT);
        hooks.hook_policy = (pr_u3b9_0708_hook_policy)2;
        CHECK(507u, pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS, SYN, &hooks) ==
                        PR_U3B9_0708_CPU_ARGUMENT);
        /* the default CODE_PINNED rejects the synthetic code before anything runs */
        hooks.hook_policy = PR_U3B9_0708_HOOK_CONTRACT;
        CHECK(508u, pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_WRAPPED_SS,
                                         PR_U3B9_0708_CODE_PINNED, &hooks) == PR_U3B9_0708_CPU_CODE_NOT_ORIGINAL);
        CHECK(509u, pr_u3b9_0708_cpu_run(ram, sizeof ram, &c, PR_U3B9_0708_RETF_UNMEASURED,
                                         (pr_u3b9_0708_code_policy)0, NULL) == PR_U3B9_0708_CPU_CODE_NOT_ORIGINAL);
        CHECK(510u, h.calls[H_00FF] == 0u);
    }
    w8(CSEG, 0x0744u, 0xE8u);
    CHECK(503u, run(&c, PR_U3B9_0708_RETF_WRAPPED_SS) == PR_U3B9_0708_CPU_ARGUMENT);
    w8(CSEG, 0x0744u, 0x9Au);
    c.ip = 0x0709u;
    CHECK(504u, run(&c, PR_U3B9_0708_RETF_WRAPPED_SS) == PR_U3B9_0708_CPU_ARGUMENT);
    c.ip = keep.ip;
    CHECK(505u, memcmp(&c, &keep, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
}

int main(void)
{
    null_slot();
    unmeasured();
    retf_wrap();
    les_seams();
    hook_paths();
    hook_negatives();
    hook_alias();
    arguments();
    if (failures != 0u) {
        fprintf(stderr, "%u of %u checks failed\n", failures, checks);
        return 1;
    }
    printf("u3b9 +0708 cpu: %u checks passed\n", checks);
    return 0;
}
