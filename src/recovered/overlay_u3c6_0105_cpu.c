/* GAME.OVR unit 0x3C6 + 0x0105 (entry 3) on live CPU/RAM; see the header.
 * Comments name the original offsets: entry body (+0105..+0126) and the
 * folded resident 0x464 body (+2A0D..+2A47). */
#include "poolrad/recovered/overlay_u3c6_0105_cpu.h"

#define MASK (PR_U3C6_0105_CPU_RAM_SIZE - 1u)
#define F_CF 0x0001u
#define F_PF 0x0004u
#define F_AF 0x0010u
#define F_ZF 0x0040u
#define F_SF 0x0080u
#define F_OF 0x0800u
#define ARITH (F_CF | F_PF | F_AF | F_ZF | F_SF | F_OF)

typedef pr_u3c6_0105_cpu cpu_t;
typedef pr_u3c6_0105_cpu_status st_t;
#define OK PR_U3C6_0105_CPU_RETURNED

typedef struct {
    uint8_t *ram;
    cpu_t *c;
    const pr_u3c6_0105_cpu_hooks *h;
    uint16_t entry_cs, res_cs;
    uint8_t snap[0x22u + 0x3Du];
} M;

static size_t phys(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint8_t rd8(const M *m, uint16_t seg, uint16_t off) { return m->ram[phys(seg, off)]; }
static uint16_t rd16(const M *m, uint16_t seg, uint16_t off)
{
    size_t at = phys(seg, off);
    return (uint16_t)(m->ram[at] | ((uint16_t)m->ram[(at + 1u) & MASK] << 8));
}
/* the entry body [0105,0127) and the folded 0x464 body [2A0D,2A4A) */
static int aliased(const M *m, size_t p)
{
    return ((p - phys(m->entry_cs, 0x0105u)) & MASK) < 0x22u ||
           ((p - phys(m->res_cs, 0x2A0Du)) & MASK) < 0x3Du;
}
static st_t put8(M *m, uint16_t seg, uint16_t off, uint8_t v)
{
    size_t p = phys(seg, off);
    if (aliased(m, p)) return PR_U3C6_0105_CPU_UNSCREENED_ALIAS;
    m->ram[p] = v;
    return OK;
}
static st_t push16(M *m, uint16_t v)
{
    cpu_t *c = m->c;
    size_t at = phys(c->ss, (uint16_t)(c->sp - 2u)), hi = (at + 1u) & MASK;
    if (aliased(m, at) || aliased(m, hi)) return PR_U3C6_0105_CPU_UNSCREENED_ALIAS;
    m->ram[at] = (uint8_t)v;
    m->ram[hi] = (uint8_t)(v >> 8);
    c->sp = (uint16_t)(c->sp - 2u);
    return OK;
}
static uint16_t pop16(M *m)
{
    uint16_t v = rd16(m, m->c->ss, m->c->sp);
    m->c->sp = (uint16_t)(m->c->sp + 2u);
    return v;
}
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }

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
static void logic8_zero(cpu_t *c) { put_arith(c, (uint16_t)(F_ZF | F_PF)); }   /* XOR AH,AH: AF written 0 */
static int zf(const cpu_t *c) { return (c->flags & F_ZF) != 0u; }
static int cf(const cpu_t *c) { return (c->flags & F_CF) != 0u; }

static size_t snapshot(const M *m, uint8_t *out)
{
    size_t k = 0;
    for (uint16_t o = 0x0105u; o < 0x0127u; ++o) out[k++] = rd8(m, m->entry_cs, o);
    for (uint16_t o = 0x2A0Du; o < 0x2A4Au; ++o) out[k++] = rd8(m, m->res_cs, o);
    return k;
}

/* resident 0x464:+2A0D folded exactly; the far CALL at `site` returns to `ret` */
static st_t fold_2a0d(M *m, uint16_t site, uint16_t ret)
{
    cpu_t *c = m->c;
    uint16_t caller_cs = c->cs, intr_seg, intr_off;
    uint8_t v, now[sizeof m->snap];
    st_t st;
    if ((st = push16(m, c->cs)) != OK) { c->ip = site; return st; }          /* lcall: CS, IP */
    if ((st = push16(m, ret)) != OK) { c->ip = site; return st; }
    c->cs = m->res_cs;
    c->ip = 0x2A0Du;
    if ((st = push16(m, c->bp)) != OK) return st;                             /* +2A0D push bp */
    c->bp = c->sp;                                                            /* +2A0E mov bp,sp */
    c->ip = 0x2A10u;
    if ((st = put8(m, c->ds, 0x4A03u, 0x10u)) != OK) return st;               /* +2A10 mov byte [0x4A03],0x10 */
    c->ip = 0x2A15u;
    if ((st = put8(m, c->ds, 0x4A02u, 0x00u)) != OK) return st;               /* +2A15 mov byte [0x4A02],0 */
    v = rd8(m, c->ss, (uint16_t)(c->bp + 6u));
    cmp8(c, v, 8u);                                                           /* +2A1A cmp byte [bp+6],8 */
    if (cf(c)) {                                                              /* +2A1E jae +2A28 not taken */
        set_al(c, rd8(m, c->ss, (uint16_t)(c->bp + 6u)));                     /* +2A20 mov al,[bp+6] */
        c->ip = 0x2A23u;
        if ((st = put8(m, c->ds, 0x4A05u, (uint8_t)c->ax)) != OK) return st;  /* +2A23 mov [0x4A05],al */
    } else {                                                                  /* +2A26 jmp / +2A28 */
        set_al(c, rd8(m, c->ss, (uint16_t)(c->bp + 6u)));                     /* +2A28 mov al,[bp+6] */
        c->ax = (uint16_t)(c->ax & 0x00FFu);                                  /* +2A2B xor ah,ah */
        logic8_zero(c);
        c->ax = add16(c, c->ax, 8u);                                          /* +2A2D add ax,8 */
        c->ip = 0x2A30u;
        if ((st = put8(m, c->ds, 0x4A05u, (uint8_t)c->ax)) != OK) return st;  /* +2A30 mov [0x4A05],al */
    }
    set_al(c, rd8(m, c->ss, (uint16_t)(c->bp + 8u)));                         /* +2A33 mov al,[bp+8] */
    c->ip = 0x2A36u;
    if ((st = put8(m, c->ds, 0x4A04u, (uint8_t)c->ax)) != OK) return st;      /* +2A36 mov [0x4A04],al */
    set_al(c, 0x10u);                                                         /* +2A39 mov al,0x10 */
    c->ip = 0x2A3Bu;
    if ((st = push16(m, c->ax)) != OK) return st;                             /* +2A3B push ax */
    c->di = 0x4A02u;                                                          /* +2A3C mov di,0x4A02 */
    c->ip = 0x2A3Fu;
    if ((st = push16(m, c->ds)) != OK) return st;                             /* +2A3F push ds */
    c->ip = 0x2A40u;
    if ((st = push16(m, c->di)) != OK) return st;                             /* +2A40 push di */
    c->ip = 0x2A41u;                                                          /* +2A41 lcall 0xA32:000B */
    if (m->h == NULL || m->h->dos_intr_a32_000b == NULL) return PR_U3C6_0105_CPU_NEED_HOOK;
    intr_off = rd16(m, c->cs, 0x2A42u);
    intr_seg = rd16(m, c->cs, 0x2A44u);
    if ((st = push16(m, c->cs)) != OK) return st;
    if ((st = push16(m, 0x2A46u)) != OK) return st;
    c->cs = intr_seg;
    c->ip = intr_off;
    if (m->h->dos_intr_a32_000b(m->h->context, m->ram, PR_U3C6_0105_CPU_RAM_SIZE, c) != 0)
        return PR_U3C6_0105_CPU_CALLEE_FAILED;
    if (c->cs != m->res_cs || c->ip != 0x2A46u) return PR_U3C6_0105_CPU_CALLEE_TRANSFER;
    if (snapshot(m, now) != sizeof m->snap) return PR_U3C6_0105_CPU_CODE_CHANGED;
    for (size_t k = 0; k < sizeof m->snap; ++k)
        if (now[k] != m->snap[k]) return PR_U3C6_0105_CPU_CODE_CHANGED;
    c->bp = pop16(m);                                                         /* +2A46 pop bp */
    c->ip = pop16(m);                                                         /* +2A47 retf 4 */
    c->cs = pop16(m);
    c->sp = (uint16_t)(c->sp + 4u);
    return (c->cs == caller_cs && c->ip == ret) ? OK : PR_U3C6_0105_CPU_SYSTEM_TRANSFER;
}

pr_u3c6_0105_cpu_status pr_u3c6_0105_cpu_run(
    uint8_t *ram, size_t ram_size, pr_u3c6_0105_cpu *cpu, const pr_u3c6_0105_cpu_hooks *hooks)
{
    M m;
    cpu_t *c = cpu;
    st_t st;
    if (ram == NULL || c == NULL || ram_size != PR_U3C6_0105_CPU_RAM_SIZE || c->ip != 0x0105u)
        return PR_U3C6_0105_CPU_ARGUMENT;
    m.ram = ram;
    m.c = c;
    m.h = hooks;
    m.entry_cs = c->cs;
    if (rd8(&m, c->cs, 0x0115u) != 0x9Au || rd16(&m, c->cs, 0x0116u) != 0x2A0Du ||
        rd8(&m, c->cs, 0x0120u) != 0x9Au || rd16(&m, c->cs, 0x0121u) != 0x2A0Du ||
        rd16(&m, c->cs, 0x0118u) != rd16(&m, c->cs, 0x0123u))
        return PR_U3C6_0105_CPU_ARGUMENT;                                     /* both lcalls: same 0x464 target */
    m.res_cs = rd16(&m, c->cs, 0x0118u);
    if (rd8(&m, m.res_cs, 0x2A41u) != 0x9Au || rd16(&m, m.res_cs, 0x2A42u) != 0x000Bu)
        return PR_U3C6_0105_CPU_ARGUMENT;                                     /* folded body's Intr call */
    (void)snapshot(&m, m.snap);

    if ((st = push16(&m, c->bp)) != OK) return st;                            /* +0105 push bp */
    c->bp = c->sp;                                                            /* +0106 mov bp,sp */
    set_al(c, rd8(&m, c->ds, 0x4A1Eu));                                       /* +0108 mov al,[0x4A1E] */
    cmp8(c, (uint8_t)c->ax, 1u);                                              /* +010B cmp al,1 */
    if (zf(c)) {                                                              /* +010D jne +0125 */
        set_al(c, 0u);                                                        /* +010F mov al,0 */
        c->ip = 0x0111u;
        if ((st = push16(&m, c->ax)) != OK) return st;                        /* +0111 push ax (register 0) */
        set_al(c, 0u);                                                        /* +0112 mov al,0 */
        c->ip = 0x0114u;
        if ((st = push16(&m, c->ax)) != OK) return st;                        /* +0114 push ax (value 0) */
        if ((st = fold_2a0d(&m, 0x0115u, 0x011Au)) != OK) return st;          /* +0115 lcall 0x464:2A0D */
        set_al(c, 8u);                                                        /* +011A mov al,8 */
        c->ip = 0x011Cu;
        if ((st = push16(&m, c->ax)) != OK) return st;                        /* +011C push ax (register 8) */
        set_al(c, 8u);                                                        /* +011D mov al,8 */
        c->ip = 0x011Fu;
        if ((st = push16(&m, c->ax)) != OK) return st;                        /* +011F push ax (value 8) */
        if ((st = fold_2a0d(&m, 0x0120u, 0x0125u)) != OK) return st;          /* +0120 lcall 0x464:2A0D */
    }
    c->bp = pop16(&m);                                                        /* +0125 pop bp */
    c->ip = pop16(&m);                                                        /* +0126 retf */
    c->cs = pop16(&m);
    return OK;
}
