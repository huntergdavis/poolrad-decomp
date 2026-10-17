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
#define F_DF 0x0400u
#define F_OF 0x0800u
#define ARITH (F_CF | F_PF | F_AF | F_ZF | F_SF | F_OF)

typedef pr_u3c6_0105_cpu cpu_t;
typedef pr_u3c6_0105_cpu_status st_t;
#define OK PR_U3C6_0105_CPU_RETURNED

typedef struct {
    uint8_t *ram;
    cpu_t *c;
    const pr_u3c6_0105_cpu_hooks *h;
    pr_u3c6_0105_retf_policy policy;
    uint16_t entry_cs, res_cs, intr_cs, entry_sp;
    uint8_t snap[0x22u + 0x3Du + 0x61u];
} M;

static size_t phys(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint8_t rd8(const M *m, uint16_t seg, uint16_t off) { return m->ram[phys(seg, off)]; }
static uint16_t rd16(const M *m, uint16_t seg, uint16_t off)
{
    size_t at = phys(seg, off);
    return (uint16_t)(m->ram[at] | ((uint16_t)m->ram[(at + 1u) & MASK] << 8));
}
/* the entry body [0105,0127), the folded 0x464 body [2A0D,2A4A) and Intr's
 * code 0xA32:[000B,006C) */
static int aliased(const M *m, size_t p)
{
    return ((p - phys(m->entry_cs, 0x0105u)) & MASK) < 0x22u ||
           ((p - phys(m->res_cs, 0x2A0Du)) & MASK) < 0x3Du ||
           ((p - phys(m->intr_cs, 0x000Bu)) & MASK) < 0x61u;
}
/* 64-bit FNV-1a of seg:[lo,hi), skipping the loader-fixup words at skip1 and
 * skip2 (0 = none) */
static uint64_t fnv1a(const M *m, uint16_t seg, uint16_t lo, uint16_t hi, uint16_t skip1, uint16_t skip2)
{
    uint64_t h = 0xCBF29CE484222325u;
    for (uint16_t o = lo; o < hi; ++o) {
        if ((skip1 != 0u && (o == skip1 || o == skip1 + 1u)) || (skip2 != 0u && (o == skip2 || o == skip2 + 1u)))
            continue;
        h = (h ^ rd8(m, seg, o)) * 0x100000001B3u;
    }
    return h;
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
/* RETF n at retf_ip under the declared policy (public +0000 contract):
 * UNMEASURED stops before it (IP = retf_ip, RETURN_PENDING); WRAPPED_SS reads
 * CS at SS:(SP+2) with the offset wrapped; CONTIGUOUS_PHYSICAL reads CS two
 * linear bytes above the IP word. The IP word is byte-contiguous in both. */
static st_t retf(M *m, uint16_t n, uint16_t retf_ip)
{
    cpu_t *c = m->c;
    size_t at = phys(c->ss, c->sp), cs_at;
    c->ip = retf_ip;
    if (m->policy == PR_U3C6_0105_RETF_UNMEASURED) return PR_U3C6_0105_CPU_RETURN_PENDING;
    cs_at = m->policy == PR_U3C6_0105_RETF_WRAPPED_SS ? phys(c->ss, (uint16_t)(c->sp + 2u))
                                                      : (at + 2u) & MASK;
    c->ip = (uint16_t)(m->ram[at] | ((uint16_t)m->ram[(at + 1u) & MASK] << 8));
    c->cs = (uint16_t)(m->ram[cs_at] | ((uint16_t)m->ram[(cs_at + 1u) & MASK] << 8));
    c->sp = (uint16_t)(c->sp + 4u + n);
    return OK;
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
    for (uint16_t o = 0x000Bu; o < 0x006Cu; ++o) out[k++] = rd8(m, m->intr_cs, o);
    return k;
}

/* The CPU outputs Intr's tail (0xA32:+0045..+006B, after the interrupt returns
 * through 0xA32:0045) leaves: PUSHF/ES/DI/BP, LES DI to the pushed record, CLD,
 * STOSW of AX BX CX DX BP SI DI DS ES Flags, POP DS, POP BP, RETF 6. So DS and
 * BP are restored, ES:DI = record + 0x14, AX = the stored Flags word, FLAGS =
 * that word with DF clear, and BX CX DX SI equal their stored record words. */
static int intr_outputs(const M *m, uint16_t rec_seg, uint16_t bp)
{
    const cpu_t *c = m->c;
    return c->ds == rec_seg && c->bp == bp && c->es == rec_seg && c->di == 0x4A16u &&
           c->ax == rd16(m, rec_seg, 0x4A14u) && c->flags == (uint16_t)(c->ax & ~F_DF) &&
           c->bx == rd16(m, rec_seg, 0x4A04u) && c->cx == rd16(m, rec_seg, 0x4A06u) &&
           c->dx == rd16(m, rec_seg, 0x4A08u) && c->si == rd16(m, rec_seg, 0x4A0Cu);
}

/* At +2A41 before the far call: the live argument words are DI = 0x4A02, DS
 * and int 0x10, and the record DS:[0x4A02,0x4A16) misses the bytes the call
 * and its arguments own, SS:[S, S+10) with S = SP - 4 */
static int intr_frame_ok(const M *m)
{
    const cpu_t *c = m->c;
    uint16_t s = (uint16_t)(c->sp - 4u);
    size_t rec = phys(c->ds, 0x4A02u);
    if (rd16(m, c->ss, c->sp) != 0x4A02u || rd16(m, c->ss, (uint16_t)(c->sp + 2u)) != c->ds ||
        rd8(m, c->ss, (uint16_t)(c->sp + 4u)) != 0x10u)
        return 0;
    for (uint16_t k = 0; k < 10u; ++k)
        if (((phys(c->ss, (uint16_t)(s + k)) - rec) & MASK) < 0x14u) return 0;
    return 1;
}

/* INTR_HOOK_BOUNDED: the record against the code, and SS:[s-20-depth, s)
 * against the code, the record, the vector IVT[0x10] and the live frame
 * SS:[s, entry SP + 4) */
static int intr_region_clear(const M *m, uint16_t s, uint16_t rec_seg)
{
    const cpu_t *c = m->c;
    uint32_t k, n = 20u + m->h->bios_depth;
    size_t rec = phys(rec_seg, 0x4A02u);
    uint16_t frame = (uint16_t)(m->entry_sp + 4u - s);
    for (k = 0; k < 0x14u; ++k)
        if (aliased(m, (rec + k) & MASK)) return 0;
    for (k = 1; k <= n; ++k) {
        uint16_t off = (uint16_t)(s - k);
        size_t p = phys(c->ss, off);
        if (aliased(m, p) || ((p - rec) & MASK) < 0x14u || (p >= 0x40u && p < 0x44u) ||
            (uint16_t)(off - s) < frame)
            return 0;
    }
    return 1;
}

/* resident 0x464:+2A0D folded exactly; the far CALL at `site` returns to `ret` */
static st_t fold_2a0d(M *m, uint16_t site, uint16_t ret)
{
    cpu_t *c = m->c;
    uint16_t caller_cs = c->cs, intr_seg, intr_off, hook_ss, hook_sp, hook_bp, rec_seg;
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
    if (m->h == NULL || m->h->intr_policy == PR_U3C6_0105_INTR_UNMEASURED) return PR_U3C6_0105_CPU_INTR_PENDING;
    if (m->h->dos_intr_a32_000b == NULL) return PR_U3C6_0105_CPU_NEED_HOOK;
    if (!intr_frame_ok(m)) return PR_U3C6_0105_CPU_INTR_FRAME;
    if (!intr_region_clear(m, (uint16_t)(c->sp - 4u), c->ds)) return PR_U3C6_0105_CPU_INTR_STACK_ALIAS;
    intr_off = rd16(m, c->cs, 0x2A42u);
    intr_seg = rd16(m, c->cs, 0x2A44u);                                       /* = m->intr_cs: code unchanged */
    if ((st = push16(m, c->cs)) != OK) return st;
    if ((st = push16(m, 0x2A46u)) != OK) return st;
    c->cs = intr_seg;
    c->ip = intr_off;
    hook_ss = c->ss;
    hook_sp = c->sp;
    hook_bp = c->bp;
    rec_seg = c->ds;                                                          /* the pushed record DS:0x4A02 */
    if (m->h->dos_intr_a32_000b(m->h->context, m->ram, PR_U3C6_0105_CPU_RAM_SIZE, c) != 0)
        return PR_U3C6_0105_CPU_CALLEE_FAILED;
    if (c->cs != m->res_cs || c->ip != 0x2A46u) return PR_U3C6_0105_CPU_CALLEE_TRANSFER;
    /* Intr's RETF 6 pops the return frame and its two arguments (SS kept) */
    if (c->ss != hook_ss || c->sp != (uint16_t)(hook_sp + 10u)) return PR_U3C6_0105_CPU_CALLEE_TRANSFER;
    if (snapshot(m, now) != sizeof m->snap) return PR_U3C6_0105_CPU_CODE_CHANGED;
    for (size_t k = 0; k < sizeof m->snap; ++k)
        if (now[k] != m->snap[k]) return PR_U3C6_0105_CPU_CODE_CHANGED;
    if (!intr_outputs(m, rec_seg, hook_bp)) return PR_U3C6_0105_CPU_CALLEE_OUTPUT;
    c->bp = pop16(m);                                                         /* +2A46 pop bp */
    if ((st = retf(m, 4u, 0x2A47u)) != OK) return st;                         /* +2A47 retf 4 */
    return (c->cs == caller_cs && c->ip == ret) ? OK : PR_U3C6_0105_CPU_SYSTEM_TRANSFER;
}

pr_u3c6_0105_cpu_status pr_u3c6_0105_cpu_run(
    uint8_t *ram, size_t ram_size, pr_u3c6_0105_cpu *cpu, pr_u3c6_0105_retf_policy retf_policy,
    pr_u3c6_0105_code_policy code_policy, const pr_u3c6_0105_cpu_hooks *hooks)
{
    M m;
    cpu_t *c = cpu;
    st_t st;
    if (ram == NULL || c == NULL || ram_size != PR_U3C6_0105_CPU_RAM_SIZE || c->ip != 0x0105u ||
        (retf_policy != PR_U3C6_0105_RETF_UNMEASURED && retf_policy != PR_U3C6_0105_RETF_WRAPPED_SS &&
         retf_policy != PR_U3C6_0105_RETF_CONTIGUOUS_PHYSICAL) ||
        (code_policy != PR_U3C6_0105_CODE_PINNED && code_policy != PR_U3C6_0105_CODE_SYNTHETIC) ||
        (hooks != NULL && ((hooks->intr_policy != PR_U3C6_0105_INTR_UNMEASURED &&
                            hooks->intr_policy != PR_U3C6_0105_INTR_HOOK_BOUNDED) ||
                           hooks->bios_depth > PR_U3C6_0105_BIOS_DEPTH_MAX)))
        return PR_U3C6_0105_CPU_ARGUMENT;
    m.ram = ram;
    m.c = c;
    m.h = hooks;
    m.policy = retf_policy;
    m.entry_cs = c->cs;
    m.entry_sp = c->sp;
    m.res_cs = rd16(&m, c->cs, 0x0118u);
    m.intr_cs = rd16(&m, m.res_cs, 0x2A44u);
    if (code_policy == PR_U3C6_0105_CODE_PINNED) {
        /* every code byte against the original, the fixup words by relation */
        if (fnv1a(&m, c->cs, 0x0105u, 0x0127u, 0x0118u, 0x0123u) != 0x9C090982BB5E2A78u ||
            rd16(&m, c->cs, 0x0123u) != m.res_cs ||
            fnv1a(&m, m.res_cs, 0x2A0Du, 0x2A4Au, 0x2A44u, 0u) != 0xC7A7A486D871566Fu ||
            (uint16_t)(m.intr_cs - m.res_cs) != (uint16_t)(0x0A32u - 0x0464u) ||
            fnv1a(&m, m.intr_cs, 0x000Bu, 0x006Cu, 0u, 0u) != 0xAB4C07EEA7BD0FB2u)
            return PR_U3C6_0105_CPU_CODE_NOT_ORIGINAL;
    } else if (rd8(&m, c->cs, 0x0115u) != 0x9Au || rd16(&m, c->cs, 0x0116u) != 0x2A0Du ||
               rd8(&m, c->cs, 0x0120u) != 0x9Au || rd16(&m, c->cs, 0x0121u) != 0x2A0Du ||
               rd16(&m, c->cs, 0x0123u) != m.res_cs || rd8(&m, m.res_cs, 0x2A41u) != 0x9Au ||
               rd16(&m, m.res_cs, 0x2A42u) != 0x000Bu) {
        return PR_U3C6_0105_CPU_ARGUMENT;                                     /* SYNTHETIC: the call sites only */
    }
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
    return retf(&m, 0u, 0x0126u);                                             /* +0126 retf */
}
