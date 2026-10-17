/* GAME.OVR unit 0x3B9 + 0x0708 (entry 7) on live CPU/RAM; see the header.
 * Comments name the original offsets: entry body (+0708..+07BD) and the
 * folded SYSTEM +0A3F copy. */
#include "poolrad/recovered/overlay_u3b9_entry7_0708_cpu.h"

#define MASK (PR_U3B9_0708_CPU_RAM_SIZE - 1u)
#define F_CF 0x0001u
#define F_PF 0x0004u
#define F_AF 0x0010u
#define F_ZF 0x0040u
#define F_SF 0x0080u
#define F_DF 0x0400u
#define F_OF 0x0800u
#define ARITH (F_CF | F_PF | F_AF | F_ZF | F_SF | F_OF)

typedef pr_u3b9_0708_cpu cpu_t;

/* ---- memory (A20-off physical, as the full-state precedents) ---- */
static size_t phys(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[phys(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[phys(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = phys(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = phys(seg, off);
    ram[at] = (uint8_t)v;
    ram[(at + 1u) & MASK] = (uint8_t)(v >> 8);
}
static void push16(uint8_t *ram, cpu_t *c, uint16_t v)
{
    c->sp = (uint16_t)(c->sp - 2u);
    wr16(ram, c->ss, c->sp, v);
}
static uint16_t pop16(const uint8_t *ram, cpu_t *c)
{
    uint16_t v = rd16(ram, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    return v;
}
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static void set_cl(cpu_t *c, uint8_t v) { c->cx = (uint16_t)((c->cx & 0xFF00u) | v); }
/* LES/LDS segment word in the pinned DOSBox-X normal-core profile: linear
 * EA+2, minus 0x10000 when the pointer's offset is above 0xFFFC (only offset
 * 0xFFFD differs from seg:(off+2)). */
static uint16_t les_seg(const uint8_t *ram, uint16_t seg, uint16_t ea_off)
{
    size_t at = (phys(seg, ea_off) + 2u - (ea_off > 0xFFFCu ? 0x10000u : 0u)) & MASK;
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

/* ---- flags ---- */
static uint16_t parity(uint8_t v)
{
    v ^= (uint8_t)(v >> 4);
    v ^= (uint8_t)(v >> 2);
    v ^= (uint8_t)(v >> 1);
    return (v & 1u) ? 0u : F_PF;
}
static void put_arith(cpu_t *c, uint16_t f) { c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f); }
static int cf(const cpu_t *c) { return (c->flags & F_CF) != 0u; }
static int zf(const cpu_t *c) { return (c->flags & F_ZF) != 0u; }
static uint16_t szp16(uint16_t r)
{
    uint16_t f = parity((uint8_t)r);
    if (r == 0u) f |= F_ZF;
    if (r & 0x8000u) f |= F_SF;
    return f;
}
static uint16_t add16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint32_t wide = (uint32_t)a + b;
    uint16_t r = (uint16_t)wide, f = szp16(r);
    if (wide > 0xFFFFu) f |= F_CF;
    if ((a ^ b ^ r) & 0x10u) f |= F_AF;
    if ((a ^ r) & (b ^ r) & 0x8000u) f |= F_OF;
    put_arith(c, f);
    return r;
}
static uint16_t dec16(cpu_t *c, uint16_t a)    /* DEC keeps CF */
{
    uint16_t r = (uint16_t)(a - 1u), f = szp16(r);
    if ((a ^ 1u ^ r) & 0x10u) f |= F_AF;
    if ((a ^ 1u) & (a ^ r) & 0x8000u) f |= F_OF;
    f |= (uint16_t)(c->flags & F_CF);
    put_arith(c, f);
    return r;
}
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
    uint16_t r = (uint16_t)(a - b), f = szp16(r);
    if (a < b) f |= F_CF;
    if ((a ^ b ^ r) & 0x10u) f |= F_AF;
    if ((a ^ b) & (a ^ r) & 0x8000u) f |= F_OF;
    put_arith(c, f);
    return r;
}
static void logic8(cpu_t *c, uint8_t r)          /* XOR/OR: CF = OF = 0, AF written 0 */
{
    uint16_t f = parity(r);
    if (r == 0u) f |= F_ZF;
    if (r & 0x80u) f |= F_SF;
    put_arith(c, f);
}
static void logic16(cpu_t *c, uint16_t r) { put_arith(c, szp16(r)); }
/* SHL r16, CL with CL = 3: CF = last bit out; OF = MSB(result) ^ CF and AF = 0 by convention */
static uint16_t shl16_3(cpu_t *c, uint16_t a)
{
    uint16_t r = (uint16_t)(a << 3), f = szp16(r);
    if (a & 0x2000u) f |= F_CF;
    if (((r & 0x8000u) != 0u) != ((a & 0x2000u) != 0u)) f |= F_OF;
    put_arith(c, f);
    return r;
}

/* ---- folded SYSTEM +0A3F: copy CX = [bx+4] bytes from [bx+0xA] to [bx+6] ---- */
static pr_u3b9_0708_cpu_status fold_copy(uint8_t *ram, cpu_t *c, uint16_t seg, uint16_t ret)
{
    uint16_t caller_cs = c->cs;
    push16(ram, c, c->cs);                                   /* lcall: return CS, IP */
    push16(ram, c, ret);
    c->cs = seg;
    c->ip = 0x0A3Fu;
    c->bx = c->sp;                                           /* +0A3F mov bx,sp */
    c->dx = c->ds;                                           /* +0A41 mov dx,ds */
    c->si = rd16(ram, c->ss, (uint16_t)(c->bx + 0x0Au));     /* +0A43 lds si,ss:[bx+0xA] */
    c->ds = les_seg(ram, c->ss, (uint16_t)(c->bx + 0x0Au));
    c->di = rd16(ram, c->ss, (uint16_t)(c->bx + 6u));        /* +0A47 les di,ss:[bx+6] */
    c->es = les_seg(ram, c->ss, (uint16_t)(c->bx + 6u));
    c->cx = rd16(ram, c->ss, (uint16_t)(c->bx + 4u));        /* +0A4B mov cx,ss:[bx+4] */
    c->flags = (uint16_t)(c->flags & (uint16_t)~F_DF);       /* +0A4F cld */
    while (c->cx != 0u) {                                    /* +0A50 rep movsb */
        wr8(ram, c->es, c->di, rd8(ram, c->ds, c->si));
        c->si = (uint16_t)(c->si + 1u);
        c->di = (uint16_t)(c->di + 1u);
        c->cx = (uint16_t)(c->cx - 1u);
    }
    c->ds = c->dx;                                           /* +0A52 mov ds,dx */
    c->ip = pop16(ram, c);                                   /* +0A54 retf 0xA */
    c->cs = pop16(ram, c);
    c->sp = (uint16_t)(c->sp + 0x0Au);
    return (c->ip == ret && c->cs == caller_cs) ? PR_U3B9_0708_CPU_RETURNED : PR_U3B9_0708_CPU_SYSTEM_TRANSFER;
}

/* ---- preconditions and screens ---- */
static const struct { uint16_t site, off; } CALLS[7] = {
    {0x071Cu, 0x0A3Fu}, {0x072Du, 0x00FFu}, {0x073Fu, 0x0919u}, {0x0744u, 0x084Au},
    {0x0749u, 0x0000u}, {0x07ACu, 0x0D71u}, {0x07B5u, 0x1B52u}};

typedef struct { uint16_t seg, start, end; } range_t;
#define N_RANGES 2
#define SNAP_MAX 256u
#define STACK_DEPTH 0x5Cu       /* deepest write below the entry SP (the 0x0D71 call frame) */

static int in_ranges(size_t p, const range_t *r, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        for (uint16_t o = r[i].start; o < r[i].end; ++o)
            if (phys(r[i].seg, o) == p) return 1;
    return 0;
}
static size_t snapshot(const uint8_t *ram, const range_t *r, size_t n, uint8_t *out)
{
    size_t k = 0;
    for (size_t i = 0; i < n; ++i)
        for (uint16_t o = r[i].start; o < r[i].end && k < SNAP_MAX; ++o)
            out[k++] = rd8(ram, r[i].seg, o);
    return k;
}

typedef struct {
    const range_t *r;
    size_t nr;
    const uint8_t *snap;
    size_t nsnap;
    uint16_t entry_cs;
} guard_t;

/* lcall at `site` to an external hook; the caller's pushes are already done */
static pr_u3b9_0708_cpu_status call_hook(uint8_t *ram, cpu_t *c, const pr_u3b9_0708_cpu_hooks *h,
                                         pr_u3b9_0708_cpu_callee fn, uint16_t site, uint16_t ret,
                                         const guard_t *g)
{
    uint8_t now[SNAP_MAX];
    c->ip = site;
    if (h == NULL || fn == NULL) return PR_U3B9_0708_CPU_NEED_HOOK;
    push16(ram, c, c->cs);
    push16(ram, c, ret);
    {
        uint16_t seg = rd16(ram, c->cs, (uint16_t)(site + 3u));
        uint16_t off = rd16(ram, c->cs, (uint16_t)(site + 1u));
        c->cs = seg;
        c->ip = off;
    }
    if (fn(h->context, ram, PR_U3B9_0708_CPU_RAM_SIZE, c) != 0) return PR_U3B9_0708_CPU_CALLEE_FAILED;
    if (c->cs != g->entry_cs || c->ip != ret) return PR_U3B9_0708_CPU_CALLEE_TRANSFER;
    if (snapshot(ram, g->r, g->nr, now) != g->nsnap) return PR_U3B9_0708_CPU_CODE_CHANGED;
    for (size_t k = 0; k < g->nsnap; ++k)
        if (now[k] != g->snap[k]) return PR_U3B9_0708_CPU_CODE_CHANGED;
    for (uint16_t k = 1u; k <= 0x20u; ++k)                   /* pushes still ahead */
        if (in_ranges(phys(c->ss, (uint16_t)(c->sp - k)), g->r, g->nr)) return PR_U3B9_0708_CPU_UNSCREENED_ALIAS;
    for (uint16_t k = 1u; k <= 0x46u; ++k)                   /* the temporary at BP-0x46/-0x44 */
        if (in_ranges(phys(c->ss, (uint16_t)(c->bp - k)), g->r, g->nr)) return PR_U3B9_0708_CPU_UNSCREENED_ALIAS;
    return PR_U3B9_0708_CPU_RETURNED;
}

/* +074E/+0761/+0793 shape: mov al,[bp+0xA]; xor ah,ah; mov di,ax; mov cl,3; shl di,cl */
static void index_to_di(const uint8_t *ram, cpu_t *c)
{
    set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Au)));
    c->ax = (uint16_t)(c->ax & 0x00FFu);
    logic8(c, 0u);
    c->di = c->ax;
    set_cl(c, 3u);
    c->di = shl16_3(c, c->di);
}

pr_u3b9_0708_cpu_status pr_u3b9_0708_cpu_run(
    uint8_t *ram, size_t ram_size, pr_u3b9_0708_cpu *cpu, const pr_u3b9_0708_cpu_hooks *hooks)
{
    range_t ranges[N_RANGES];
    uint8_t snap[SNAP_MAX];
    guard_t g;
    pr_u3b9_0708_cpu_status st;
    cpu_t *c = cpu;
    uint8_t v;
    int bad;

    if (ram == NULL || c == NULL || ram_size != PR_U3B9_0708_CPU_RAM_SIZE || c->ip != 0x0708u)
        return PR_U3B9_0708_CPU_ARGUMENT;
    for (int i = 0; i < 7; ++i)
        if (rd8(ram, c->cs, CALLS[i].site) != 0x9Au ||
            rd16(ram, c->cs, (uint16_t)(CALLS[i].site + 1u)) != CALLS[i].off)
            return PR_U3B9_0708_CPU_ARGUMENT;
    ranges[0] = (range_t){c->cs, 0x06E9u, 0x07C0u};                                   /* entry + pool */
    ranges[1] = (range_t){rd16(ram, c->cs, 0x071Fu), 0x0A3Fu, 0x0A57u};                /* folded copy */
    for (uint16_t k = 1u; k <= STACK_DEPTH; ++k)
        if (in_ranges(phys(c->ss, (uint16_t)(c->sp - k)), ranges, N_RANGES)) return PR_U3B9_0708_CPU_ARGUMENT;
    g.r = ranges;
    g.nr = N_RANGES;
    g.snap = snap;
    g.nsnap = snapshot(ram, ranges, N_RANGES, snap);
    g.entry_cs = c->cs;

    push16(ram, c, c->bp);                                   /* +0708 push bp */
    c->bp = c->sp;                                           /* +0709 mov bp,sp */
    c->sp = sub16(c, c->sp, 0x46u);                          /* +070B sub sp,0x46 */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));        /* +070E les di,[bp+6] */
    c->es = les_seg(ram, c->ss, (uint16_t)(c->bp + 6u));
    push16(ram, c, c->es);                                   /* +0711 push es */
    push16(ram, c, c->di);                                   /* +0712 push di */
    c->di = (uint16_t)(c->bp - 0x42u);                       /* +0713 lea di,[bp-0x42] */
    push16(ram, c, c->ss);                                   /* +0716 push ss */
    push16(ram, c, c->di);                                   /* +0717 push di */
    c->ax = 0x42u;                                           /* +0718 mov ax,0x42 */
    push16(ram, c, c->ax);                                   /* +071B push ax */
    st = fold_copy(ram, c, rd16(ram, c->cs, 0x071Fu), 0x0721u);   /* +071C lcall SYSTEM:0A3F */
    if (st != PR_U3B9_0708_CPU_RETURNED) return st;

    v = rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Au));
    cmp8(c, v, 1u);                                          /* +0721 cmp byte [bp+0xA],1 */
    if (cf(c)) {
        bad = 1;                                             /* +0725 jb +072D */
    } else {
        v = rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Au));
        cmp8(c, v, 3u);                                      /* +0727 cmp byte [bp+0xA],3 */
        bad = !(cf(c) || zf(c));                             /* +072B jbe +074E */
    }
    if (bad) {                                               /* +072D */
        st = call_hook(ram, c, hooks, hooks ? hooks->resident_0464_00ff : NULL, 0x072Du, 0x0732u, &g);
        if (st != PR_U3B9_0708_CPU_RETURNED) return st;      /* +072D lcall 0x464:00FF */
        c->di = 0x8608u;                                     /* +0732 mov di,0x8608 */
        push16(ram, c, c->ds);                               /* +0735 push ds */
        push16(ram, c, c->di);                               /* +0736 push di */
        c->di = 0x06E9u;                                     /* +0737 mov di,0x06E9 */
        push16(ram, c, c->cs);                               /* +073A push cs */
        push16(ram, c, c->di);                               /* +073B push di */
        c->ax = 0u;                                          /* +073C xor ax,ax */
        logic16(c, 0u);
        push16(ram, c, c->ax);                               /* +073E push ax (width 0) */
        st = call_hook(ram, c, hooks, hooks ? hooks->system_0919 : NULL, 0x073Fu, 0x0744u, &g);
        if (st != PR_U3B9_0708_CPU_RETURNED) return st;      /* +073F lcall SYSTEM:0919 */
        st = call_hook(ram, c, hooks, hooks ? hooks->system_084a : NULL, 0x0744u, 0x0749u, &g);
        if (st != PR_U3B9_0708_CPU_RETURNED) return st;      /* +0744 lcall SYSTEM:084A */
        st = call_hook(ram, c, hooks, hooks ? hooks->resident_0899_0000 : NULL, 0x0749u, 0x074Eu, &g);
        if (st != PR_U3B9_0708_CPU_RETURNED) return st;      /* +0749 lcall 0x899:0000 */
    }

    index_to_di(ram, c);                                     /* +074E..+0757 */
    c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + c->di - 0x44u));   /* +0759 mov ax,[bp+di-0x44] */
    c->ax = (uint16_t)(c->ax | rd16(ram, c->ss, (uint16_t)(c->bp + c->di - 0x42u)));
    logic16(c, c->ax);                                       /* +075C or ax,[bp+di-0x42] */
    if (!zf(c)) {                                            /* +075F je +07B3 */
        index_to_di(ram, c);                                 /* +0761..+076A */
        {
            uint16_t at = (uint16_t)(c->bp + c->di - 0x44u);
            c->di = rd16(ram, c->ss, at);                    /* +076C les di,[bp+di-0x44] */
            c->es = les_seg(ram, c->ss, at);
        }
        wr16(ram, c->ss, (uint16_t)(c->bp - 0x46u), c->di);  /* +076F mov [bp-0x46],di */
        wr16(ram, c->ss, (uint16_t)(c->bp - 0x44u), c->es);  /* +0772 mov [bp-0x44],es */
        c->di = rd16(ram, c->ss, (uint16_t)(c->bp - 0x46u)); /* +0775 les di,[bp-0x46] */
        c->es = les_seg(ram, c->ss, (uint16_t)(c->bp - 0x46u));
        c->ax = rd16(ram, c->es, (uint16_t)(c->di + 4u));    /* +0778 mov ax,es:[di+4] */
        c->ax = add16(c, c->ax, 3u);                         /* +077C add ax,3 */
        c->ax = dec16(c, c->ax);                             /* +077F dec ax */
        push16(ram, c, c->ax);                               /* +0780 push ax (x) */
        c->di = rd16(ram, c->ss, (uint16_t)(c->bp - 0x46u)); /* +0781 les di,[bp-0x46] */
        c->es = les_seg(ram, c->ss, (uint16_t)(c->bp - 0x46u));
        c->ax = rd16(ram, c->es, (uint16_t)(c->di + 6u));    /* +0784 mov ax,es:[di+6] */
        c->ax = add16(c, c->ax, 3u);                         /* +0788 add ax,3 */
        c->ax = dec16(c, c->ax);                             /* +078B dec ax */
        push16(ram, c, c->ax);                               /* +078C push ax (y) */
        set_al(c, 0u);                                       /* +078D mov al,0 */
        push16(ram, c, c->ax);                               /* +078F push ax (k) */
        set_al(c, 1u);                                       /* +0790 mov al,1 */
        push16(ram, c, c->ax);                               /* +0792 push ax (flags) */
        index_to_di(ram, c);                                 /* +0793..+079C */
        push16(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + c->di - 0x42u)));   /* +079E push [bp+di-0x42] */
        push16(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + c->di - 0x44u)));   /* +07A1 push [bp+di-0x44] */
        push16(ram, c, rd16(ram, c->ds, 0x4A22u));           /* +07A4 push [0x4A22] */
        push16(ram, c, rd16(ram, c->ds, 0x4A20u));           /* +07A8 push [0x4A20] */
        st = call_hook(ram, c, hooks, hooks ? hooks->resident_0464_0d71 : NULL, 0x07ACu, 0x07B1u, &g);
        if (st != PR_U3B9_0708_CPU_RETURNED) return st;      /* +07AC lcall 0x464:0D71 */
        st = call_hook(ram, c, hooks, hooks ? hooks->resident_0464_1b52 : NULL, 0x07B5u, 0x07BAu, &g);
        if (st != PR_U3B9_0708_CPU_RETURNED) return st;      /* +07B1 jmp +07B5; lcall 0x464:1B52 */
    }                                                        /* +07B3 jmp +07BA */
    c->sp = c->bp;                                           /* +07BA mov sp,bp */
    c->bp = pop16(ram, c);                                   /* +07BC pop bp */
    c->ip = pop16(ram, c);                                   /* +07BD retf 6 */
    c->cs = pop16(ram, c);
    c->sp = (uint16_t)(c->sp + 6u);
    return PR_U3B9_0708_CPU_RETURNED;
}
