/* GAME.OVR unit 0x7F entries 10 (+0130) and 11 (+0185): the div 5 leaves, full CPU state (docs/re/OVL_007F_DIV5_LEAVES_CPU.md). */
#include "poolrad/recovered/overlay_u7f_div5_leaves_cpu.h"

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define ARITH 0x08D5u
typedef pr_u7f_div5_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static void bwr16(pr_bus *bus, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); pr_bus_write8(bus, p, (uint8_t)v); pr_bus_write8(bus, (p + 1u) & MASK, (uint8_t)(v >> 8u)); }
/* LES as pinned DOSBox-X: the segment word at (offset+2) mod 64K */
static void les(const uint8_t *ram, cpu_t *c, uint16_t seg, uint16_t off)
{ uint16_t o = rd16(ram, seg, off); c->es = rd16(ram, seg, (uint16_t)(off + 2u)); c->di = o; }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 0x0004u; }
static void set_arith(cpu_t *c, uint16_t f) { c->flags = (uint16_t)((c->flags & ~ARITH) | f); }
/* SUB SP,2: the six arithmetic flags (PF of the low byte) */
static uint16_t sub16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a - b), f = parity((uint8_t)r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;
    set_arith(c, f); return r;
}
/* ADD DI,AX */
static uint16_t add16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a + b), f = parity((uint8_t)r);
    if (r < a) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (~(a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;
    set_arith(c, f); return r;
}
static void cmp8(cpu_t *c, uint8_t a, uint8_t b)
{
    uint8_t r = (uint8_t)(a - b); uint16_t f = parity(r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x80u) f |= 0x0800u;
    set_arith(c, f);
}
static uint16_t xor_zero(cpu_t *c) { set_arith(c, 0x0044u); return 0u; }    /* XOR r,r: ZF and PF set, CF/OF/AF/SF clear */
/* INC r/m8: CF kept */
static uint8_t inc8(cpu_t *c, uint8_t v)
{
    uint8_t r = (uint8_t)(v + 1u); uint16_t f = parity(r);
    if ((v & 0x0Fu) == 0x0Fu) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if (r == 0x80u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D4u) | f); return r;
}
/* SHL r16,1, pinned stored rule: AF set for a non-zero count; OF = CF XOR bit15(r) */
static uint16_t shl1(cpu_t *c, uint16_t v)
{
    uint16_t r = (uint16_t)(v << 1u), f = (uint16_t)(parity((uint8_t)r) | 0x0010u);
    if (v & 0x8000u) f |= 0x0001u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((v ^ r) & 0x8000u) f |= 0x0800u;
    set_arith(c, f); return r;
}
/* DIV CX with DX = 0 and CX = 5 (never faults); pinned DIVW flags */
static void div5(cpu_t *c)
{
    uint32_t n = ((uint32_t)c->dx << 16u) | c->ax;
    uint16_t quo = (uint16_t)(n / c->cx), rem = (uint16_t)(n % c->cx), f;
    c->ax = quo; c->dx = rem;
    f = parity((uint8_t)(rem ^ (rem >> 8u) ^ quo ^ (quo >> 8u)));
    if (rem == 0u && (quo & 1u)) f |= 0x0040u;
    if ((rem & 3u) == 1u || (rem & 3u) == 2u) f |= 0x0001u;
    set_arith(c, f);
}
static void xchg_dx_ax(cpu_t *c) { uint16_t t = c->ax; c->ax = c->dx; c->dx = t; }

#define STEP(ip_) do { if (*budget == 0u) { c->ip = (ip_); return PR_U7F_DIV5_CPU_LIMIT; } --*budget; } while (0)
#define ENTER(entry_) \
    uint8_t *ram; uint16_t ret_ip, ret_cs; \
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != (entry_) || c->sp < 0x0004u || c->sp > 0xFFFAu) \
        return PR_U7F_DIV5_CPU_ARGUMENT; \
    ram = bus->ram; ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u))
#define LEAVE(at_) \
    STEP((uint16_t)(at_)); c->sp = c->bp;                                          /* MOV SP,BP */ \
    STEP((uint16_t)((at_) + 2u)); c->bp = pop(ram, c);                             /* POP BP */ \
    STEP((uint16_t)((at_) + 3u)); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 2u);   /* RETF 2 */ \
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_DIV5_CPU_TRANSFER; \
    return PR_U7F_DIV5_CPU_RETURNED

pr_u7f_div5_cpu_status pr_u7f_div5_0130_cpu(pr_bus *bus, size_t ram_size, pr_u7f_div5_cpu *c, size_t *budget)
{
    ENTER(0x0130u);
    STEP(0x0130u); push(ram, c, c->bp);                                                                  /* 0130 PUSH BP */
    STEP(0x0131u); c->bp = c->sp;                                                                        /* 0131 MOV BP,SP */
    STEP(0x0133u); c->sp = sub16(c, c->sp, 2u);                                                          /* 0133 SUB SP,2 */
    STEP(0x0136u); wr8(ram, c->ss, (uint16_t)(c->bp - 1u), 0u);                                          /* 0136 MOV BYTE [BP-1],0 */
    STEP(0x013Au);                                                                                       /* 013A JMP 013F */
    for (;;) {
        STEP(0x013Fu); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ss, (uint16_t)(c->bp - 1u)));  /* 013F MOV AL,[BP-1] */
        STEP(0x0142u); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax;                                /* 0142 CBW */
        STEP(0x0143u); c->ax = shl1(c, c->ax);                                                           /* 0143 SHL AX,1 */
        STEP(0x0145u); les(ram, c, c->ds, 0x5D92u);                                                      /* 0145 LES DI,[5D92] */
        STEP(0x0149u); c->di = add16(c, c->di, c->ax);                                                   /* 0149 ADD DI,AX */
        STEP(0x014Bu); c->ax = xor_zero(c);                                                              /* 014B XOR AX,AX */
        STEP(0x014Du); bwr16(bus, c->es, (uint16_t)(c->di + 0x0088u), c->ax);                            /* 014D MOV ES:[DI+88],AX */
        STEP(0x0152u); cmp8(c, rd8(ram, c->ss, (uint16_t)(c->bp - 1u)), 4u);                             /* 0152 CMP BYTE [BP-1],4 */
        STEP(0x0156u); if (c->flags & 0x0040u) break;                                                    /* 0156 JNE 013C */
        STEP(0x013Cu); wr8(ram, c->ss, (uint16_t)(c->bp - 1u), inc8(c, rd8(ram, c->ss, (uint16_t)(c->bp - 1u))));   /* 013C INC BYTE [BP-1] */
    }
    STEP(0x0158u); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));                                     /* 0158 MOV AX,[BP+6] */
    STEP(0x015Bu); c->dx = xor_zero(c);                                                                  /* 015B XOR DX,DX */
    STEP(0x015Du); c->cx = 5u;                                                                           /* 015D MOV CX,5 */
    STEP(0x0160u); div5(c);                                                                              /* 0160 DIV CX */
    STEP(0x0162u); les(ram, c, c->ds, 0x5D92u);                                                          /* 0162 LES DI,[5D92] */
    STEP(0x0166u); bwr16(bus, c->es, (uint16_t)(c->di + 0x0090u), c->ax);                                /* 0166 MOV ES:[DI+90],AX */
    STEP(0x016Bu); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));                                     /* 016B MOV AX,[BP+6] */
    STEP(0x016Eu); c->dx = xor_zero(c);                                                                  /* 016E XOR DX,DX */
    STEP(0x0170u); c->cx = 5u;                                                                           /* 0170 MOV CX,5 */
    STEP(0x0173u); div5(c);                                                                              /* 0173 DIV CX */
    STEP(0x0175u); xchg_dx_ax(c);                                                                        /* 0175 XCHG DX,AX */
    STEP(0x0176u); les(ram, c, c->ds, 0x5D92u);                                                          /* 0176 LES DI,[5D92] */
    STEP(0x017Au); bwr16(bus, c->es, (uint16_t)(c->di + 0x008Eu), c->ax);                                /* 017A MOV ES:[DI+8E],AX */
    LEAVE(0x017Fu);
}

pr_u7f_div5_cpu_status pr_u7f_div5_0185_cpu(pr_bus *bus, size_t ram_size, pr_u7f_div5_cpu *c, size_t *budget)
{
    ENTER(0x0185u);
    STEP(0x0185u); push(ram, c, c->bp);                                                                  /* 0185 PUSH BP */
    STEP(0x0186u); c->bp = c->sp;                                                                        /* 0186 MOV BP,SP */
    STEP(0x0188u); c->sp = sub16(c, c->sp, 2u);                                                          /* 0188 SUB SP,2 */
    STEP(0x018Bu); wr8(ram, c->ss, (uint16_t)(c->bp - 1u), 0u);                                          /* 018B MOV BYTE [BP-1],0 */
    STEP(0x018Fu);                                                                                       /* 018F JMP 0194 */
    for (;;) {
        STEP(0x0194u); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ss, (uint16_t)(c->bp - 1u)));  /* 0194 MOV AL,[BP-1] */
        STEP(0x0197u); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax;                                /* 0197 CBW */
        STEP(0x0198u); c->di = c->ax;                                                                    /* 0198 MOV DI,AX */
        STEP(0x019Au); c->di = shl1(c, c->di);                                                           /* 019A SHL DI,1 */
        STEP(0x019Cu); c->di = shl1(c, c->di);                                                           /* 019C SHL DI,1 */
        STEP(0x019Eu); c->ax = xor_zero(c);                                                              /* 019E XOR AX,AX */
        STEP(0x01A0u); wr16(ram, c->ds, (uint16_t)(c->di + 0x67F4u), c->ax);                             /* 01A0 MOV [DI+67F4],AX */
        STEP(0x01A4u); wr16(ram, c->ds, (uint16_t)(c->di + 0x67F6u), c->ax);                             /* 01A4 MOV [DI+67F6],AX */
        STEP(0x01A8u); cmp8(c, rd8(ram, c->ss, (uint16_t)(c->bp - 1u)), 4u);                             /* 01A8 CMP BYTE [BP-1],4 */
        STEP(0x01ACu); if (c->flags & 0x0040u) break;                                                    /* 01AC JNE 0191 */
        STEP(0x0191u); wr8(ram, c->ss, (uint16_t)(c->bp - 1u), inc8(c, rd8(ram, c->ss, (uint16_t)(c->bp - 1u))));   /* 0191 INC BYTE [BP-1] */
    }
    STEP(0x01AEu); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));                                     /* 01AE MOV AX,[BP+6] */
    STEP(0x01B1u); c->dx = xor_zero(c);                                                                  /* 01B1 XOR DX,DX */
    STEP(0x01B3u); c->cx = 5u;                                                                           /* 01B3 MOV CX,5 */
    STEP(0x01B6u); div5(c);                                                                              /* 01B6 DIV CX */
    STEP(0x01B8u); c->dx = xor_zero(c);                                                                  /* 01B8 XOR DX,DX */
    STEP(0x01BAu); wr16(ram, c->ds, 0x6804u, c->ax);                                                     /* 01BA MOV [6804],AX */
    STEP(0x01BDu); wr16(ram, c->ds, 0x6806u, c->dx);                                                     /* 01BD MOV [6806],DX */
    STEP(0x01C1u); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));                                     /* 01C1 MOV AX,[BP+6] */
    STEP(0x01C4u); c->dx = xor_zero(c);                                                                  /* 01C4 XOR DX,DX */
    STEP(0x01C6u); c->cx = 5u;                                                                           /* 01C6 MOV CX,5 */
    STEP(0x01C9u); div5(c);                                                                              /* 01C9 DIV CX */
    STEP(0x01CBu); xchg_dx_ax(c);                                                                        /* 01CB XCHG DX,AX */
    STEP(0x01CCu); c->dx = xor_zero(c);                                                                  /* 01CC XOR DX,DX */
    STEP(0x01CEu); wr16(ram, c->ds, 0x6800u, c->ax);                                                     /* 01CE MOV [6800],AX */
    STEP(0x01D1u); wr16(ram, c->ds, 0x6802u, c->dx);                                                     /* 01D1 MOV [6802],DX */
    LEAVE(0x01D5u);
}
