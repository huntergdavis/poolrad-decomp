/* GAME.OVR unit 0x3C6 + 0x04BE (entry 6) on live CPU/RAM; see the header.
 * Comments name the original offsets +04BE..+0528. */
#include "poolrad/recovered/overlay_u3c6_04be_cpu.h"

#define MASK (PR_U3C6_04BE_CPU_RAM_SIZE - 1u)
#define F_CF 0x0001u
#define F_PF 0x0004u
#define F_AF 0x0010u
#define F_ZF 0x0040u
#define F_SF 0x0080u
#define F_OF 0x0800u
#define ARITH (F_CF | F_PF | F_AF | F_ZF | F_SF | F_OF)
#define BODY_START 0x04BEu
#define BODY_END 0x052Bu

typedef pr_u3c6_04be_cpu cpu_t;

static size_t phys(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[phys(seg, off)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = phys(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}
static int in_body(const cpu_t *c, uint16_t entry_cs, size_t p)
{
    (void)c;
    return ((p - phys(entry_cs, BODY_START)) & MASK) < (size_t)(BODY_END - BODY_START);
}
static int put8(uint8_t *ram, const cpu_t *c, uint16_t entry_cs, uint16_t seg, uint16_t off, uint8_t v)
{
    size_t p = phys(seg, off);
    if (in_body(c, entry_cs, p)) return -1;
    ram[p] = v;
    return 0;
}
static int put16(uint8_t *ram, const cpu_t *c, uint16_t entry_cs, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = phys(seg, off), hi = (at + 1u) & MASK;
    if (in_body(c, entry_cs, at) || in_body(c, entry_cs, hi)) return -1;
    ram[at] = (uint8_t)v;
    ram[hi] = (uint8_t)(v >> 8);
    return 0;
}

static uint16_t parity(uint8_t v)
{
    v ^= (uint8_t)(v >> 4);
    v ^= (uint8_t)(v >> 2);
    v ^= (uint8_t)(v >> 1);
    return (v & 1u) ? 0u : F_PF;
}
static void put_arith(cpu_t *c, uint16_t f) { c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f); }
/* CMP r/m8,imm8 */
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
static int jl(const cpu_t *c) { return ((c->flags & F_SF) != 0u) != ((c->flags & F_OF) != 0u); }
static int jg(const cpu_t *c) { return (c->flags & F_ZF) == 0u && !jl(c); }
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
/* LES di,[seg:off] in the pinned DOSBox-X normal-core profile: the offset word
 * at linear EA, the segment word at linear EA+2, minus 0x10000 when the
 * offset is above 0xFFFC (only offset 0xFFFD differs from seg:(off+2)). */
/* nonzero when a stack LES operand offset is in the unmeasured seam FFFD..FFFF */
static int les_seam(uint16_t off) { return off > 0xFFFCu; }
static void les(const uint8_t *ram, cpu_t *c, uint16_t seg, uint16_t off)
{
    size_t ea = phys(seg, off);
    size_t at = (ea + 2u - (off > 0xFFFCu ? 0x10000u : 0u)) & MASK;
    c->di = rd16(ram, seg, off);
    c->es = (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

pr_u3c6_04be_cpu_status pr_u3c6_04be_cpu_run(uint8_t *ram, size_t ram_size, pr_u3c6_04be_cpu *cpu,
                                             pr_u3c6_04be_retf_policy retf_policy)
{
    cpu_t *c = cpu;
    uint16_t cs0;
    size_t at, cs_at;
    if (ram == NULL || c == NULL || ram_size != PR_U3C6_04BE_CPU_RAM_SIZE || c->ip != BODY_START ||
        (retf_policy != PR_U3C6_04BE_RETF_UNMEASURED && retf_policy != PR_U3C6_04BE_RETF_WRAPPED_SS &&
         retf_policy != PR_U3C6_04BE_RETF_CONTIGUOUS_PHYSICAL))
        return PR_U3C6_04BE_CPU_ARGUMENT;
    cs0 = c->cs;

    if (put16(ram, c, cs0, c->ss, (uint16_t)(c->sp - 2u), c->bp) != 0)   /* +04BE push bp */
        return PR_U3C6_04BE_CPU_UNSCREENED_ALIAS;
    c->sp = (uint16_t)(c->sp - 2u);
    c->bp = c->sp;                                                       /* +04BF mov bp,sp */

    cmp8(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x10u)), 0x00u);          /* +04C1 cmp byte [bp+0x10],0 */
    if (jl(c)) goto out_of_range;                                        /* +04C5 jl +0519 */
    cmp8(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x10u)), 0x31u);          /* +04C7 cmp byte [bp+0x10],0x31 */
    if (jg(c)) goto out_of_range;                                        /* +04CB jg +0519 */
    cmp8(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Eu)), 0x00u);          /* +04CD cmp byte [bp+0xE],0 */
    if (jl(c)) goto out_of_range;                                        /* +04D1 jl +0519 */
    cmp8(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Eu)), 0x18u);          /* +04D3 cmp byte [bp+0xE],0x18 */
    if (jg(c)) goto out_of_range;                                        /* +04D7 jg +0519 */

    set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x10u)));               /* +04D9 mov al,[bp+0x10] */
    cbw(c);                                                              /* +04DC cbw */
    c->cx = c->ax;                                                       /* +04DD mov cx,ax */
    set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Eu)));               /* +04DF mov al,[bp+0xE] */
    cbw(c);                                                              /* +04E2 cbw */
    c->dx = 0x32u;                                                       /* +04E3 mov dx,0x32 */
    mul16(c, c->dx);                                                     /* +04E6 mul dx */
    les(ram, c, c->ds, 0x6716u);                                         /* +04E8 les di,[0x6716] */
    c->di = add16(c, c->di, c->ax);                                      /* +04EC add di,ax */
    c->di = add16(c, c->di, c->cx);                                      /* +04EE add di,cx */
    set_al(c, rd8(ram, c->es, (uint16_t)(c->di + 7u)));                  /* +04F0 mov al,es:[di+7] */
    if (les_seam((uint16_t)(c->bp + 6u))) goto seam_04f4;
    les(ram, c, c->ss, (uint16_t)(c->bp + 6u));                          /* +04F4 les di,[bp+6] */
    if (put8(ram, c, cs0, c->es, c->di, (uint8_t)c->ax) != 0) {          /* +04F7 mov es:[di],al */
        c->ip = 0x04F7u;
        return PR_U3C6_04BE_CPU_UNSCREENED_ALIAS;
    }
    set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x10u)));               /* +04FA mov al,[bp+0x10] */
    cbw(c);                                                              /* +04FD cbw */
    c->cx = c->ax;                                                       /* +04FE mov cx,ax */
    set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Eu)));               /* +0500 mov al,[bp+0xE] */
    cbw(c);                                                              /* +0503 cbw */
    c->dx = 0x32u;                                                       /* +0504 mov dx,0x32 */
    mul16(c, c->dx);                                                     /* +0507 mul dx */
    c->di = c->ax;                                                       /* +0509 mov di,ax */
    c->di = add16(c, c->di, c->cx);                                      /* +050B add di,cx */
    set_al(c, rd8(ram, c->ds, (uint16_t)(c->di + 0x60DBu)));             /* +050D mov al,[di+0x60DB] */
    if (les_seam((uint16_t)(c->bp + 0x0Au))) { c->ip = 0x0511u; return PR_U3C6_04BE_CPU_UNSUPPORTED_LES_SEAM; }
    les(ram, c, c->ss, (uint16_t)(c->bp + 0x0Au));                       /* +0511 les di,[bp+0xA] */
    if (put8(ram, c, cs0, c->es, c->di, (uint8_t)c->ax) != 0) {          /* +0514 mov es:[di],al */
        c->ip = 0x0514u;
        return PR_U3C6_04BE_CPU_UNSCREENED_ALIAS;
    }
    goto done;                                                           /* +0517 jmp +0527 */

out_of_range:
    if (les_seam((uint16_t)(c->bp + 0x0Au))) { c->ip = 0x0519u; return PR_U3C6_04BE_CPU_UNSUPPORTED_LES_SEAM; }
    les(ram, c, c->ss, (uint16_t)(c->bp + 0x0Au));                       /* +0519 les di,[bp+0xA] */
    if (put8(ram, c, cs0, c->es, c->di, 0u) != 0) {                      /* +051C mov byte es:[di],0 */
        c->ip = 0x051Cu;
        return PR_U3C6_04BE_CPU_UNSCREENED_ALIAS;
    }
    if (les_seam((uint16_t)(c->bp + 6u))) { c->ip = 0x0520u; return PR_U3C6_04BE_CPU_UNSUPPORTED_LES_SEAM; }
    les(ram, c, c->ss, (uint16_t)(c->bp + 6u));                          /* +0520 les di,[bp+6] */
    if (put8(ram, c, cs0, c->es, c->di, 0u) != 0) {                      /* +0523 mov byte es:[di],0 */
        c->ip = 0x0523u;
        return PR_U3C6_04BE_CPU_UNSCREENED_ALIAS;
    }

done:
    c->bp = rd16(ram, c->ss, c->sp);                                     /* +0527 pop bp */
    c->sp = (uint16_t)(c->sp + 2u);
    c->ip = 0x0528u;                                                     /* +0528 retf 0xC, by policy */
    if (retf_policy == PR_U3C6_04BE_RETF_UNMEASURED) return PR_U3C6_04BE_CPU_RETURN_PENDING;
    at = phys(c->ss, c->sp);
    cs_at = retf_policy == PR_U3C6_04BE_RETF_WRAPPED_SS ? phys(c->ss, (uint16_t)(c->sp + 2u))
                                                        : (at + 2u) & MASK;
    c->ip = (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
    c->cs = (uint16_t)(ram[cs_at] | ((uint16_t)ram[(cs_at + 1u) & MASK] << 8));
    c->sp = (uint16_t)(c->sp + 4u + 0x0Cu);
    return PR_U3C6_04BE_CPU_RETURNED;

seam_04f4:
    c->ip = 0x04F4u;
    return PR_U3C6_04BE_CPU_UNSUPPORTED_LES_SEAM;
}
