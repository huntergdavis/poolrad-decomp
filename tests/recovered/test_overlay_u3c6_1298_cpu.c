/* U3C6 +1298 CPU/RAM model: frame, hook-contract, alias and RETF-policy
 * controls on synthetic state (no game bytes). Entry 10 (+0996) is a test
 * stub that checks what it receives and performs its own RETF 8. */
#include "poolrad/recovered/overlay_u3c6_1298_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSEG 0x2000u
#define DSEG 0x1000u
#define SSEG 0x3000u
#define MSEG 0x5000u
#define RET_CS 0x0BADu
#define RET_IP 0x1234u
#define ENTRY_SP 0x7FF4u /* SP on the return IP; 8 argument bytes above */
#define R_OFF 0x0100u    /* R = MSEG:R_OFF via DS:0x6716 */

static uint8_t ram[PR_U3C6_1298_CPU_RAM_SIZE], before[PR_U3C6_1298_CPU_RAM_SIZE];
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
    uint16_t q_seg, q_off;
    uint8_t a, b;
} args_t;

/* caller frame pushed word by word (byte-contiguous words, as PUSH does) */
static void enter_at(pr_u3c6_1298_cpu *c, const args_t *a, uint16_t entry_sp)
{
    const uint16_t words[6] = {a->q_seg, a->q_off, (uint16_t)(0x5A00u | a->b), (uint16_t)(0xA500u | a->a),
                               RET_CS, RET_IP};
    uint16_t sp = (uint16_t)(entry_sp + 12u);
    unsigned i;
    for (i = 0; i < 6; i++) {
        sp = (uint16_t)(sp - 2u);
        w16(SSEG, sp, words[i]);
    }
    memset(c, 0, sizeof *c);
    c->ax = 0x1111u; c->bx = 0x2222u; c->cx = 0x3333u; c->dx = 0x4444u;
    c->si = 0x5555u; c->di = 0x6666u; c->bp = 0x7777u; c->sp = sp;
    c->cs = CSEG; c->ip = 0x1298u; c->ds = DSEG; c->es = 0x9999u; c->ss = SSEG;
    c->flags = 0x0002u;
}

static uint32_t lcg;
static uint8_t rnd8(void) { lcg = lcg * 1103515245u + 12345u; return (uint8_t)(lcg >> 16); }

/* entry 15: first slot 1.. whose DS:0x65B9 far pointer equals Q, up to [0x5F2A] */
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
static uint16_t parity_pf(uint8_t v)
{
    unsigned i, ones = 0;
    for (i = 0; i < 8; i++) ones += (v >> i) & 1u;
    return (ones % 2u == 0u) ? 0x0004u : 0u;
}

static void tables(unsigned seed, args_t *a)
{
    unsigned j;
    uint8_t s = (uint8_t)(seed % 6u), count = (uint8_t)((seed / 6u) % 5u * 2u);
    memset(ram, 0, sizeof ram);
    lcg = seed * 2654435761u + 7u;
    for (j = 0; j < 0x10000u; j++) w8(DSEG, (uint16_t)j, rnd8());
    for (j = 0; j < 256u; j++) {
        w16(DSEG, (uint16_t)(0x65B9u + 4u * j), (uint16_t)(0x1100u + j));
        w16(DSEG, (uint16_t)(0x65BBu + 4u * j), 0x2222u);
    }
    a->q_seg = 0x5555u; a->q_off = 0x1234u;
    if (s != 0) {
        w16(DSEG, (uint16_t)(0x65B9u + 4u * s), a->q_off);
        w16(DSEG, (uint16_t)(0x65BBu + 4u * s), a->q_seg);
    }
    w8(DSEG, 0x5F2Au, count);
    w16(DSEG, 0x6716u, R_OFF);
    w16(DSEG, 0x6718u, MSEG);
    w8(MSEG, (uint16_t)(R_OFF + 4u), 0xC4u);
    w8(MSEG, (uint16_t)(R_OFF + 5u), 0xC5u);
    a->a = rnd8();
    a->b = rnd8();
}

enum { H_NORMAL, H_FAIL, H_TRANSFER, H_CODE, H_MOVE_R };
typedef struct {
    int mode;
    unsigned calls;
    uint16_t sp, ret_ip, ret_cs, w8_, wb, w13, w12, flags;
    uint8_t r4, r5;
    uint16_t out_flags;
} hook_t;

static int entry10_stub(void *context, uint8_t *mem, size_t size, pr_u3c6_1298_cpu *c)
{
    hook_t *h = context;
    uint16_t r_off = r16(DSEG, 0x6716u), r_seg = r16(DSEG, 0x6718u);
    (void)size;
    if (mem != ram || c->cs != CSEG || c->ip != 0x0996u) return 9;
    h->calls++;
    h->sp = c->sp;
    h->ret_ip = r16(c->ss, c->sp);
    h->ret_cs = r16(c->ss, (uint16_t)(c->sp + 2u));
    h->w8_ = r16(c->ss, (uint16_t)(c->sp + 4u));
    h->wb = r16(c->ss, (uint16_t)(c->sp + 6u));
    h->w13 = r16(c->ss, (uint16_t)(c->sp + 8u));
    h->w12 = r16(c->ss, (uint16_t)(c->sp + 10u));
    h->flags = c->flags;
    h->r4 = r8(r_seg, (uint16_t)(r_off + 4u));
    h->r5 = r8(r_seg, (uint16_t)(r_off + 5u));
    if (h->mode == H_FAIL) return 1;
    if (h->mode == H_CODE) w8(CSEG, 0x12F0u, (uint8_t)(r8(CSEG, 0x12F0u) ^ 0xFFu));
    if (h->mode == H_MOVE_R) {
        w16(DSEG, 0x6716u, 0x0200u);
        w8(MSEG, 0x0204u, 0x77u);
        w8(MSEG, 0x0205u, 0x77u);
    }
    /* the callee's own RETF 8 */
    c->ip = h->ret_ip;
    c->cs = h->ret_cs;
    c->sp = (uint16_t)(c->sp + 4u + 8u);
    if (h->mode == H_TRANSFER) c->ip = 0x4321u;
    c->ax = 0xDEADu; c->bx = 0xDEADu; c->cx = 0xDEADu; c->dx = 0xDEADu;
    c->si = 0xDEADu; c->di = 0xDEADu; c->bp = 0xBEEFu; c->es = 0xCAFEu;
    c->flags = h->out_flags;
    return 0;
}

static const pr_u3c6_1298_retf_policy synthetic[2] = {
    PR_U3C6_1298_RETF_WRAPPED_SS, PR_U3C6_1298_RETF_CONTIGUOUS_PHYSICAL};

static void reference_runs(void)
{
    unsigned n;
    for (n = 0; n < 2u * 120u; n++) {
        unsigned seed = 1u + n % 120u;
        pr_u3c6_1298_cpu c;
        args_t a;
        hook_t h;
        pr_u3c6_1298_cpu_hooks hooks;
        uint8_t s, e12, e13, t5;
        uint16_t want_flags;
        tables(seed, &a);
        s = ref_slot(&a);
        e12 = r8(DSEG, (uint16_t)(0x5F27u + 4u * s));
        e13 = r8(DSEG, (uint16_t)(0x5F28u + 4u * s));
        t5 = r8(DSEG, (uint16_t)(0x5F2Au + 4u * s));
        memset(&h, 0, sizeof h);
        h.mode = H_NORMAL;
        h.out_flags = (uint16_t)(0x0002u | 0x0891u);
        hooks.context = &h;
        hooks.entry10_0996 = entry10_stub;
        enter_at(&c, &a, ENTRY_SP);
        CHECK(seed, pr_u3c6_1298_cpu_run(ram, sizeof ram, &c, synthetic[n / 120u], &hooks) ==
                        PR_U3C6_1298_CPU_RETURNED);
        /* what +0996 received: frame, byte arguments as words, R, flags of +0C25 SHL */
        want_flags = (uint16_t)(0x0002u | 0x0010u | parity_pf((uint8_t)(4u * s)) | (s == 0u ? 0x0040u : 0u));
        CHECK(seed, h.calls == 1u && h.sp == (uint16_t)(ENTRY_SP - 0x0Eu));
        CHECK(seed, h.ret_ip == 0x12E5u && h.ret_cs == CSEG);
        CHECK(seed, h.w8_ == 0x0008u && h.wb == a.b && h.w13 == e13 && h.w12 == e12);
        CHECK(seed, h.r4 == a.a && h.r5 == t5);
        CHECK(seed, h.flags == want_flags);
        /* after the return */
        CHECK(seed, c.cs == RET_CS && c.ip == RET_IP);
        CHECK(seed, c.sp == (uint16_t)(ENTRY_SP + 4u + 8u) && c.bp == 0x7777u);
        CHECK(seed, c.flags == h.out_flags);                    /* LES/MOV/POP/RETF keep the hook's flags */
        CHECK(seed, r8(MSEG, (uint16_t)(R_OFF + 4u)) == 0u && r8(MSEG, (uint16_t)(R_OFF + 5u)) == 1u);
    }
}

/* the hook moves DS:0x6716; the +12E5/+12EE LES must follow it */
static void live_r_control(void)
{
    pr_u3c6_1298_cpu c;
    args_t a;
    hook_t h;
    pr_u3c6_1298_cpu_hooks hooks = {0, entry10_stub};
    uint8_t s, t5;
    tables(4u, &a);
    s = ref_slot(&a);
    t5 = r8(DSEG, (uint16_t)(0x5F2Au + 4u * s));
    memset(&h, 0, sizeof h);
    h.mode = H_MOVE_R;
    h.out_flags = 0x0002u;
    hooks.context = &h;
    enter_at(&c, &a, ENTRY_SP);
    CHECK(1000u, pr_u3c6_1298_cpu_run(ram, sizeof ram, &c, PR_U3C6_1298_RETF_WRAPPED_SS, &hooks) ==
                     PR_U3C6_1298_CPU_RETURNED);
    CHECK(1000u, r8(MSEG, 0x0204u) == 0u && r8(MSEG, 0x0205u) == 1u);      /* new R */
    CHECK(1000u, r8(MSEG, (uint16_t)(R_OFF + 4u)) == a.a && r8(MSEG, (uint16_t)(R_OFF + 5u)) == t5);
}

/* hook contract stops */
static void hook_controls(void)
{
    static const struct { int mode; int no_hook; pr_u3c6_1298_cpu_status want; } cases[] = {
        {H_NORMAL, 1, PR_U3C6_1298_CPU_NEED_HOOK},
        {H_FAIL, 0, PR_U3C6_1298_CPU_CALLEE_FAILED},
        {H_TRANSFER, 0, PR_U3C6_1298_CPU_CALLEE_TRANSFER},
        {H_CODE, 0, PR_U3C6_1298_CPU_CODE_CHANGED},
    };
    unsigned n;
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        pr_u3c6_1298_cpu c;
        args_t a;
        hook_t h;
        pr_u3c6_1298_cpu_hooks hooks = {0, entry10_stub};
        unsigned tag = 2000u + n;
        uint8_t s, t5;
        tables(5u, &a);
        s = ref_slot(&a);
        t5 = r8(DSEG, (uint16_t)(0x5F2Au + 4u * s));
        memset(&h, 0, sizeof h);
        h.mode = cases[n].mode;
        h.out_flags = 0x0002u;
        hooks.context = &h;
        enter_at(&c, &a, ENTRY_SP);
        CHECK(tag, pr_u3c6_1298_cpu_run(ram, sizeof ram, &c, PR_U3C6_1298_RETF_WRAPPED_SS,
                                        cases[n].no_hook ? NULL : &hooks) == cases[n].want);
        CHECK(tag, r8(MSEG, (uint16_t)(R_OFF + 4u)) == a.a && r8(MSEG, (uint16_t)(R_OFF + 5u)) == t5);
        if (cases[n].no_hook) {
            CHECK(tag, c.cs == CSEG && c.ip == 0x0996u && c.sp == (uint16_t)(ENTRY_SP - 0x0Eu));
            CHECK(tag, r16(SSEG, c.sp) == 0x12E5u && r16(SSEG, (uint16_t)(c.sp + 2u)) == CSEG);
        }
        if (cases[n].mode == H_TRANSFER) CHECK(tag, c.ip == 0x4321u && c.cs == CSEG);
    }
}

/* writes into modelled code stop before the instruction; neighbours do not */
static void code_alias_controls(void)
{
    static const struct { uint16_t r_off; uint16_t stop_ip; } cases[] = {
        {0x1294u, 0x12A2u}, /* R+4 = +1298, first body byte */
        {0x12F6u, 0x12A2u}, /* R+4 = +12FA, last body byte */
        {0x12F7u, 0},       /* R+4 = +12FB, after the body */
        {0x0BE3u, 0x12A2u}, /* R+4 = +0BE7 */
        {0x0BE2u, 0x12C0u}, /* R+4 = +0BE6 outside; R+5 = +0BE7 inside */
        {0x0CB4u, 0x12A2u}, /* R+4 = +0CB8, last byte of entry 15 */
        {0x0CB5u, 0},       /* R+4 = +0CB9, outside */
    };
    unsigned n;
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        pr_u3c6_1298_cpu c;
        args_t a;
        hook_t h;
        pr_u3c6_1298_cpu_hooks hooks = {0, entry10_stub};
        unsigned tag = 3000u + n;
        pr_u3c6_1298_cpu_status st;
        tables(6u, &a);
        w16(DSEG, 0x6716u, cases[n].r_off);
        w16(DSEG, 0x6718u, CSEG);
        memset(&h, 0, sizeof h);
        h.out_flags = 0x0002u;
        hooks.context = &h;
        enter_at(&c, &a, ENTRY_SP);
        memcpy(before, ram, sizeof ram);
        st = pr_u3c6_1298_cpu_run(ram, sizeof ram, &c, PR_U3C6_1298_RETF_WRAPPED_SS, &hooks);
        if (cases[n].stop_ip == 0u) {
            CHECK(tag, st == PR_U3C6_1298_CPU_RETURNED);
            continue;
        }
        CHECK(tag, st == PR_U3C6_1298_CPU_UNSCREENED_ALIAS);
        CHECK(tag, c.cs == CSEG && c.ip == cases[n].stop_ip);
        CHECK(tag, r8(CSEG, (uint16_t)(cases[n].r_off + (cases[n].stop_ip == 0x12A2u ? 4u : 5u))) ==
                       before[at(CSEG, (uint16_t)(cases[n].r_off + (cases[n].stop_ip == 0x12A2u ? 4u : 5u)))]);
        if (cases[n].stop_ip == 0x12A2u) CHECK(tag, c.sp == (uint16_t)(ENTRY_SP - 2u) && h.calls == 0u);
    }
}

/* UNMEASURED fails closed at the first RETF reached: entry 15's RETF 4
 * (+0CB6) called from +12AD, after its POP BP */
static void unmeasured_control(void)
{
    pr_u3c6_1298_cpu c;
    args_t a;
    uint8_t s;
    tables(7u, &a);
    s = ref_slot(&a);
    enter_at(&c, &a, ENTRY_SP);
    CHECK(4000u, pr_u3c6_1298_cpu_run(ram, sizeof ram, &c, PR_U3C6_1298_RETF_UNMEASURED, NULL) ==
                     PR_U3C6_1298_CPU_RETURN_PENDING);
    CHECK(4000u, c.cs == CSEG && c.ip == 0x0CB6u);
    CHECK(4000u, c.sp == (uint16_t)(ENTRY_SP - 0x0Au) && c.bp == (uint16_t)(ENTRY_SP - 2u));
    CHECK(4000u, r16(SSEG, c.sp) == 0x12B0u && r16(SSEG, (uint16_t)(c.sp + 2u)) == CSEG);
    CHECK(4000u, (uint8_t)c.ax == s);
    CHECK(4000u, r8(MSEG, (uint16_t)(R_OFF + 4u)) == a.a && r8(MSEG, (uint16_t)(R_OFF + 5u)) == 0xC5u);
}

/* WRAPPED_SS vs CONTIGUOUS_PHYSICAL: (a) entry SP 0x0008 puts entry 15's return
 * IP (from +12AD) at SS:FFFE and its CS at SS:0000; (b) entry SP 0xFFFE puts
 * the body's own return IP at SS:FFFE. A decoy sits at linear SS*16 + 0x10000. */
static void wrap_policy_controls(void)
{
    unsigned k, pol;
    for (k = 0; k < 2u; k++) {
        for (pol = 0; pol < 2u; pol++) {
            pr_u3c6_1298_cpu c;
            args_t a;
            hook_t h;
            pr_u3c6_1298_cpu_hooks hooks = {0, entry10_stub};
            unsigned tag = 5000u + 10u * k + pol;
            pr_u3c6_1298_cpu_status st;
            tables(8u, &a);
            memset(&h, 0, sizeof h);
            h.out_flags = 0x0002u;
            hooks.context = &h;
            enter_at(&c, &a, k == 0u ? 0x0008u : 0xFFFEu);
            w16((uint16_t)(SSEG + 0x1000u), 0x0000u, 0xBEEFu);
            st = pr_u3c6_1298_cpu_run(ram, sizeof ram, &c, synthetic[pol], &hooks);
            if (pol == 0u) {
                CHECK(tag, st == PR_U3C6_1298_CPU_RETURNED && c.cs == RET_CS && c.ip == RET_IP);
                CHECK(tag, r8(MSEG, (uint16_t)(R_OFF + 4u)) == 0u && r8(MSEG, (uint16_t)(R_OFF + 5u)) == 1u);
            } else if (k == 0u) {
                CHECK(tag, st == PR_U3C6_1298_CPU_RETURN_CHANGED);
                CHECK(tag, c.cs == 0xBEEFu && c.ip == 0x12B0u && c.sp == 0x0006u && h.calls == 0u);
            } else {
                CHECK(tag, st == PR_U3C6_1298_CPU_RETURNED && c.cs == 0xBEEFu && c.ip == RET_IP);
                CHECK(tag, c.sp == 0x000Au);
            }
        }
    }
}

static void argument_controls(void)
{
    pr_u3c6_1298_cpu c, keep;
    args_t a;
    tables(3u, &a);
    enter_at(&c, &a, ENTRY_SP);
    keep = c;
    memcpy(before, ram, sizeof ram);
    CHECK(6000u, pr_u3c6_1298_cpu_run(NULL, sizeof ram, &c, PR_U3C6_1298_RETF_WRAPPED_SS, NULL) ==
                     PR_U3C6_1298_CPU_ARGUMENT);
    CHECK(6001u, pr_u3c6_1298_cpu_run(ram, sizeof ram - 1u, &c, PR_U3C6_1298_RETF_WRAPPED_SS, NULL) ==
                     PR_U3C6_1298_CPU_ARGUMENT);
    CHECK(6002u, pr_u3c6_1298_cpu_run(ram, sizeof ram, NULL, PR_U3C6_1298_RETF_WRAPPED_SS, NULL) ==
                     PR_U3C6_1298_CPU_ARGUMENT);
    CHECK(6003u, pr_u3c6_1298_cpu_run(ram, sizeof ram, &c, (pr_u3c6_1298_retf_policy)3, NULL) ==
                     PR_U3C6_1298_CPU_ARGUMENT);
    c.ip = 0x1299u;
    CHECK(6004u, pr_u3c6_1298_cpu_run(ram, sizeof ram, &c, PR_U3C6_1298_RETF_WRAPPED_SS, NULL) ==
                     PR_U3C6_1298_CPU_ARGUMENT);
    c.ip = keep.ip;
    CHECK(6005u, memcmp(&c, &keep, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
}

int main(void)
{
    reference_runs();
    live_r_control();
    hook_controls();
    code_alias_controls();
    unmeasured_control();
    wrap_policy_controls();
    argument_controls();
    if (failures != 0u) {
        fprintf(stderr, "%u of %u checks failed\n", failures, checks);
        return 1;
    }
    printf("u3c6 +1298 cpu: %u checks passed\n", checks);
    return 0;
}
