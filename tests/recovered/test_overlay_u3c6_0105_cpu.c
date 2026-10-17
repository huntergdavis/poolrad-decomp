/* U3C6 +0105 CPU/RAM model: the [0x4A1E] == 1 gate, the two folded 0x2A0D
 * palette calls with a hook-captured Intr 0xA32:+000B frame and record, the
 * declared Intr policy (UNMEASURED stop, bounded hook region), the hook's
 * RETF 6 and tail-output contract, live argument aliasing, code/alias stops,
 * the declared RETF policy and argument negatives, on synthetic state. Only
 * the call-operand bytes the model verifies are planted; no other game bytes
 * are used, so every behavioural case runs under CODE_SYNTHETIC (call sites
 * only, other instruction bytes unverified) and the default CODE_PINNED is
 * exercised only as a rejection. A RETURNED run here goes through a synthetic
 * Intr stand-in: a modelled subset, not BIOS evidence. */
#include "poolrad/recovered/overlay_u3c6_0105_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSEG 0x2000u     /* unit 0x3C6 load segment */
#define RSEG 0x0600u     /* resident 0x464 segment (relocated) */
#define ISEG 0x0A32u     /* DOS-unit segment */
#define DSEG 0x1000u
#define SSEG 0x3000u
#define RET_CS 0x0BADu
#define RET_IP 0x1234u
#define ENTRY_SP 0x7FFCu /* SP on the return IP; no arguments */
#define DEPTH 0x0100u    /* declared BIOS stack depth for the ordinary cases */
#define F_DF 0x0400u
#define SYN PR_U3C6_0105_CODE_SYNTHETIC /* every behavioural case: call sites only, no parity */

static uint8_t ram[PR_U3C6_0105_CPU_RAM_SIZE], before[PR_U3C6_0105_CPU_RAM_SIZE];
static unsigned failures, checks;
static uint16_t cseg = CSEG;

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

/* RAM bytes that differ from `before` outside [lo1,hi1) and [lo2,hi2) (linear) */
static unsigned stray(size_t lo1, size_t hi1, size_t lo2, size_t hi2)
{
    size_t p;
    unsigned n = 0;
    for (p = 0; p < sizeof ram; p++)
        if (ram[p] != before[p] && !(p >= lo1 && p < hi1) && !(p >= lo2 && p < hi2)) n++;
    return n;
}

/* the operand bytes the model verifies before running (both entry lcalls to
 * RSEG:2A0D and the folded body's lcall to ISEG:000B), the gate, and stale
 * record inputs DS:[0x4A06..0x4A13] */
static void fill(uint8_t gate)
{
    unsigned k;
    memset(ram, 0, sizeof ram);
    w8(cseg, 0x0115u, 0x9Au); w16(cseg, 0x0116u, 0x2A0Du); w16(cseg, 0x0118u, RSEG);
    w8(cseg, 0x0120u, 0x9Au); w16(cseg, 0x0121u, 0x2A0Du); w16(cseg, 0x0123u, RSEG);
    w8(RSEG, 0x2A41u, 0x9Au); w16(RSEG, 0x2A42u, 0x000Bu); w16(RSEG, 0x2A44u, ISEG);
    w8(DSEG, 0x4A1Eu, gate);
    for (k = 0; k < 14u; k++) w8(DSEG, (uint16_t)(0x4A06u + k), (uint8_t)(0xA0u + k));
}
static void enter_at(pr_u3c6_0105_cpu *c, uint16_t ss, uint16_t entry_sp)
{
    w16(ss, (uint16_t)(entry_sp + 2u), RET_CS);
    w16(ss, entry_sp, RET_IP);
    memset(c, 0, sizeof *c);
    c->ax = 0x5A11u; c->bx = 0x2222u; c->cx = 0x3333u; c->dx = 0x4444u;
    c->si = 0x5555u; c->di = 0x6666u; c->bp = 0x7777u; c->sp = entry_sp;
    c->cs = cseg; c->ip = 0x0105u; c->ds = DSEG; c->es = 0x9999u; c->ss = ss;
    c->flags = 0x0002u;
}

enum {
    H_NORMAL, H_FAIL, H_TRANSFER, H_SHORT_POP, H_NEW_SS, H_CODE_ENTRY, H_CODE_RES, H_CODE_INTR,
    H_BAD_AX, H_BAD_FLAGS, H_DF, H_BAD_BX, H_BAD_CX, H_BAD_DX, H_BAD_SI, H_BAD_DI, H_BAD_ES, H_BAD_DS, H_BAD_BP
};
typedef struct {
    int mode, write_record;
    unsigned calls;
    uint16_t sp[2], ret_ip[2], ret_cs[2], di[2], ds[2], axw[2], flags[2], value_w[2], reg_w[2];
    uint8_t regs[2][4], stale[2][14];
    uint16_t out_flags;
} hook_t;

/* Intr stand-in: captures its entry frame and record, optionally stores an
 * output record (AX BX CX DX BP SI DI DS ES Flags), does the RETF 6 and leaves
 * the CPU outputs of Intr's tail unless a mode breaks one */
static int intr_stub(void *context, uint8_t *mem, size_t size, pr_u3c6_0105_cpu *c)
{
    static const uint16_t out[9] = {0x1111u, 0x2B2Bu, 0x3C3Cu, 0x4D4Du, 0x5E5Eu, 0x6F6Fu, 0x7A7Au, 0x8B8Bu, 0x9C9Cu};
    hook_t *h = context;
    unsigned n = h->calls, k;
    (void)size;
    if (mem != ram || c->cs != ISEG || c->ip != 0x000Bu) return 9;
    if (n < 2u) {
        h->sp[n] = c->sp;
        h->ret_ip[n] = r16(c->ss, c->sp);
        h->ret_cs[n] = r16(c->ss, (uint16_t)(c->sp + 2u));
        h->di[n] = r16(c->ss, (uint16_t)(c->sp + 4u));
        h->ds[n] = r16(c->ss, (uint16_t)(c->sp + 6u));
        h->axw[n] = r16(c->ss, (uint16_t)(c->sp + 8u));
        h->value_w[n] = r16(c->ss, (uint16_t)(c->sp + 16u));
        h->reg_w[n] = r16(c->ss, (uint16_t)(c->sp + 18u));
        h->flags[n] = c->flags;
        for (k = 0; k < 4u; k++) h->regs[n][k] = r8(c->ds, (uint16_t)(0x4A02u + k));
        for (k = 0; k < 14u; k++) h->stale[n][k] = r8(c->ds, (uint16_t)(0x4A06u + k));
    }
    h->calls++;
    if (h->mode == H_FAIL) return 1;
    if (h->mode == H_CODE_ENTRY) w8(cseg, 0x0110u, (uint8_t)(r8(cseg, 0x0110u) ^ 0xFFu));
    if (h->mode == H_CODE_RES) w8(RSEG, 0x2A20u, (uint8_t)(r8(RSEG, 0x2A20u) ^ 0xFFu));
    if (h->mode == H_CODE_INTR) w8(ISEG, 0x006Bu, (uint8_t)(r8(ISEG, 0x006Bu) ^ 0xFFu));
    if (h->write_record) {
        for (k = 0; k < 9u; k++) w16(c->ds, (uint16_t)(0x4A02u + 2u * k), out[k]);
        w16(c->ds, 0x4A14u, h->out_flags);
    }
    /* RETF 6 */
    c->ip = r16(c->ss, c->sp);
    c->cs = r16(c->ss, (uint16_t)(c->sp + 2u));
    c->sp = (uint16_t)(c->sp + (h->mode == H_SHORT_POP ? 8u : 10u));
    if (h->mode == H_TRANSFER) c->ip = 0x4321u;
    if (h->mode == H_NEW_SS) c->ss = (uint16_t)(c->ss + 1u);
    /* the tail's outputs: DS and BP kept */
    c->ax = r16(c->ds, 0x4A14u);
    c->bx = r16(c->ds, 0x4A04u);
    c->cx = r16(c->ds, 0x4A06u);
    c->dx = r16(c->ds, 0x4A08u);
    c->si = r16(c->ds, 0x4A0Cu);
    c->di = 0x4A16u;
    c->es = c->ds;
    c->flags = (uint16_t)(c->ax & ~F_DF);
    switch (h->mode) {
    case H_BAD_AX: c->ax ^= 0x0100u; break;
    case H_BAD_FLAGS: c->flags ^= 0x0001u; break;
    case H_DF: c->flags |= F_DF; break;
    case H_BAD_BX: c->bx ^= 1u; break;
    case H_BAD_CX: c->cx ^= 1u; break;
    case H_BAD_DX: c->dx ^= 1u; break;
    case H_BAD_SI: c->si ^= 1u; break;
    case H_BAD_DI: c->di = 0x4A14u; break;
    case H_BAD_ES: c->es ^= 1u; break;
    case H_BAD_DS: c->ds ^= 1u; break;
    case H_BAD_BP: c->bp ^= 2u; break;
    default: break;
    }
    return 0;
}

static pr_u3c6_0105_cpu_hooks bounded(hook_t *h, uint16_t depth)
{
    pr_u3c6_0105_cpu_hooks hooks;
    hooks.context = h;
    hooks.intr_policy = PR_U3C6_0105_INTR_HOOK_BOUNDED;
    hooks.bios_depth = depth;
    hooks.dos_intr_a32_000b = intr_stub;
    return hooks;
}
static void hook_init(hook_t *h, uint16_t out_flags)
{
    memset(h, 0, sizeof *h);
    h->out_flags = out_flags;
}

static void gate_on(void)
{
    static const pr_u3c6_0105_retf_policy pols[2] = {PR_U3C6_0105_RETF_WRAPPED_SS,
                                                       PR_U3C6_0105_RETF_CONTIGUOUS_PHYSICAL};
    static const uint8_t out_stale[14] = {0x3Cu, 0x3Cu, 0x4Du, 0x4Du, 0x5Eu, 0x5Eu, 0x6Fu,
                                          0x6Fu, 0x7Au, 0x7Au, 0x8Bu, 0x8Bu, 0x9Cu, 0x9Cu};
    unsigned p, k;
    for (p = 0; p < 2u; p++) {
        pr_u3c6_0105_cpu c;
        hook_t h;
        pr_u3c6_0105_cpu_hooks hooks;
        unsigned tag = 100u + p;
        hook_init(&h, 0x0CD7u);                                                 /* DF set in the record */
        h.write_record = 1;
        hooks = bounded(&h, DEPTH);
        fill(1u);
        enter_at(&c, SSEG, ENTRY_SP);
        memcpy(before, ram, sizeof ram);
        CHECK(tag, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, pols[p], SYN, &hooks) == PR_U3C6_0105_CPU_RETURNED);
        CHECK(tag, h.calls == 2u);
        /* call 1: (register 0, value 0); cmp 0,8 gives CF, AF and SF at the Intr entry */
        CHECK(tag, h.sp[0] == 0x7FE6u && h.ret_ip[0] == 0x2A46u && h.ret_cs[0] == RSEG);
        CHECK(tag, h.di[0] == 0x4A02u && h.ds[0] == DSEG && h.axw[0] == 0x5A10u);  /* int 0x10, AH carried */
        CHECK(tag, h.value_w[0] == 0x5A00u && h.reg_w[0] == 0x5A00u);
        CHECK(tag, h.regs[0][0] == 0x00u && h.regs[0][1] == 0x10u && h.regs[0][2] == 0x00u && h.regs[0][3] == 0x00u);
        CHECK(tag, h.flags[0] == 0x0093u);                                          /* 0x0002 | CF AF SF */
        for (k = 0; k < 14u; k++) CHECK(tag, h.stale[0][k] == (uint8_t)(0xA0u + k)); /* stale inputs untouched */
        /* call 2: AH is the first Intr's AX high byte; (register 8, value 8 -> 16) */
        CHECK(tag, h.sp[1] == 0x7FE6u && h.axw[1] == 0x0010u);
        CHECK(tag, h.value_w[1] == 0x0C08u && h.reg_w[1] == 0x0C08u);
        CHECK(tag, h.regs[1][0] == 0x00u && h.regs[1][1] == 0x10u && h.regs[1][2] == 0x08u && h.regs[1][3] == 0x10u);
        CHECK(tag, h.flags[1] == 0x0012u);                                          /* reserved bit | AF */
        CHECK(tag, memcmp(h.stale[1], out_stale, sizeof out_stale) == 0);          /* the first call's outputs */
        /* return: the second Intr's outputs survive POP BP/RETF 4/POP BP/RETF */
        CHECK(tag, c.cs == RET_CS && c.ip == RET_IP && c.sp == (uint16_t)(ENTRY_SP + 4u) && c.bp == 0x7777u);
        CHECK(tag, c.ax == 0x0CD7u && c.flags == 0x08D7u && c.bx == 0x2B2Bu && c.cx == 0x3C3Cu);
        CHECK(tag, c.dx == 0x4D4Du && c.si == 0x6F6Fu && c.di == 0x4A16u && c.es == DSEG);
        CHECK(tag, c.ss == SSEG && c.ds == DSEG && r16(DSEG, 0x4A14u) == 0x0CD7u);
        /* writes: only the (synthetic) record and the stack from S up */
        CHECK(tag, stray(at(DSEG, 0x4A02u), at(DSEG, 0x4A16u), at(SSEG, 0x7FE6u), at(SSEG, ENTRY_SP)) == 0u);
    }
}

/* gate not 1: no call and no store; the flags of cmp al,1 */
static void gate_off(void)
{
    static const uint8_t gates[3] = {0x00u, 0x02u, 0xFFu};
    unsigned n;
    for (n = 0; n < 3u; n++) {
        pr_u3c6_0105_cpu c;
        hook_t h;
        pr_u3c6_0105_cpu_hooks hooks;
        unsigned tag = 200u + n;
        uint8_t g = gates[n], r = (uint8_t)(g - 1u);
        uint16_t f = 0x0002u;
        unsigned i, ones = 0;
        hook_init(&h, 0x0002u);
        hooks = bounded(&h, DEPTH);
        fill(g);
        enter_at(&c, SSEG, ENTRY_SP);
        memcpy(before, ram, sizeof ram);
        for (i = 0; i < 8u; i++) ones += (r >> i) & 1u;
        if (ones % 2u == 0u) f |= 0x0004u;
        if (g < 1u) f |= 0x0001u;
        if ((g ^ 1u ^ r) & 0x10u) f |= 0x0010u;
        if (r & 0x80u) f |= 0x0080u;
        if ((g ^ 1u) & (g ^ r) & 0x80u) f |= 0x0800u;
        CHECK(tag, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                       PR_U3C6_0105_CPU_RETURNED);
        CHECK(tag, h.calls == 0u && c.flags == f && c.ax == (uint16_t)(0x5A00u | g));
        CHECK(tag, c.cs == RET_CS && c.ip == RET_IP && c.sp == (uint16_t)(ENTRY_SP + 4u) && c.bp == 0x7777u);
        CHECK(tag, stray(at(SSEG, 0x7FFAu), at(SSEG, ENTRY_SP), 0u, 0u) == 0u);   /* PUSH BP only */
    }
}

/* RETF UNMEASURED fails closed at the first RETF reached */
static void retf_unmeasured(void)
{
    pr_u3c6_0105_cpu c;
    hook_t h;
    pr_u3c6_0105_cpu_hooks hooks;
    hook_init(&h, 0x0246u);
    h.write_record = 1;
    hooks = bounded(&h, DEPTH);
    fill(1u);
    enter_at(&c, SSEG, ENTRY_SP);
    CHECK(300u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_UNMEASURED, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_RETURN_PENDING);
    CHECK(300u, h.calls == 1u && c.cs == RSEG && c.ip == 0x2A47u);              /* the first folded RETF 4 */
    CHECK(300u, c.sp == 0x7FF2u && c.bp == 0x7FFAu && r16(SSEG, 0x7FF2u) == 0x011Au);
    CHECK(300u, c.ax == 0x0246u && c.flags == 0x0246u && c.di == 0x4A16u);      /* the Intr effects kept */
    CHECK(300u, r16(DSEG, 0x4A04u) == 0x2B2Bu && r16(DSEG, 0x4A14u) == 0x0246u);
    fill(0u);
    enter_at(&c, SSEG, ENTRY_SP);
    CHECK(301u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_UNMEASURED, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_RETURN_PENDING);
    CHECK(301u, c.cs == cseg && c.ip == 0x0126u && c.sp == ENTRY_SP && c.bp == 0x7777u);  /* body RETF */
}

/* the Intr policy: UNMEASURED stops before the far call; a bounded hook's
 * region SS:[S-20-D,S) must miss both bodies, the record, IVT[0x10] and the
 * live frame, and stops at the same point with nothing changed otherwise */
static void intr_policy(void)
{
    static const struct {
        uint16_t ss, sp, ds, depth;
        pr_u3c6_0105_cpu_status want;
    } cases[] = {
        {CSEG, 0x0200u, DSEG, 0x00AFu, PR_U3C6_0105_CPU_RETURNED},          /* lowest byte 0127: clear */
        {CSEG, 0x0200u, DSEG, 0x00B0u, PR_U3C6_0105_CPU_INTR_STACK_ALIAS},  /* 0126: the entry body */
        {RSEG, 0x2B00u, DSEG, 0x008Cu, PR_U3C6_0105_CPU_RETURNED},          /* lowest byte 2A4A: clear */
        {RSEG, 0x2B00u, DSEG, 0x008Du, PR_U3C6_0105_CPU_INTR_STACK_ALIAS},  /* 2A49: the folded body */
        {SSEG, ENTRY_SP, 0x3350u, DEPTH, PR_U3C6_0105_CPU_INTR_STACK_ALIAS}, /* the record at SS:7F02 */
        {0x0000u, 0x0100u, DSEG, 0x0092u, PR_U3C6_0105_CPU_RETURNED},       /* lowest byte 0044: clear */
        {0x0000u, 0x0100u, DSEG, 0x0093u, PR_U3C6_0105_CPU_INTR_STACK_ALIAS}, /* 0043: IVT[0x10] */
        {SSEG, ENTRY_SP, DSEG, 0xFFD2u, PR_U3C6_0105_CPU_RETURNED},         /* wraps up to S + 0x1A */
        {SSEG, ENTRY_SP, DSEG, 0xFFD3u, PR_U3C6_0105_CPU_INTR_STACK_ALIAS}, /* S + 0x19: the live frame */
        {SSEG, ENTRY_SP, DSEG, PR_U3C6_0105_BIOS_DEPTH_MAX, PR_U3C6_0105_CPU_INTR_STACK_ALIAS},
        {ISEG, 0x0100u, DSEG, 0x006Au, PR_U3C6_0105_CPU_RETURNED},          /* lowest byte 006C: clear */
        {ISEG, 0x0100u, DSEG, 0x006Bu, PR_U3C6_0105_CPU_INTR_STACK_ALIAS},  /* 006B: Intr's code */
        {SSEG, ENTRY_SP, 0x0592u, DEPTH, PR_U3C6_0105_CPU_INTR_STACK_ALIAS}, /* record 4A0B.. on Intr's code */
        /* the record against the call's own frame SS:[S,S+10) */
        {SSEG, 0x7FF6u, 0x335Eu, DEPTH, PR_U3C6_0105_CPU_INTR_FRAME},       /* record at S+2 */
        {SSEG, ENTRY_SP, 0x335Du, 0x0000u, PR_U3C6_0105_CPU_INTR_STACK_ALIAS}, /* record ends at S-1 */
        {SSEG, 0x7FFBu, 0x335Du, 0x0000u, PR_U3C6_0105_CPU_INTR_FRAME},     /* record ends at S */
        {SSEG, 0x0010u, 0x2B60u, DEPTH, PR_U3C6_0105_CPU_INTR_FRAME},       /* wrap: S = FFFA, record SS:0002 */
        /* wrap: S = FFF8, SS:[S,S+10) clear; the record over the caller frame above S+10 is not
         * pre-checked but modelled live, and its stores send the folded RETF 4 elsewhere */
        {SSEG, 0x000Eu, 0x2B60u, DEPTH, PR_U3C6_0105_CPU_SYSTEM_TRANSFER},
    };
    pr_u3c6_0105_cpu c, stop;
    hook_t h;
    pr_u3c6_0105_cpu_hooks hooks;
    unsigned n;
    /* no hooks, and declared UNMEASURED with a hook present */
    for (n = 0; n < 2u; n++) {
        unsigned tag = 800u + n;
        hook_init(&h, 0x0002u);
        hooks = bounded(&h, DEPTH);
        hooks.intr_policy = PR_U3C6_0105_INTR_UNMEASURED;
        fill(1u);
        enter_at(&c, SSEG, ENTRY_SP);
        CHECK(tag, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, n ? &hooks : NULL) ==
                       PR_U3C6_0105_CPU_INTR_PENDING);
        CHECK(tag, h.calls == 0u && c.cs == RSEG && c.ip == 0x2A41u && c.sp == (uint16_t)(ENTRY_SP - 0x12u));
        CHECK(tag, r8(DSEG, 0x4A02u) == 0x00u && r8(DSEG, 0x4A03u) == 0x10u && r8(DSEG, 0x4A04u) == 0x00u &&
                       r8(DSEG, 0x4A05u) == 0x00u);
        CHECK(tag, r16(SSEG, 0x7FEAu) == 0x4A02u && r16(SSEG, 0x7FECu) == DSEG && r16(SSEG, 0x7FEEu) == 0x5A10u);
    }
    /* a bounded policy without a hook function */
    hook_init(&h, 0x0002u);
    hooks = bounded(&h, DEPTH);
    hooks.dos_intr_a32_000b = NULL;
    fill(1u);
    enter_at(&c, SSEG, ENTRY_SP);
    CHECK(802u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_NEED_HOOK && c.ip == 0x2A41u && c.sp == (uint16_t)(ENTRY_SP - 0x12u));
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        unsigned tag = 810u + n;
        pr_u3c6_0105_cpu_status st;
        /* reference: the UNMEASURED stop on the same state */
        hook_init(&h, 0x0002u);
        hooks = bounded(&h, cases[n].depth);
        hooks.intr_policy = PR_U3C6_0105_INTR_UNMEASURED;
        fill(1u);
        enter_at(&stop, cases[n].ss, cases[n].sp);
        stop.ds = cases[n].ds;
        w8(cases[n].ds, 0x4A1Eu, 1u);
        CHECK(tag, pr_u3c6_0105_cpu_run(ram, sizeof ram, &stop, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                       PR_U3C6_0105_CPU_INTR_PENDING);
        memcpy(before, ram, sizeof ram);
        hooks.intr_policy = PR_U3C6_0105_INTR_HOOK_BOUNDED;
        fill(1u);
        enter_at(&c, cases[n].ss, cases[n].sp);
        c.ds = cases[n].ds;
        w8(cases[n].ds, 0x4A1Eu, 1u);
        st = pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks);
        CHECK(tag, st == cases[n].want);
        if (cases[n].want == PR_U3C6_0105_CPU_INTR_STACK_ALIAS || cases[n].want == PR_U3C6_0105_CPU_INTR_FRAME)
            CHECK(tag, h.calls == 0u && memcmp(&c, &stop, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
        else if (cases[n].want == PR_U3C6_0105_CPU_SYSTEM_TRANSFER)
            CHECK(tag, h.calls == 1u && c.cs == cseg && c.ip != 0x011Au);   /* the record stores hit the live frame */
        else
            CHECK(tag, h.calls == 2u && c.cs == RET_CS && c.ip == RET_IP);
    }
}

/* hook contract: failing, foreign return, wrong SP or SS, code change, and
 * each tail output broken */
static void hook_contract(void)
{
    static const struct { int mode; pr_u3c6_0105_cpu_status want; } cases[] = {
        {H_FAIL, PR_U3C6_0105_CPU_CALLEE_FAILED},
        {H_TRANSFER, PR_U3C6_0105_CPU_CALLEE_TRANSFER},
        {H_SHORT_POP, PR_U3C6_0105_CPU_CALLEE_TRANSFER},
        {H_NEW_SS, PR_U3C6_0105_CPU_CALLEE_TRANSFER},
        {H_CODE_ENTRY, PR_U3C6_0105_CPU_CODE_CHANGED},
        {H_CODE_RES, PR_U3C6_0105_CPU_CODE_CHANGED},
        {H_CODE_INTR, PR_U3C6_0105_CPU_CODE_CHANGED},
        {H_BAD_AX, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_BAD_FLAGS, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_DF, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_BAD_BX, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_BAD_CX, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_BAD_DX, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_BAD_SI, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_BAD_DI, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_BAD_ES, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_BAD_DS, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
        {H_BAD_BP, PR_U3C6_0105_CPU_CALLEE_OUTPUT},
    };
    unsigned n;
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        pr_u3c6_0105_cpu c;
        hook_t h;
        pr_u3c6_0105_cpu_hooks hooks;
        unsigned tag = 400u + n;
        hook_init(&h, 0x0002u);
        h.mode = cases[n].mode;
        hooks = bounded(&h, DEPTH);
        fill(1u);
        enter_at(&c, SSEG, ENTRY_SP);
        CHECK(tag, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) == cases[n].want);
        CHECK(tag, h.calls == 1u && r8(DSEG, 0x4A03u) == 0x10u && r8(DSEG, 0x4A04u) == 0x00u);
        if (cases[n].mode == H_SHORT_POP) CHECK(tag, c.sp == 0x7FEEu && c.ip == 0x2A46u);
        if (cases[n].mode == H_TRANSFER) CHECK(tag, c.ip == 0x4321u);
        if (cases[n].want == PR_U3C6_0105_CPU_CALLEE_OUTPUT)
            CHECK(tag, c.cs == RSEG && c.ip == 0x2A46u && c.sp == 0x7FF0u);
    }
}

/* live argument reads: with DS:0x4A05 on [bp+8] and DS:0x4A03 on [bp+6], the
 * stores feed the later reads (value 0x10 takes the JAE path, 0x18 is read
 * back as the register) */
static void live_alias(void)
{
    const uint16_t ds = 0x335Fu;   /* DS:0x4A05 = SS:0x7FF5 = the first call's [bp+8] */
    pr_u3c6_0105_cpu c;
    hook_t h;
    pr_u3c6_0105_cpu_hooks hooks;
    unsigned n;
    hook_init(&h, 0x0002u);
    hooks = bounded(&h, DEPTH);
    cseg = 0x00F0u;                /* DS:0x4A02 = 0 lands on this CS word's zero high byte */
    fill(0u);
    enter_at(&c, SSEG, 0x7FF9u);
    c.ds = ds;
    w8(ds, 0x4A1Eu, 1u);
    w16(ds, 0x4A14u, 0x0246u);
    CHECK(500u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_RETURNED);
    CHECK(500u, h.calls == 2u && c.cs == RET_CS && c.ip == RET_IP && c.sp == 0x7FFDu && c.bp == 0x7777u);
    for (n = 0; n < 2u; n++) {
        CHECK(501u + n, h.sp[n] == 0x7FE3u && h.ds[n] == ds);
        CHECK(501u + n, h.regs[n][0] == 0x00u && h.regs[n][1] == 0x10u && h.regs[n][2] == 0x18u && h.regs[n][3] == 0x18u);
    }
    CHECK(503u, c.ax == 0x0246u && c.flags == 0x0246u);
    cseg = CSEG;
}

/* writes into the modelled code stop before them */
static void code_alias(void)
{
    pr_u3c6_0105_cpu c, keep;
    hook_t h;
    pr_u3c6_0105_cpu_hooks hooks;
    hook_init(&h, 0x0002u);
    hooks = bounded(&h, DEPTH);
    /* the entry PUSH BP onto [0105,0127): SS = CS, word at 0105..0106, and a straddle at 0104..0105 */
    fill(1u);
    enter_at(&c, CSEG, 0x0107u);
    keep = c;
    memcpy(before, ram, sizeof ram);
    CHECK(550u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_UNSCREENED_ALIAS);
    CHECK(550u, memcmp(&c, &keep, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    fill(1u);
    enter_at(&c, CSEG, 0x0106u);
    CHECK(551u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_UNSCREENED_ALIAS && c.ip == 0x0105u);
    /* the folded PUSH BP onto [2A0D,2A4A): SS = RSEG, entry SP 0x2A54 puts it at 2A48..2A49 */
    fill(1u);
    enter_at(&c, RSEG, 0x2A54u);
    w8(RSEG, 0x2A48u, 0xCCu);
    CHECK(552u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_UNSCREENED_ALIAS);
    CHECK(552u, c.cs == RSEG && c.ip == 0x2A0Du && c.sp == 0x2A4Au && r8(RSEG, 0x2A48u) == 0xCCu && h.calls == 0u);
    /* a byte store onto the folded body: DS:0x4A03 = RSEG:0x2A13 */
    fill(1u);
    enter_at(&c, SSEG, ENTRY_SP);
    c.ds = 0x0401u;
    w8(c.ds, 0x4A1Eu, 1u);
    CHECK(553u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_UNSCREENED_ALIAS);
    CHECK(553u, c.cs == RSEG && c.ip == 0x2A10u && r8(RSEG, 0x2A13u) == 0x00u && h.calls == 0u);
    /* the same store onto Intr's code: DS:0x4A03 = ISEG:0x0013 */
    fill(1u);
    enter_at(&c, SSEG, ENTRY_SP);
    c.ds = 0x0593u;
    w8(c.ds, 0x4A1Eu, 1u);
    CHECK(554u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_UNSCREENED_ALIAS);
    CHECK(554u, c.cs == RSEG && c.ip == 0x2A10u && r8(ISEG, 0x0013u) == 0x00u && h.calls == 0u);
}

/* RETF policy discriminators. (a) gate off, body RETF with the return IP at
 * SS:FFFD/FFFE/FFFF; (b) gate on, entry SP 0x0008 puts the folded RETF 4's
 * IP at SS:FFFE and its CS at SS:0000. Decoys sit at linear SS*16 + 0x10000
 * (FFFE) or + 0x10001 (FFFF). */
static void retf_wrap(void)
{
    static const uint16_t sps[3] = {0xFFFDu, 0xFFFEu, 0xFFFFu};
    unsigned k, p;
    for (k = 0; k < 3u; k++)
        for (p = 0; p < 2u; p++) {
            pr_u3c6_0105_cpu c;
            unsigned tag = 600u + 10u * k + p;
            uint16_t want_cs = RET_CS;
            fill(0u);
            enter_at(&c, SSEG, sps[k]);
            if (k == 1u) w16((uint16_t)(SSEG + 0x1000u), 0x0000u, 0xBEEFu);
            if (k == 2u) w16((uint16_t)(SSEG + 0x1000u), 0x0001u, 0xBEEFu);
            if (p == 1u && k != 0u) want_cs = 0xBEEFu;
            CHECK(tag, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c,
                                            p ? PR_U3C6_0105_RETF_CONTIGUOUS_PHYSICAL : PR_U3C6_0105_RETF_WRAPPED_SS,
                                            SYN, NULL) == PR_U3C6_0105_CPU_RETURNED);
            CHECK(tag, c.cs == want_cs && c.ip == RET_IP && c.sp == (uint16_t)(sps[k] + 4u));
        }
    for (p = 0; p < 2u; p++) {
        pr_u3c6_0105_cpu c;
        hook_t h;
        pr_u3c6_0105_cpu_hooks hooks;
        unsigned tag = 650u + p;
        pr_u3c6_0105_cpu_status st;
        hook_init(&h, 0x0002u);
        hooks = bounded(&h, DEPTH);
        fill(1u);
        enter_at(&c, SSEG, 0x0008u);
        w16((uint16_t)(SSEG + 0x1000u), 0x0000u, 0xBEEFu);
        st = pr_u3c6_0105_cpu_run(ram, sizeof ram, &c,
                                  p ? PR_U3C6_0105_RETF_CONTIGUOUS_PHYSICAL : PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks);
        if (p == 0u) {
            CHECK(tag, st == PR_U3C6_0105_CPU_RETURNED && h.calls == 2u && c.cs == RET_CS && c.sp == 0x000Cu);
        } else {
            CHECK(tag, st == PR_U3C6_0105_CPU_SYSTEM_TRANSFER && h.calls == 1u);
            CHECK(tag, c.cs == 0xBEEFu && c.ip == 0x011Au && c.sp == 0x0006u);
        }
    }
}

/* argument negatives leave CPU and RAM unchanged */
static void arguments(void)
{
    static const struct { uint16_t seg, off; uint8_t v; } bad[] = {
        {CSEG, 0x0115u, 0xE8u},  /* first lcall opcode */
        {CSEG, 0x0121u, 0x0Eu},  /* second lcall offset low byte */
        {CSEG, 0x0123u, 0x07u},  /* second lcall segment differs from the first */
        {RSEG, 0x2A41u, 0xE8u},  /* folded Intr lcall opcode */
        {RSEG, 0x2A42u, 0x0Cu},  /* folded Intr lcall offset */
    };
    pr_u3c6_0105_cpu c, keep;
    hook_t h;
    pr_u3c6_0105_cpu_hooks hooks;
    unsigned n;
    for (n = 0; n < sizeof bad / sizeof bad[0]; n++) {
        unsigned tag = 700u + n;
        fill(1u);
        w8(bad[n].seg, bad[n].off, bad[n].v);
        enter_at(&c, SSEG, ENTRY_SP);
        keep = c;
        memcpy(before, ram, sizeof ram);
        CHECK(tag, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, NULL) ==
                       PR_U3C6_0105_CPU_ARGUMENT);
        CHECK(tag, memcmp(&c, &keep, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    }
    fill(1u);
    enter_at(&c, SSEG, ENTRY_SP);
    keep = c;
    memcpy(before, ram, sizeof ram);
    hook_init(&h, 0x0002u);
    CHECK(710u, pr_u3c6_0105_cpu_run(NULL, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, NULL) ==
                    PR_U3C6_0105_CPU_ARGUMENT);
    CHECK(711u, pr_u3c6_0105_cpu_run(ram, sizeof ram - 1u, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, NULL) ==
                    PR_U3C6_0105_CPU_ARGUMENT);
    CHECK(712u, pr_u3c6_0105_cpu_run(ram, sizeof ram, NULL, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, NULL) ==
                    PR_U3C6_0105_CPU_ARGUMENT);
    CHECK(713u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, (pr_u3c6_0105_retf_policy)3, SYN, NULL) ==
                    PR_U3C6_0105_CPU_ARGUMENT);
    hooks = bounded(&h, (uint16_t)(PR_U3C6_0105_BIOS_DEPTH_MAX + 1u));
    CHECK(714u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_ARGUMENT);
    hooks = bounded(&h, DEPTH);
    hooks.intr_policy = (pr_u3c6_0105_intr_policy)2;
    CHECK(715u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, &hooks) ==
                    PR_U3C6_0105_CPU_ARGUMENT);
    CHECK(718u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS,
                                     (pr_u3c6_0105_code_policy)2, NULL) == PR_U3C6_0105_CPU_ARGUMENT);
    /* the default PINNED policy rejects the synthetic code before anything runs */
    hooks = bounded(&h, DEPTH);
    CHECK(719u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, PR_U3C6_0105_CODE_PINNED,
                                     &hooks) == PR_U3C6_0105_CPU_CODE_NOT_ORIGINAL);
    CHECK(720u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_UNMEASURED,
                                     (pr_u3c6_0105_code_policy)0, NULL) == PR_U3C6_0105_CPU_CODE_NOT_ORIGINAL);
    c.ip = 0x0106u;
    CHECK(716u, pr_u3c6_0105_cpu_run(ram, sizeof ram, &c, PR_U3C6_0105_RETF_WRAPPED_SS, SYN, NULL) ==
                    PR_U3C6_0105_CPU_ARGUMENT);
    c.ip = keep.ip;
    CHECK(717u, h.calls == 0u && memcmp(&c, &keep, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
}

int main(void)
{
    gate_on();
    gate_off();
    retf_unmeasured();
    intr_policy();
    hook_contract();
    live_alias();
    code_alias();
    retf_wrap();
    arguments();
    if (failures != 0u) {
        fprintf(stderr, "%u of %u checks failed\n", failures, checks);
        return 1;
    }
    printf("u3c6 +0105 cpu: %u checks passed\n", checks);
    return 0;
}
