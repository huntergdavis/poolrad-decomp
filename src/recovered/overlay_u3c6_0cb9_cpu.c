/* GAME.OVR unit 0x3C6 + 0x0CB9 (entry 16) on live CPU/RAM; see the header.
 * Comments name the original offsets: +0CB9..+0E0E for the body, +0C5F..+0CB6
 * (entry 15), +0000..+0065 (entry 1) and +04BE..+0528 (entry 6) for the
 * folded near callees. */
#include "poolrad/recovered/overlay_u3c6_0cb9_cpu.h"

#define MASK (PR_U3C6_0CB9_CPU_RAM_SIZE - 1u)
#define F_CF 0x0001u
#define F_PF 0x0004u
#define F_AF 0x0010u
#define F_ZF 0x0040u
#define F_SF 0x0080u
#define F_OF 0x0800u
#define ARITH (F_CF | F_PF | F_AF | F_ZF | F_SF | F_OF)
#define ALIAS PR_U3C6_0CB9_CPU_UNSCREENED_ALIAS
#define TRY(e)                       \
    do {                             \
        if ((e) != 0) return ALIAS;  \
    } while (0)

typedef pr_u3c6_0cb9_cpu cpu_t;
typedef pr_u3c6_0cb9_cpu_status status_t;
typedef struct {
    uint8_t *ram;
    cpu_t *c;
    uint16_t cs; /* the loaded code segment of the unit (entry CS) */
} ctx_t;

static size_t phys(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint8_t rd8(const ctx_t *x, uint16_t seg, uint16_t off) { return x->ram[phys(seg, off)]; }
static uint16_t rd16(const ctx_t *x, uint16_t seg, uint16_t off)
{
    size_t at = phys(seg, off);
    return (uint16_t)(x->ram[at] | ((uint16_t)x->ram[(at + 1u) & MASK] << 8));
}
/* the modelled original code: [0000,0068), [04BE,052B), [0C5F,0E11) */
static int in_code(const ctx_t *x, size_t p)
{
    static const uint16_t lo[3] = {0x0000u, 0x04BEu, 0x0C5Fu};
    static const uint16_t hi[3] = {0x0068u, 0x052Bu, 0x0E11u};
    int i;
    for (i = 0; i < 3; i++)
        if (((p - phys(x->cs, lo[i])) & MASK) < (size_t)(hi[i] - lo[i])) return 1;
    return 0;
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
static void retf(ctx_t *x, uint16_t n)
{
    cpu_t *c = x->c;
    c->ip = rd16(x, c->ss, c->sp);
    c->cs = rd16(x, c->ss, (uint16_t)(c->sp + 2u));
    c->sp = (uint16_t)(c->sp + 4u + n);
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
static uint16_t add16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint32_t wide = (uint32_t)a + b;
    uint16_t r = (uint16_t)wide, f = parity((uint8_t)r);
    if (wide > 0xFFFFu) f |= F_CF;
    if ((a ^ b ^ r) & 0x10u) f |= F_AF;
    if (r == 0u) f |= F_ZF;
    if (r & 0x8000u) f |= F_SF;
    if ((a ^ r) & (b ^ r) & 0x8000u) f |= F_OF;
    put_arith(c, f);
    return r;
}
/* INC r/m8: CF kept */
static uint8_t inc8(cpu_t *c, uint8_t a)
{
    uint8_t r = (uint8_t)(a + 1u);
    uint16_t f = (uint16_t)(parity(r) | (c->flags & F_CF));
    if ((a ^ 1u ^ r) & 0x10u) f |= F_AF;
    if (r == 0u) f |= F_ZF;
    if (r & 0x80u) f |= F_SF;
    if (r == 0x80u) f |= F_OF;
    put_arith(c, f);
    return r;
}
/* XOR/OR r8 result flags: CF = OF = 0, AF = 0 (undefined; convention) */
static void logic8(cpu_t *c, uint8_t r)
{
    uint16_t f = parity(r);
    if (r == 0u) f |= F_ZF;
    if (r & 0x80u) f |= F_SF;
    put_arith(c, f);
}
/* SHL r16 by n (1..15), pinned DOSBox-X lazy flags (flags.cpp t_SHLw):
 * CF = last bit out; OF = MSB(result) XOR MSB(operand), which equals
 * MSB(result) XOR CF only for n = 1; AF = 1 because n & 0x1F is non-zero */
static uint16_t shl16(cpu_t *c, uint16_t a, unsigned n)
{
    uint16_t r = (uint16_t)(a << n), f = (uint16_t)(parity((uint8_t)r) | F_AF);
    if ((a >> (16u - n)) & 1u) f |= F_CF;
    if (r == 0u) f |= F_ZF;
    if (r & 0x8000u) f |= F_SF;
    if ((r ^ a) & 0x8000u) f |= F_OF;
    put_arith(c, f);
    return r;
}
static int zf(const cpu_t *c) { return (c->flags & F_ZF) != 0u; }
static int cf(const cpu_t *c) { return (c->flags & F_CF) != 0u; }
static int jl(const cpu_t *c) { return ((c->flags & F_SF) != 0u) != ((c->flags & F_OF) != 0u); }
static int jg(const cpu_t *c) { return !zf(c) && !jl(c); }
static void cbw(cpu_t *c) { c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; }
/* MUL r16 on the pinned 386-type normal core (instructions.h MULW):
 * DX:AX = AX * src; ZF = (AX == 0); PF from all 32 product bits;
 * CF = OF = (DX != 0); SF and AF kept */
static void mul16(cpu_t *c, uint16_t src)
{
    uint32_t p = (uint32_t)c->ax * src;
    uint16_t f;
    c->ax = (uint16_t)p;
    c->dx = (uint16_t)(p >> 16);
    f = (uint16_t)(c->flags & (F_SF | F_AF));
    f |= parity((uint8_t)(p ^ (p >> 8) ^ (p >> 16) ^ (p >> 24)));
    if (c->ax == 0u) f |= F_ZF;
    if (c->dx != 0u) f |= F_CF | F_OF;
    put_arith(c, f);
}
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static void xor_ah(cpu_t *c)
{
    c->ax = (uint16_t)(c->ax & 0x00FFu);
    logic8(c, 0u);
}
/* LES di,[seg:off] in the pinned DOSBox-X normal-core profile: the offset word
 * at linear EA, the segment word at linear EA+2, minus 0x10000 when the
 * offset is above 0xFFFC (only offset 0xFFFD differs from seg:(off+2)). */
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
        (void)inc8(c, v);
    }
    set_al(c, rd8(x, c->ss, BP(-3)));                                    /* +0C74 mov al,[bp-3] */
    xor_ah(c);                                                           /* +0C77 xor ah,ah */
    c->di = c->ax;                                                       /* +0C79 mov di,ax */
    c->di = shl16(c, c->di, 1u);                                         /* +0C7B shl di,1 */
    c->di = shl16(c, c->di, 1u);                                         /* +0C7D shl di,1 */
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
    retf(x, 4u);                                                         /* +0CB6 retf 4 */
    return PR_U3C6_0CB9_CPU_RETURNED;
}

/* entry 1, +0000: the offset pair for direction [bp+0xE] of class [bp+0x10] */
static status_t entry1_0000(ctx_t *x)
{
    cpu_t *c = x->c;
    TRY(push16(x, c->bp, 0x0000u));                                      /* +0000 push bp */
    c->bp = c->sp;                                                       /* +0001 mov bp,sp */
    c->sp = sub16(c, c->sp, 2u);                                         /* +0003 sub sp,2 */
    TRY(put8(x, c->ss, BP(-1), 0u, 0x0006u));                            /* +0006 mov byte [bp-1],0 */
    cmp8(c, rd8(x, c->ss, BP(0x10)), 0u);                                /* +000A cmp byte [bp+0x10],0 */
    if (zf(c)) goto leave;                                               /* +000E jne +0012; +0010 jmp +005F */
    set_al(c, rd8(x, c->ss, BP(0x0E)));                                  /* +0012 mov al,[bp+0xE] */
    xor_ah(c);                                                           /* +0015 xor ah,ah */
    c->ax = shl16(c, c->ax, 1u);                                         /* +0017 shl ax,1 */
    c->dx = c->ax;                                                       /* +0019 mov dx,ax */
    set_al(c, rd8(x, c->ss, BP(0x10)));                                  /* +001B mov al,[bp+0x10] */
    xor_ah(c);                                                           /* +001E xor ah,ah */
    c->di = c->ax;                                                       /* +0020 mov di,ax */
    c->cx = (uint16_t)((c->cx & 0xFF00u) | 3u);                          /* +0022 mov cl,3 */
    c->di = shl16(c, c->di, 3u);                                         /* +0024 shl di,cl */
    c->di = add16(c, c->di, c->dx);                                      /* +0026 add di,dx */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x2844u)));               /* +0028 mov al,[di+0x2844] */
    les(x, c->ss, BP(0x0A));                                             /* +002C les di,[bp+0xA] */
    TRY(put8(x, c->es, c->di, (uint8_t)c->ax, 0x002Fu));                 /* +002F mov es:[di],al */
    set_al(c, rd8(x, c->ss, BP(0x0E)));                                  /* +0032 mov al,[bp+0xE] */
    xor_ah(c);                                                           /* +0035 xor ah,ah */
    c->ax = shl16(c, c->ax, 1u);                                         /* +0037 shl ax,1 */
    c->dx = c->ax;                                                       /* +0039 mov dx,ax */
    set_al(c, rd8(x, c->ss, BP(0x10)));                                  /* +003B mov al,[bp+0x10] */
    xor_ah(c);                                                           /* +003E xor ah,ah */
    c->di = c->ax;                                                       /* +0040 mov di,ax */
    c->cx = (uint16_t)((c->cx & 0xFF00u) | 3u);                          /* +0042 mov cl,3 */
    c->di = shl16(c, c->di, 3u);                                         /* +0044 shl di,cl */
    c->di = add16(c, c->di, c->dx);                                      /* +0046 add di,dx */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x2845u)));               /* +0048 mov al,[di+0x2845] */
    les(x, c->ss, BP(6));                                                /* +004C les di,[bp+6] */
    TRY(put8(x, c->es, c->di, (uint8_t)c->ax, 0x004Fu));                 /* +004F mov es:[di],al */
    les(x, c->ss, BP(0x0A));                                             /* +0052 les di,[bp+0xA] */
    cmp8(c, rd8(x, c->es, c->di), 0u);                                   /* +0055 cmp byte es:[di],0 */
    if (jl(c)) goto leave;                                               /* +0059 jl +005F */
    TRY(put8(x, c->ss, BP(-1), 1u, 0x005Bu));                            /* +005B mov byte [bp-1],1 */
leave:
    set_al(c, rd8(x, c->ss, BP(-1)));                                    /* +005F mov al,[bp-1] */
    c->sp = c->bp;                                                       /* +0062 mov sp,bp */
    c->bp = pop16(x);                                                    /* +0064 pop bp */
    retf(x, 0x0Cu);                                                      /* +0065 retf 0xC */
    return PR_U3C6_0CB9_CPU_RETURNED;
}

/* entry 6, +04BE: the two cell bytes at column [bp+0x10], row [bp+0xE] */
static status_t entry6_04be(ctx_t *x)
{
    cpu_t *c = x->c;
    TRY(push16(x, c->bp, 0x04BEu));                                      /* +04BE push bp */
    c->bp = c->sp;                                                       /* +04BF mov bp,sp */
    cmp8(c, rd8(x, c->ss, BP(0x10)), 0x00u);                             /* +04C1 cmp byte [bp+0x10],0 */
    if (jl(c)) goto out_of_range;                                        /* +04C5 jl +0519 */
    cmp8(c, rd8(x, c->ss, BP(0x10)), 0x31u);                             /* +04C7 cmp byte [bp+0x10],0x31 */
    if (jg(c)) goto out_of_range;                                        /* +04CB jg +0519 */
    cmp8(c, rd8(x, c->ss, BP(0x0E)), 0x00u);                             /* +04CD cmp byte [bp+0xE],0 */
    if (jl(c)) goto out_of_range;                                        /* +04D1 jl +0519 */
    cmp8(c, rd8(x, c->ss, BP(0x0E)), 0x18u);                             /* +04D3 cmp byte [bp+0xE],0x18 */
    if (jg(c)) goto out_of_range;                                        /* +04D7 jg +0519 */
    set_al(c, rd8(x, c->ss, BP(0x10)));                                  /* +04D9 mov al,[bp+0x10] */
    cbw(c);                                                              /* +04DC cbw */
    c->cx = c->ax;                                                       /* +04DD mov cx,ax */
    set_al(c, rd8(x, c->ss, BP(0x0E)));                                  /* +04DF mov al,[bp+0xE] */
    cbw(c);                                                              /* +04E2 cbw */
    c->dx = 0x32u;                                                       /* +04E3 mov dx,0x32 */
    mul16(c, c->dx);                                                     /* +04E6 mul dx */
    les(x, c->ds, 0x6716u);                                              /* +04E8 les di,[0x6716] */
    c->di = add16(c, c->di, c->ax);                                      /* +04EC add di,ax */
    c->di = add16(c, c->di, c->cx);                                      /* +04EE add di,cx */
    set_al(c, rd8(x, c->es, (uint16_t)(c->di + 7u)));                    /* +04F0 mov al,es:[di+7] */
    les(x, c->ss, BP(6));                                                /* +04F4 les di,[bp+6] */
    TRY(put8(x, c->es, c->di, (uint8_t)c->ax, 0x04F7u));                 /* +04F7 mov es:[di],al */
    set_al(c, rd8(x, c->ss, BP(0x10)));                                  /* +04FA mov al,[bp+0x10] */
    cbw(c);                                                              /* +04FD cbw */
    c->cx = c->ax;                                                       /* +04FE mov cx,ax */
    set_al(c, rd8(x, c->ss, BP(0x0E)));                                  /* +0500 mov al,[bp+0xE] */
    cbw(c);                                                              /* +0503 cbw */
    c->dx = 0x32u;                                                       /* +0504 mov dx,0x32 */
    mul16(c, c->dx);                                                     /* +0507 mul dx */
    c->di = c->ax;                                                       /* +0509 mov di,ax */
    c->di = add16(c, c->di, c->cx);                                      /* +050B add di,cx */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x60DBu)));               /* +050D mov al,[di+0x60DB] */
    les(x, c->ss, BP(0x0A));                                             /* +0511 les di,[bp+0xA] */
    TRY(put8(x, c->es, c->di, (uint8_t)c->ax, 0x0514u));                 /* +0514 mov es:[di],al */
    goto leave;                                                          /* +0517 jmp +0527 */
out_of_range:
    les(x, c->ss, BP(0x0A));                                             /* +0519 les di,[bp+0xA] */
    TRY(put8(x, c->es, c->di, 0u, 0x051Cu));                             /* +051C mov byte es:[di],0 */
    les(x, c->ss, BP(6));                                                /* +0520 les di,[bp+6] */
    TRY(put8(x, c->es, c->di, 0u, 0x0523u));                             /* +0523 mov byte es:[di],0 */
leave:
    c->bp = pop16(x);                                                    /* +0527 pop bp */
    retf(x, 0x0Cu);                                                      /* +0528 retf 0xC */
    return PR_U3C6_0CB9_CPU_RETURNED;
}

/* SHL DI,1 twice after MOV DI,AX: DI = 4 * AX with the flags of the second shift */
static void di_times4(cpu_t *c)
{
    c->di = c->ax;
    c->di = shl16(c, c->di, 1u);
    c->di = shl16(c, c->di, 1u);
}

pr_u3c6_0cb9_cpu_status pr_u3c6_0cb9_cpu_run(uint8_t *ram, size_t ram_size, pr_u3c6_0cb9_cpu *cpu)
{
    ctx_t ctx, *x = &ctx;
    cpu_t *c = cpu;
    status_t st;
    if (ram == NULL || c == NULL || ram_size != PR_U3C6_0CB9_CPU_RAM_SIZE || c->ip != 0x0CB9u)
        return PR_U3C6_0CB9_CPU_ARGUMENT;
    x->ram = ram;
    x->c = c;
    x->cs = c->cs;

    TRY(push16(x, c->bp, 0x0CB9u));                                      /* +0CB9 push bp */
    c->bp = c->sp;                                                       /* +0CBA mov bp,sp */
    c->sp = sub16(c, c->sp, 0x0Cu);                                      /* +0CBC sub sp,0xC */
    les(x, c->ss, BP(0x0A));                                             /* +0CBF les di,[bp+0xA] */
    TRY(put8(x, c->es, c->di, 0x00u, 0x0CC2u));                          /* +0CC2 mov byte es:[di],0 */
    les(x, c->ss, BP(6));                                                /* +0CC6 les di,[bp+6] */
    TRY(put8(x, c->es, c->di, 0x17u, 0x0CC9u));                          /* +0CC9 mov byte es:[di],0x17 */
    TRY(put8(x, c->ss, BP(-5), 0x01u, 0x0CCDu));                         /* +0CCD mov byte [bp-5],1 */
    TRY(push16(x, rd16(x, c->ss, BP(0x12)), 0x0CD1u));                   /* +0CD1 push word [bp+0x12] */
    TRY(push16(x, rd16(x, c->ss, BP(0x10)), 0x0CD4u));                   /* +0CD4 push word [bp+0x10] */
    TRY(call_near(x, 0x0CD7u, 0x0CD8u, 0x0CDBu, 0x0C5Fu));               /* +0CD7 push cs; +0CD8 call +0C5F */
    if ((st = entry15_0c5f(x)) != PR_U3C6_0CB9_CPU_RETURNED) return st;
    if (!back_at(x, 0x0CDBu)) return PR_U3C6_0CB9_CPU_RETURN_CHANGED;
    TRY(put8(x, c->ss, BP(-2), (uint8_t)c->ax, 0x0CDBu));                /* +0CDB mov [bp-2],al */
    set_al(c, rd8(x, c->ss, BP(-2)));                                    /* +0CDE mov al,[bp-2] */
    xor_ah(c);                                                           /* +0CE1 xor ah,ah */
    di_times4(c);                                                        /* +0CE3 mov di,ax; +0CE5/+0CE7 shl di,1 */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x5F27u)));               /* +0CE9 mov al,[di+0x5F27] */
    TRY(put8(x, c->ss, BP(-8), (uint8_t)c->ax, 0x0CEDu));                /* +0CED mov [bp-8],al */
    set_al(c, rd8(x, c->ss, BP(-2)));                                    /* +0CF0 mov al,[bp-2] */
    xor_ah(c);                                                           /* +0CF3 xor ah,ah */
    di_times4(c);                                                        /* +0CF5 mov di,ax; +0CF7/+0CF9 shl di,1 */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x5F28u)));               /* +0CFB mov al,[di+0x5F28] */
    TRY(put8(x, c->ss, BP(-9), (uint8_t)c->ax, 0x0CFFu));                /* +0CFF mov [bp-9],al */
    TRY(put8(x, c->ss, BP(-1), 0x00u, 0x0D02u));                         /* +0D02 mov byte [bp-1],0 */
    goto body;                                                           /* +0D06 jmp +0D0B */

next_i:
    {
        uint8_t v = rd8(x, c->ss, BP(-1));
        TRY(put8(x, c->ss, BP(-1), (uint8_t)(v + 1u), 0x0D08u));         /* +0D08 inc byte [bp-1] */
        (void)inc8(c, v);
    }
body:
    set_al(c, rd8(x, c->ss, BP(-2)));                                    /* +0D0B mov al,[bp-2] */
    xor_ah(c);                                                           /* +0D0E xor ah,ah */
    di_times4(c);                                                        /* +0D10 mov di,ax; +0D12/+0D14 shl di,1 */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x5F2Au)));               /* +0D16 mov al,[di+0x5F2A] */
    TRY(push16(x, c->ax, 0x0D1Au));                                      /* +0D1A push ax */
    set_al(c, rd8(x, c->ss, BP(-1)));                                    /* +0D1B mov al,[bp-1] */
    TRY(push16(x, c->ax, 0x0D1Eu));                                      /* +0D1E push ax */
    c->di = BP(-0x0A);                                                   /* +0D1F lea di,[bp-0xA] */
    TRY(push16(x, c->ss, 0x0D22u));                                      /* +0D22 push ss */
    TRY(push16(x, c->di, 0x0D23u));                                      /* +0D23 push di */
    c->di = BP(-0x0B);                                                   /* +0D24 lea di,[bp-0xB] */
    TRY(push16(x, c->ss, 0x0D27u));                                      /* +0D27 push ss */
    TRY(push16(x, c->di, 0x0D28u));                                      /* +0D28 push di */
    TRY(call_near(x, 0x0D29u, 0x0D2Au, 0x0D2Du, 0x0000u));               /* +0D29 push cs; +0D2A call +0000 */
    if ((st = entry1_0000(x)) != PR_U3C6_0CB9_CPU_RETURNED) return st;
    if (!back_at(x, 0x0D2Du)) return PR_U3C6_0CB9_CPU_RETURN_CHANGED;
    logic8(c, (uint8_t)c->ax);                                           /* +0D2D or al,al */
    if (zf(c)) goto test_i;                                              /* +0D2F jne +0D34; +0D31 jmp +0E02 */

    set_al(c, rd8(x, c->ss, BP(0x0E)));                                  /* +0D34 mov al,[bp+0xE] */
    xor_ah(c);                                                           /* +0D37 xor ah,ah */
    c->di = c->ax;                                                       /* +0D39 mov di,ax */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x2736u)));               /* +0D3B mov al,[di+0x2736] */
    cbw(c);                                                              /* +0D3F cbw */
    c->cx = c->ax;                                                       /* +0D40 mov cx,ax */
    set_al(c, rd8(x, c->ss, BP(-0x0A)));                                 /* +0D42 mov al,[bp-0xA] */
    cbw(c);                                                              /* +0D45 cbw */
    c->dx = c->ax;                                                       /* +0D46 mov dx,ax */
    set_al(c, rd8(x, c->ss, BP(-8)));                                    /* +0D48 mov al,[bp-8] */
    cbw(c);                                                              /* +0D4B cbw */
    c->ax = add16(c, c->ax, c->dx);                                      /* +0D4C add ax,dx */
    c->ax = add16(c, c->ax, c->cx);                                      /* +0D4E add ax,cx */
    TRY(put8(x, c->ss, BP(-6), (uint8_t)c->ax, 0x0D50u));                /* +0D50 mov [bp-6],al */
    set_al(c, rd8(x, c->ss, BP(0x0E)));                                  /* +0D53 mov al,[bp+0xE] */
    xor_ah(c);                                                           /* +0D56 xor ah,ah */
    c->di = c->ax;                                                       /* +0D58 mov di,ax */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x273Fu)));               /* +0D5A mov al,[di+0x273F] */
    cbw(c);                                                              /* +0D5E cbw */
    c->cx = c->ax;                                                       /* +0D5F mov cx,ax */
    set_al(c, rd8(x, c->ss, BP(-0x0B)));                                 /* +0D61 mov al,[bp-0xB] */
    cbw(c);                                                              /* +0D64 cbw */
    c->dx = c->ax;                                                       /* +0D65 mov dx,ax */
    set_al(c, rd8(x, c->ss, BP(-9)));                                    /* +0D67 mov al,[bp-9] */
    cbw(c);                                                              /* +0D6A cbw */
    c->ax = add16(c, c->ax, c->dx);                                      /* +0D6B add ax,dx */
    c->ax = add16(c, c->ax, c->cx);                                      /* +0D6D add ax,cx */
    TRY(put8(x, c->ss, BP(-7), (uint8_t)c->ax, 0x0D6Fu));                /* +0D6F mov [bp-7],al */
    set_al(c, rd8(x, c->ss, BP(-6)));                                    /* +0D72 mov al,[bp-6] */
    TRY(push16(x, c->ax, 0x0D75u));                                      /* +0D75 push ax */
    set_al(c, rd8(x, c->ss, BP(-7)));                                    /* +0D76 mov al,[bp-7] */
    TRY(push16(x, c->ax, 0x0D79u));                                      /* +0D79 push ax */
    c->di = BP(-3);                                                      /* +0D7A lea di,[bp-3] */
    TRY(push16(x, c->ss, 0x0D7Du));                                      /* +0D7D push ss */
    TRY(push16(x, c->di, 0x0D7Eu));                                      /* +0D7E push di */
    c->di = BP(-4);                                                      /* +0D7F lea di,[bp-4] */
    TRY(push16(x, c->ss, 0x0D82u));                                      /* +0D82 push ss */
    TRY(push16(x, c->di, 0x0D83u));                                      /* +0D83 push di */
    TRY(call_near(x, 0x0D84u, 0x0D85u, 0x0D88u, 0x04BEu));               /* +0D84 push cs; +0D85 call +04BE */
    if ((st = entry6_04be(x)) != PR_U3C6_0CB9_CPU_RETURNED) return st;
    if (!back_at(x, 0x0D88u)) return PR_U3C6_0CB9_CPU_RETURN_CHANGED;

    set_al(c, rd8(x, c->ss, BP(-3)));                                    /* +0D88 mov al,[bp-3] */
    cmp8(c, (uint8_t)c->ax, rd8(x, c->ss, BP(-2)));                      /* +0D8B cmp al,[bp-2] */
    if (!zf(c)) goto test_b;                                             /* +0D8E jne +0D94 */
    TRY(put8(x, c->ss, BP(-3), 0x00u, 0x0D90u));                         /* +0D90 mov byte [bp-3],0 */
test_b:
    cmp8(c, rd8(x, c->ss, BP(-3)), 0x00u);                               /* +0D94 cmp byte [bp-3],0 */
    if (cf(c) || zf(c)) goto test_a;                                     /* +0D98 jbe +0DA3 */
    set_al(c, rd8(x, c->ss, BP(-3)));                                    /* +0D9A mov al,[bp-3] */
    les(x, c->ss, BP(0x0A));                                             /* +0D9D les di,[bp+0xA] */
    TRY(put8(x, c->es, c->di, (uint8_t)c->ax, 0x0DA0u));                 /* +0DA0 mov es:[di],al */
test_a:
    cmp8(c, rd8(x, c->ss, BP(-4)), 0x00u);                               /* +0DA3 cmp byte [bp-4],0 */
    if (zf(c)) goto clear_a;                                             /* +0DA7 je +0DB2 */
    les(x, c->ss, BP(6));                                                /* +0DA9 les di,[bp+6] */
    cmp8(c, rd8(x, c->es, c->di), 0x00u);                                /* +0DAC cmp byte es:[di],0 */
    if (!zf(c)) goto test_1e;                                            /* +0DB0 jne +0DBB */
clear_a:
    les(x, c->ss, BP(6));                                                /* +0DB2 les di,[bp+6] */
    TRY(put8(x, c->es, c->di, 0x00u, 0x0DB5u));                          /* +0DB5 mov byte es:[di],0 */
    goto test_i;                                                         /* +0DB9 jmp +0E02 */
test_1e:
    cmp8(c, rd8(x, c->ss, BP(-4)), 0x1Eu);                               /* +0DBB cmp byte [bp-4],0x1E */
    if (zf(c)) goto set_1e;                                              /* +0DBF je +0DCA */
    les(x, c->ss, BP(6));                                                /* +0DC1 les di,[bp+6] */
    cmp8(c, rd8(x, c->es, c->di), 0x1Eu);                                /* +0DC4 cmp byte es:[di],0x1E */
    if (!zf(c)) goto rank;                                               /* +0DC8 jne +0DD3 */
set_1e:
    les(x, c->ss, BP(6));                                                /* +0DCA les di,[bp+6] */
    TRY(put8(x, c->es, c->di, 0x1Eu, 0x0DCDu));                          /* +0DCD mov byte es:[di],0x1E */
    goto test_i;                                                         /* +0DD1 jmp +0E02 */
rank:
    set_al(c, rd8(x, c->ss, BP(-4)));                                    /* +0DD3 mov al,[bp-4] */
    xor_ah(c);                                                           /* +0DD6 xor ah,ah */
    di_times4(c);                                                        /* +0DD8 mov di,ax; +0DDA/+0DDC shl di,1 */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x2744u)));               /* +0DDE mov al,[di+0x2744] */
    cmp8(c, (uint8_t)c->ax, rd8(x, c->ss, BP(-5)));                      /* +0DE2 cmp al,[bp-5] */
    if (cf(c)) goto test_i;                                              /* +0DE5 jb +0E02 */
    set_al(c, rd8(x, c->ss, BP(-4)));                                    /* +0DE7 mov al,[bp-4] */
    xor_ah(c);                                                           /* +0DEA xor ah,ah */
    di_times4(c);                                                        /* +0DEC mov di,ax; +0DEE/+0DF0 shl di,1 */
    set_al(c, rd8(x, c->ds, (uint16_t)(c->di + 0x2744u)));               /* +0DF2 mov al,[di+0x2744] */
    TRY(put8(x, c->ss, BP(-5), (uint8_t)c->ax, 0x0DF6u));                /* +0DF6 mov [bp-5],al */
    set_al(c, rd8(x, c->ss, BP(-4)));                                    /* +0DF9 mov al,[bp-4] */
    les(x, c->ss, BP(6));                                                /* +0DFC les di,[bp+6] */
    TRY(put8(x, c->es, c->di, (uint8_t)c->ax, 0x0DFFu));                 /* +0DFF mov es:[di],al */
test_i:
    cmp8(c, rd8(x, c->ss, BP(-1)), 0x03u);                               /* +0E02 cmp byte [bp-1],3 */
    if (!zf(c)) goto next_i;                                             /* +0E06 je +0E0B; +0E08 jmp +0D08 */

    c->sp = c->bp;                                                       /* +0E0B mov sp,bp */
    c->bp = pop16(x);                                                    /* +0E0D pop bp */
    retf(x, 0x0Eu);                                                      /* +0E0E retf 0xE */
    return PR_U3C6_0CB9_CPU_RETURNED;
}
