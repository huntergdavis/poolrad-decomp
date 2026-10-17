/* GAME.OVR unit 0x3C6 + 0x1298 (entry 19) on live CPU/RAM; see the header.
 * Comments name the original offsets: +1298..+12F8 for the body, +0C5F..+0CB6
 * (entry 15), +0BE7..+0C0C (entry 12) and +0C0F..+0C34 (entry 13) for the
 * folded near callees. Entry 10 (+0996) is the hook. */
#include "poolrad/recovered/overlay_u3c6_1298_cpu.h"

#define MASK (PR_U3C6_1298_CPU_RAM_SIZE - 1u)
#define F_CF 0x0001u
#define F_PF 0x0004u
#define F_AF 0x0010u
#define F_ZF 0x0040u
#define F_SF 0x0080u
#define F_OF 0x0800u
#define ARITH (F_CF | F_PF | F_AF | F_ZF | F_SF | F_OF)
#define ALIAS PR_U3C6_1298_CPU_UNSCREENED_ALIAS
#define TRY(e)                       \
    do {                             \
        if ((e) != 0) return ALIAS;  \
    } while (0)
#define NRANGES 3
#define SNAP_BYTES ((0x0C37u - 0x0BE7u) + (0x0CB9u - 0x0C5Fu) + (0x12FBu - 0x1298u))

typedef pr_u3c6_1298_cpu cpu_t;
typedef pr_u3c6_1298_cpu_status status_t;
typedef struct {
    uint8_t *ram;
    cpu_t *c;
    uint16_t cs; /* the loaded code segment of the unit (entry CS) */
    pr_u3c6_1298_retf_policy policy;
} ctx_t;

/* the modelled original code: [0BE7,0C37), [0C5F,0CB9), [1298,12FB) */
static const uint16_t code_lo[NRANGES] = {0x0BE7u, 0x0C5Fu, 0x1298u};
static const uint16_t code_hi[NRANGES] = {0x0C37u, 0x0CB9u, 0x12FBu};

static size_t phys(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint8_t rd8(const ctx_t *x, uint16_t seg, uint16_t off) { return x->ram[phys(seg, off)]; }
static uint16_t rd16(const ctx_t *x, uint16_t seg, uint16_t off)
{
    size_t at = phys(seg, off);
    return (uint16_t)(x->ram[at] | ((uint16_t)x->ram[(at + 1u) & MASK] << 8));
}
static int in_code(const ctx_t *x, size_t p)
{
    int i;
    for (i = 0; i < NRANGES; i++)
        if (((p - phys(x->cs, code_lo[i])) & MASK) < (size_t)(code_hi[i] - code_lo[i])) return 1;
    return 0;
}
static void snapshot(const ctx_t *x, uint8_t *out)
{
    size_t k = 0;
    int i;
    uint16_t off;
    for (i = 0; i < NRANGES; i++)
        for (off = code_lo[i]; off < code_hi[i]; off++) out[k++] = rd8(x, x->cs, off);
}
static int put8(ctx_t *x, uint16_t seg, uint16_t off, uint8_t v, uint16_t ip)
{
    size_t p = phys(seg, off);
    if (in_code(x, p)) {
        x->c->ip = ip;
        return -1;
    }
    x->ram[p] = v;
    return 0;
}
static int put16(ctx_t *x, uint16_t seg, uint16_t off, uint16_t v, uint16_t ip)
{
    size_t at = phys(seg, off), hi = (at + 1u) & MASK;
    if (in_code(x, at) || in_code(x, hi)) {
        x->c->ip = ip;
        return -1;
    }
    x->ram[at] = (uint8_t)v;
    x->ram[hi] = (uint8_t)(v >> 8);
    return 0;
}
static int push16(ctx_t *x, uint16_t v, uint16_t ip)
{
    if (put16(x, x->c->ss, (uint16_t)(x->c->sp - 2u), v, ip) != 0) return -1;
    x->c->sp = (uint16_t)(x->c->sp - 2u);
    return 0;
}
static uint16_t pop16(ctx_t *x)
{
    uint16_t v = rd16(x, x->c->ss, x->c->sp);
    x->c->sp = (uint16_t)(x->c->sp + 2u);
    return v;
}
/* RETF n at retf_ip under the declared policy (public +0000 contract):
 * UNMEASURED stops before it (IP = retf_ip, RETURN_PENDING); WRAPPED_SS reads
 * CS at SS:(SP+2) with the offset wrapped; CONTIGUOUS_PHYSICAL reads CS two
 * linear bytes above the IP word. The IP word is byte-contiguous in both. */
static status_t retf(ctx_t *x, uint16_t n, uint16_t retf_ip)
{
    cpu_t *c = x->c;
    size_t at = phys(c->ss, c->sp), cs_at;
    c->ip = retf_ip;
    if (x->policy == PR_U3C6_1298_RETF_UNMEASURED) return PR_U3C6_1298_CPU_RETURN_PENDING;
    cs_at = x->policy == PR_U3C6_1298_RETF_WRAPPED_SS ? phys(c->ss, (uint16_t)(c->sp + 2u))
                                                      : (at + 2u) & MASK;
    c->ip = (uint16_t)(x->ram[at] | ((uint16_t)x->ram[(at + 1u) & MASK] << 8));
    c->cs = (uint16_t)(x->ram[cs_at] | ((uint16_t)x->ram[(cs_at + 1u) & MASK] << 8));
    c->sp = (uint16_t)(c->sp + 4u + n);
    return PR_U3C6_1298_CPU_RETURNED;
}
/* PUSH CS at push_ip, then CALL near at call_ip (pushes ret); IP := target */
static int call_near(ctx_t *x, uint16_t push_ip, uint16_t call_ip, uint16_t ret, uint16_t target)
{
    if (push16(x, x->c->cs, push_ip) != 0) return -1;
    if (push16(x, ret, call_ip) != 0) return -1;
    x->c->ip = target;
    return 0;
}
static int back_at(const ctx_t *x, uint16_t ret) { return x->c->cs == x->cs && x->c->ip == ret; }

static uint16_t parity(uint8_t v)
{
    v ^= (uint8_t)(v >> 4);
    v ^= (uint8_t)(v >> 2);
    v ^= (uint8_t)(v >> 1);
    return (v & 1u) ? 0u : F_PF;
}
static void put_arith(cpu_t *c, uint16_t f) { c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f); }
static void cmp8(cpu_t *c, uint8_t a, uint8_t b)
{
    uint8_t r = (uint8_t)(a - b);
    uint16_t f = parity(r);
    if (a < b) f |= F_CF;
    if ((a ^ b ^ r) & 0x10u) f |= F_AF;
    if (r == 0u) f |= F_ZF;
    if (r & 0x80u) f |= F_SF;
    if ((a ^ b) & (a ^ r) & 0x80u) f |= F_OF;
    put_arith(c, f);
}
static uint16_t sub16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a - b), f = parity((uint8_t)r);
    if (a < b) f |= F_CF;
    if ((a ^ b ^ r) & 0x10u) f |= F_AF;
    if (r == 0u) f |= F_ZF;
    if (r & 0x8000u) f |= F_SF;
    if ((a ^ b) & (a ^ r) & 0x8000u) f |= F_OF;
    put_arith(c, f);
    return r;
}
static void cmp16(cpu_t *c, uint16_t a, uint16_t b) { (void)sub16(c, a, b); }
/* INC r/m8: CF kept */
static void inc8_flags(cpu_t *c, uint8_t a)
{
    uint8_t r = (uint8_t)(a + 1u);
    uint16_t f = (uint16_t)(parity(r) | (c->flags & F_CF));
    if ((a ^ 1u ^ r) & 0x10u) f |= F_AF;
    if (r == 0u) f |= F_ZF;
    if (r & 0x80u) f |= F_SF;
    if (r == 0x80u) f |= F_OF;
    put_arith(c, f);
}
/* SHL r16 by 1, pinned DOSBox-X lazy flags (flags.cpp t_SHLw): CF = bit out;
 * OF = MSB(result) XOR MSB(operand); AF = 1 (count non-zero) */
static uint16_t shl16_1(cpu_t *c, uint16_t a)
{
    uint16_t r = (uint16_t)(a << 1), f = (uint16_t)(parity((uint8_t)r) | F_AF);
    if (a & 0x8000u) f |= F_CF;
    if (r == 0u) f |= F_ZF;
    if (r & 0x8000u) f |= F_SF;
    if ((r ^ a) & 0x8000u) f |= F_OF;
    put_arith(c, f);
    return r;
}
static int zf(const cpu_t *c) { return (c->flags & F_ZF) != 0u; }
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
/* XOR AH,AH: CF = OF = 0, AF = 0 (flags.cpp: false for t_XOR*), ZF = PF = 1 */
static void xor_ah(cpu_t *c)
{
    c->ax = (uint16_t)(c->ax & 0x00FFu);
    put_arith(c, (uint16_t)(F_ZF | F_PF));
}
/* MOV DI,AX; SHL DI,1; SHL DI,1 */
static void di_times4(cpu_t *c)
{
    c->di = c->ax;
    c->di = shl16_1(c, c->di);
    c->di = shl16_1(c, c->di);
}
/* LES di,[seg:off] in the pinned DOSBox-X normal-core profile */
static void les(ctx_t *x, uint16_t seg, uint16_t off)
{
    size_t ea = phys(seg, off);
    size_t at = (ea + 2u - (off > 0xFFFCu ? 0x10000u : 0u)) & MASK;
    x->c->di = rd16(x, seg, off);
    x->c->es = (uint16_t)(x->ram[at] | ((uint16_t)x->ram[(at + 1u) & MASK] << 8));
}
#define BP(d) ((uint16_t)(c->bp + (uint16_t)(d)))

/* entry 15, +0C5F: the slot whose DS:0x65B9 far pointer equals the argument */
static status_t entry15_0c5f(ctx_t *x)
{
    cpu_t *c = x->c;
    TRY(push16(x, c->bp, 0x0C5Fu));                                      /* +0C5F push bp */
    c->bp = c->sp;                                                       /* +0C60 mov bp,sp */
    c->sp = sub16(c, c->sp, 4u);                                         /* +0C62 sub sp,4 */
    TRY(put8(x, c->ss, BP(-2), 0u, 0x0C65u));                            /* +0C65 mov byte [bp-2],0 */
    TRY(put8(x, c->ss, BP(-1), 0u, 0x0C69u));                            /* +0C69 mov byte [bp-1],0 */
    TRY(put8(x, c->ss, BP(-3), 0u, 0x0C6Du));                            /* +0C6D mov byte [bp-3],0 */
next:
    {
        uint8_t v = rd8(x, c->ss, BP(-3));
        TRY(put8(x, c->ss, BP(-3), (uint8_t)(v + 1u), 0x0C71u));         /* +0C71 inc byte [bp-3] */
        inc8_flags(c, v);
    }
    set_al(c, rd8(x, c->ss, BP(-3)));                                    /* +0C74 mov al,[bp-3] */
    xor_ah(c);                                                           /* +0C77 xor ah,ah */
    di_times4(c);                                                        /* +0C79 mov di,ax; +0C7B/+0C7D shl di,1 */
    c->ax = rd16(x, c->ds, (uint16_t)(c->di + 0x65B9u));                 /* +0C7F mov ax,[di+0x65B9] */
    c->dx = rd16(x, c->ds, (uint16_t)(c->di + 0x65BBu));                 /* +0C83 mov dx,[di+0x65BB] */
    cmp16(c, c->dx, rd16(x, c->ss, BP(8)));                              /* +0C87 cmp dx,[bp+8] */
    if (!zf(c)) goto test_found;                                         /* +0C8A jne +0C95 */
    cmp16(c, c->ax, rd16(x, c->ss, BP(6)));                              /* +0C8C cmp ax,[bp+6] */
    if (!zf(c)) goto test_found;                                         /* +0C8F jne +0C95 */
    TRY(put8(x, c->ss, BP(-2), 1u, 0x0C91u));                            /* +0C91 mov byte [bp-2],1 */
test_found:
    cmp8(c, rd8(x, c->ss, BP(-2)), 0u);                                  /* +0C95 cmp byte [bp-2],0 */
    if (!zf(c)) goto result;                                             /* +0C99 jne +0CA4 */
    set_al(c, rd8(x, c->ss, BP(-3)));                                    /* +0C9B mov al,[bp-3] */
    cmp8(c, (uint8_t)c->ax, rd8(x, c->ds, 0x5F2Au));                     /* +0C9E cmp al,[0x5F2A] */
    if (!zf(c)) goto next;                                               /* +0CA2 jne +0C71 */
result:
    cmp8(c, rd8(x, c->ss, BP(-2)), 0u);                                  /* +0CA4 cmp byte [bp-2],0 */
    if (zf(c)) goto leave;                                               /* +0CA8 je +0CB0 */
    set_al(c, rd8(x, c->ss, BP(-3)));                                    /* +0CAA mov al,[bp-3] */
    TRY(put8(x, c->ss, BP(-1), (uint8_t)c->ax, 0x0CADu));                /* +0CAD mov [bp-1],al */
leave:
    set_al(c, rd8(x, c->ss, BP(-1)));                                    /* +0CB0 mov al,[bp-1] */
    c->sp = c->bp;                                                       /* +0CB3 mov sp,bp */
    c->bp = pop16(x);                                                    /* +0CB5 pop bp */
    return retf(x, 4u, 0x0CB6u);                                         /* +0CB6 retf 4 */
}

/* entry 12, +0BE7: DS byte [0x5F27 + 4 * entry15(arg)] */
static status_t entry12_0be7(ctx_t *x)
{
    cpu_t *c = x->c;
    status_t st;
    TRY(push16(x, c->bp, 0x0BE7u));                                      /* +0BE7 push bp */
    c->bp = c->sp;                                                       /* +0BE8 mov bp,sp */
    c->sp = sub16(c, c->sp, 2u);                                         /* +0BEA sub sp,2 */
    TRY(push16(x, rd16(x, c->ss, BP(8)), 0x0BEDu));                      /* +0BED push word [bp+8] */
    TRY(push16(x, rd16(x, c->ss, BP(6)), 0x0BF0u));                      /* +0BF0 push word [bp+6] */
    TRY(call_near(x, 0x0BF3u, 0x0BF4u, 0x0BF7u, 0x0C5Fu));               /* +0BF3 push cs; +0BF4 call +0C5F */
    if ((st = entry15_0c5f(x)) != PR_U3C6_1298_CPU_RETURNED) return st;
    if (!back_at(x, 0x0BF7u)) return PR_U3C6_1298_CPU_RETURN_CHANGED;
    xor_ah(c);                                                           /* +0BF7 xor ah,ah */
    di_times4(c);                                                        /* +0BF9 mov di,ax; +0BFB/+0BFD shl di,1 */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x5F27u)));               /* +0BFF mov al,[di+0x5F27] */
    TRY(put8(x, c->ss, BP(-1), (uint8_t)c->ax, 0x0C03u));                /* +0C03 mov [bp-1],al */
    set_al(c, rd8(x, c->ss, BP(-1)));                                    /* +0C06 mov al,[bp-1] */
    c->sp = c->bp;                                                       /* +0C09 mov sp,bp */
    c->bp = pop16(x);                                                    /* +0C0B pop bp */
    return retf(x, 4u, 0x0C0Cu);                                         /* +0C0C retf 4 */
}

/* entry 13, +0C0F: DS byte [0x5F28 + 4 * entry15(arg)] */
static status_t entry13_0c0f(ctx_t *x)
{
    cpu_t *c = x->c;
    status_t st;
    TRY(push16(x, c->bp, 0x0C0Fu));                                      /* +0C0F push bp */
    c->bp = c->sp;                                                       /* +0C10 mov bp,sp */
    c->sp = sub16(c, c->sp, 2u);                                         /* +0C12 sub sp,2 */
    TRY(push16(x, rd16(x, c->ss, BP(8)), 0x0C15u));                      /* +0C15 push word [bp+8] */
    TRY(push16(x, rd16(x, c->ss, BP(6)), 0x0C18u));                      /* +0C18 push word [bp+6] */
    TRY(call_near(x, 0x0C1Bu, 0x0C1Cu, 0x0C1Fu, 0x0C5Fu));               /* +0C1B push cs; +0C1C call +0C5F */
    if ((st = entry15_0c5f(x)) != PR_U3C6_1298_CPU_RETURNED) return st;
    if (!back_at(x, 0x0C1Fu)) return PR_U3C6_1298_CPU_RETURN_CHANGED;
    xor_ah(c);                                                           /* +0C1F xor ah,ah */
    di_times4(c);                                                        /* +0C21 mov di,ax; +0C23/+0C25 shl di,1 */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x5F28u)));               /* +0C27 mov al,[di+0x5F28] */
    TRY(put8(x, c->ss, BP(-1), (uint8_t)c->ax, 0x0C2Bu));                /* +0C2B mov [bp-1],al */
    set_al(c, rd8(x, c->ss, BP(-1)));                                    /* +0C2E mov al,[bp-1] */
    c->sp = c->bp;                                                       /* +0C31 mov sp,bp */
    c->bp = pop16(x);                                                    /* +0C33 pop bp */
    return retf(x, 4u, 0x0C34u);                                         /* +0C34 retf 4 */
}

pr_u3c6_1298_cpu_status pr_u3c6_1298_cpu_run(uint8_t *ram, size_t ram_size, pr_u3c6_1298_cpu *cpu,
                                             pr_u3c6_1298_retf_policy retf_policy,
                                             const pr_u3c6_1298_cpu_hooks *hooks)
{
    ctx_t ctx, *x = &ctx;
    cpu_t *c = cpu;
    status_t st;
    uint8_t before[SNAP_BYTES], after[SNAP_BYTES];
    size_t k;
    if (ram == NULL || c == NULL || ram_size != PR_U3C6_1298_CPU_RAM_SIZE || c->ip != 0x1298u ||
        (retf_policy != PR_U3C6_1298_RETF_UNMEASURED && retf_policy != PR_U3C6_1298_RETF_WRAPPED_SS &&
         retf_policy != PR_U3C6_1298_RETF_CONTIGUOUS_PHYSICAL))
        return PR_U3C6_1298_CPU_ARGUMENT;
    x->ram = ram;
    x->c = c;
    x->cs = c->cs;
    x->policy = retf_policy;
    snapshot(x, before);

    TRY(push16(x, c->bp, 0x1298u));                                      /* +1298 push bp */
    c->bp = c->sp;                                                       /* +1299 mov bp,sp */
    set_al(c, rd8(x, c->ss, BP(6)));                                     /* +129B mov al,[bp+6] */
    les(x, c->ds, 0x6716u);                                              /* +129E les di,[0x6716] */
    TRY(put8(x, c->es, (uint16_t)(c->di + 4u), (uint8_t)c->ax, 0x12A2u)); /* +12A2 mov es:[di+4],al */
    TRY(push16(x, rd16(x, c->ss, BP(0x0C)), 0x12A6u));                   /* +12A6 push word [bp+0xC] */
    TRY(push16(x, rd16(x, c->ss, BP(0x0A)), 0x12A9u));                   /* +12A9 push word [bp+0xA] */
    TRY(call_near(x, 0x12ACu, 0x12ADu, 0x12B0u, 0x0C5Fu));               /* +12AC push cs; +12AD call +0C5F */
    if ((st = entry15_0c5f(x)) != PR_U3C6_1298_CPU_RETURNED) return st;
    if (!back_at(x, 0x12B0u)) return PR_U3C6_1298_CPU_RETURN_CHANGED;
    xor_ah(c);                                                           /* +12B0 xor ah,ah */
    di_times4(c);                                                        /* +12B2 mov di,ax; +12B4/+12B6 shl di,1 */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x5F2Au)));               /* +12B8 mov al,[di+0x5F2A] */
    les(x, c->ds, 0x6716u);                                              /* +12BC les di,[0x6716] */
    TRY(put8(x, c->es, (uint16_t)(c->di + 5u), (uint8_t)c->ax, 0x12C0u)); /* +12C0 mov es:[di+5],al */
    TRY(push16(x, rd16(x, c->ss, BP(0x0C)), 0x12C4u));                   /* +12C4 push word [bp+0xC] */
    TRY(push16(x, rd16(x, c->ss, BP(0x0A)), 0x12C7u));                   /* +12C7 push word [bp+0xA] */
    TRY(call_near(x, 0x12CAu, 0x12CBu, 0x12CEu, 0x0BE7u));               /* +12CA push cs; +12CB call +0BE7 */
    if ((st = entry12_0be7(x)) != PR_U3C6_1298_CPU_RETURNED) return st;
    if (!back_at(x, 0x12CEu)) return PR_U3C6_1298_CPU_RETURN_CHANGED;
    TRY(push16(x, c->ax, 0x12CEu));                                      /* +12CE push ax */
    TRY(push16(x, rd16(x, c->ss, BP(0x0C)), 0x12CFu));                   /* +12CF push word [bp+0xC] */
    TRY(push16(x, rd16(x, c->ss, BP(0x0A)), 0x12D2u));                   /* +12D2 push word [bp+0xA] */
    TRY(call_near(x, 0x12D5u, 0x12D6u, 0x12D9u, 0x0C0Fu));               /* +12D5 push cs; +12D6 call +0C0F */
    if ((st = entry13_0c0f(x)) != PR_U3C6_1298_CPU_RETURNED) return st;
    if (!back_at(x, 0x12D9u)) return PR_U3C6_1298_CPU_RETURN_CHANGED;
    TRY(push16(x, c->ax, 0x12D9u));                                      /* +12D9 push ax */
    set_al(c, rd8(x, c->ss, BP(8)));                                     /* +12DA mov al,[bp+8] */
    TRY(push16(x, c->ax, 0x12DDu));                                      /* +12DD push ax */
    set_al(c, 8u);                                                       /* +12DE mov al,8 */
    TRY(push16(x, c->ax, 0x12E0u));                                      /* +12E0 push ax */
    TRY(call_near(x, 0x12E1u, 0x12E2u, 0x12E5u, 0x0996u));               /* +12E1 push cs; +12E2 call +0996 */
    if (hooks == NULL || hooks->entry10_0996 == NULL) return PR_U3C6_1298_CPU_NEED_HOOK;
    if (hooks->entry10_0996(hooks->context, ram, ram_size, c) != 0) return PR_U3C6_1298_CPU_CALLEE_FAILED;
    if (!back_at(x, 0x12E5u)) return PR_U3C6_1298_CPU_CALLEE_TRANSFER;
    snapshot(x, after);
    for (k = 0; k < SNAP_BYTES; k++)
        if (after[k] != before[k]) return PR_U3C6_1298_CPU_CODE_CHANGED;

    les(x, c->ds, 0x6716u);                                              /* +12E5 les di,[0x6716] */
    TRY(put8(x, c->es, (uint16_t)(c->di + 4u), 0x00u, 0x12E9u));         /* +12E9 mov byte es:[di+4],0 */
    les(x, c->ds, 0x6716u);                                              /* +12EE les di,[0x6716] */
    TRY(put8(x, c->es, (uint16_t)(c->di + 5u), 0x01u, 0x12F2u));         /* +12F2 mov byte es:[di+5],1 */
    c->bp = pop16(x);                                                    /* +12F7 pop bp */
    return retf(x, 8u, 0x12F8u);                                         /* +12F8 retf 8 */
}
