/* Original unit 0x6A +0824: a 21-byte selection-exchange sort through a global far pointer (docs/re/OVL_006A_0824_CPU.md). */
#include "poolrad/recovered/overlay_u6a_0824_raw_cpu.h"

#define MASK (PR_BUS_RAM_SIZE - 1u)
typedef pr_u6a_0824_raw_regs cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 4u; }
static uint16_t arith(cpu_t *c, uint16_t a, uint16_t b, unsigned bits, int subtract)         /* ADD/SUB/CMP at 8 or 16 bits: every arithmetic flag */
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = (mask + 1u) >> 1u;
    uint32_t wide = subtract ? (uint32_t)a - b : (uint32_t)a + b;
    uint16_t r = (uint16_t)(wide & mask), f = parity((uint8_t)r);
    if (subtract ? a < b : wide > mask) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & sign) f |= 0x0080u;
    if ((subtract ? ((a ^ b) & (a ^ r)) : (~(a ^ b) & (a ^ r))) & sign) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
static uint8_t logic8(cpu_t *c, uint8_t r)                                                     /* AND/XOR: CF, OF and AF clear; PF, ZF, SF from the result */
{ c->flags = (uint16_t)((c->flags & ~0x08D5u) | parity(r) | (r ? 0u : 0x0040u) | ((r & 0x80u) ? 0x0080u : 0u)); return r; }
static uint8_t inc8(cpu_t *c, uint8_t a)
{
    uint8_t r = (uint8_t)(a + 1u); uint16_t f = parity(r);
    if ((a ^ 1u ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if (a == 0x7Fu) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D4u) | f);
    return r;
}
static void xor_ah(cpu_t *c) { (void)logic8(c, 0u); c->ax &= 0x00FFu; }                                  /* XOR AH,AH: the result is always zero */
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static void set_dl(cpu_t *c, uint8_t v) { c->dx = (uint16_t)((c->dx & 0xFF00u) | v); }
static uint8_t local(const uint8_t *ram, const cpu_t *c, int off) { return rd8(ram, c->ss, (uint16_t)(c->bp + off)); }
static void set_local(uint8_t *ram, const cpu_t *c, int off, uint8_t v) { wr8(ram, c->ss, (uint16_t)(c->bp + off), v); }
static void les_g(const uint8_t *ram, cpu_t *c) { c->di = rd16(ram, c->ds, 0x5D92u); c->es = rd16(ram, c->ds, 0x5D94u); }   /* LES DI,[5D92] */
/* Accesses through the far pointer go through the bus: ES:[DI+17h]. */
static uint8_t elem_rd(pr_bus *bus, const cpu_t *c) { return pr_bus_read8(bus, physical(c->es, (uint16_t)(c->di + 0x17u))); }
static void elem_wr(pr_bus *bus, const cpu_t *c, uint8_t v) { pr_bus_write8(bus, physical(c->es, (uint16_t)(c->di + 0x17u)), v); }

pr_u6a_0824_raw_status pr_u6a_0824_raw_cpu(pr_bus *bus, size_t ram_size, pr_u6a_0824_raw_regs *c, size_t *budget)
{
    uint8_t *ram; uint16_t ret_ip, ret_cs;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0824u || c->sp < 0x0008u || c->sp > 0xFFFCu)
        return PR_U6A_0824_RAW_ARGUMENT;
    ram = bus->ram; ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    for (;;) {
        if (*budget == 0u) return PR_U6A_0824_RAW_LIMIT;
        --*budget;
        switch (c->ip) {
        case 0x0824u: push(ram, c, c->bp); c->ip = 0x0825u; break;                                                /* PUSH BP */
        case 0x0825u: c->bp = c->sp; c->ip = 0x0827u; break;                                                      /* MOV BP,SP */
        case 0x0827u: c->sp = arith(c, c->sp, 4u, 16u, 1); c->ip = 0x082Au; break;                                /* SUB SP,4 */
        case 0x082Au: set_local(ram, c, -1, 0u); c->ip = 0x082Eu; break;                                          /* MOV BYTE [BP-1],0 */
        case 0x082Eu: c->ip = 0x0833u; break;                                                                      /* JMP 0833 */
        case 0x0830u: set_local(ram, c, -1, inc8(c, local(ram, c, -1))); c->ip = 0x0833u; break;                  /* INC BYTE [BP-1] */
        case 0x0833u: set_al(c, local(ram, c, -1)); c->ip = 0x0836u; break;                                        /* MOV AL,[BP-1] */
        case 0x0836u: (void)arith(c, (uint8_t)c->ax, 0x14u, 8u, 1); c->ip = 0x0838u; break;                       /* CMP AL,14h */
        case 0x0838u: c->ip = (c->flags & 0x0041u) == 0u ? 0x08B2u : 0x083Au; break;                              /* JA 08B2 */
        case 0x083Au: set_local(ram, c, -2, (uint8_t)c->ax); c->ip = 0x083Du; break;                              /* MOV [BP-2],AL */
        case 0x083Du: c->ip = 0x0842u; break;                                                                      /* JMP 0842 */
        case 0x083Fu: set_local(ram, c, -2, inc8(c, local(ram, c, -2))); c->ip = 0x0842u; break;                  /* INC BYTE [BP-2] */
        case 0x0842u: set_al(c, local(ram, c, -2)); c->ip = 0x0845u; break;                                        /* MOV AL,[BP-2] */
        case 0x0845u: xor_ah(c); c->ip = 0x0847u; break;   /* XOR AH,AH */
        case 0x0847u: les_g(ram, c); c->ip = 0x084Bu; break;                                                       /* LES DI,[5D92] */
        case 0x084Bu: c->di = arith(c, c->di, c->ax, 16u, 0); c->ip = 0x084Du; break;                              /* ADD DI,AX */
        case 0x084Du: set_al(c, elem_rd(bus, c)); c->ip = 0x0851u; break;                                          /* MOV AL,ES:[DI+17h] */
        case 0x0851u: set_al(c, logic8(c, (uint8_t)(c->ax & 0x7Fu))); c->ip = 0x0853u; break;                     /* AND AL,7Fh */
        case 0x0853u: set_dl(c, (uint8_t)c->ax); c->ip = 0x0855u; break;                                           /* MOV DL,AL */
        case 0x0855u: set_al(c, local(ram, c, -1)); c->ip = 0x0858u; break;                                        /* MOV AL,[BP-1] */
        case 0x0858u: xor_ah(c); c->ip = 0x085Au; break;    /* XOR AH,AH */
        case 0x085Au: les_g(ram, c); c->ip = 0x085Eu; break;                                                       /* LES DI,[5D92] */
        case 0x085Eu: c->di = arith(c, c->di, c->ax, 16u, 0); c->ip = 0x0860u; break;                              /* ADD DI,AX */
        case 0x0860u: set_al(c, elem_rd(bus, c)); c->ip = 0x0864u; break;                                          /* MOV AL,ES:[DI+17h] */
        case 0x0864u: set_al(c, logic8(c, (uint8_t)(c->ax & 0x7Fu))); c->ip = 0x0866u; break;                     /* AND AL,7Fh */
        case 0x0866u: (void)arith(c, (uint8_t)c->ax, (uint8_t)c->dx, 8u, 1); c->ip = 0x0868u; break;              /* CMP AL,DL */
        case 0x0868u: c->ip = (c->flags & 0x0041u) != 0u ? 0x08ACu : 0x086Au; break;                              /* JBE 08AC */
        case 0x086Au: set_al(c, local(ram, c, -1)); c->ip = 0x086Du; break;                                        /* MOV AL,[BP-1] */
        case 0x086Du: xor_ah(c); c->ip = 0x086Fu; break;    /* XOR AH,AH */
        case 0x086Fu: les_g(ram, c); c->ip = 0x0873u; break;                                                       /* LES DI,[5D92] */
        case 0x0873u: c->di = arith(c, c->di, c->ax, 16u, 0); c->ip = 0x0875u; break;                              /* ADD DI,AX */
        case 0x0875u: set_al(c, elem_rd(bus, c)); c->ip = 0x0879u; break;                                          /* MOV AL,ES:[DI+17h] */
        case 0x0879u: set_local(ram, c, -3, (uint8_t)c->ax); c->ip = 0x087Cu; break;                               /* MOV [BP-3],AL */
        case 0x087Cu: set_al(c, local(ram, c, -2)); c->ip = 0x087Fu; break;                                        /* MOV AL,[BP-2] */
        case 0x087Fu: xor_ah(c); c->ip = 0x0881u; break;    /* XOR AH,AH */
        case 0x0881u: les_g(ram, c); c->ip = 0x0885u; break;                                                       /* LES DI,[5D92] */
        case 0x0885u: c->di = arith(c, c->di, c->ax, 16u, 0); c->ip = 0x0887u; break;                              /* ADD DI,AX */
        case 0x0887u: set_dl(c, elem_rd(bus, c)); c->ip = 0x088Bu; break;                                          /* MOV DL,ES:[DI+17h] */
        case 0x088Bu: set_al(c, local(ram, c, -1)); c->ip = 0x088Eu; break;                                        /* MOV AL,[BP-1] */
        case 0x088Eu: xor_ah(c); c->ip = 0x0890u; break;    /* XOR AH,AH */
        case 0x0890u: les_g(ram, c); c->ip = 0x0894u; break;                                                       /* LES DI,[5D92] */
        case 0x0894u: c->di = arith(c, c->di, c->ax, 16u, 0); c->ip = 0x0896u; break;                              /* ADD DI,AX */
        case 0x0896u: elem_wr(bus, c, (uint8_t)c->dx); c->ip = 0x089Au; break;                                     /* MOV ES:[DI+17h],DL */
        case 0x089Au: set_dl(c, local(ram, c, -3)); c->ip = 0x089Du; break;                                        /* MOV DL,[BP-3] */
        case 0x089Du: set_al(c, local(ram, c, -2)); c->ip = 0x08A0u; break;                                        /* MOV AL,[BP-2] */
        case 0x08A0u: xor_ah(c); c->ip = 0x08A2u; break;    /* XOR AH,AH */
        case 0x08A2u: les_g(ram, c); c->ip = 0x08A6u; break;                                                       /* LES DI,[5D92] */
        case 0x08A6u: c->di = arith(c, c->di, c->ax, 16u, 0); c->ip = 0x08A8u; break;                              /* ADD DI,AX */
        case 0x08A8u: elem_wr(bus, c, (uint8_t)c->dx); c->ip = 0x08ACu; break;                                     /* MOV ES:[DI+17h],DL */
        case 0x08ACu: (void)arith(c, local(ram, c, -2), 0x14u, 8u, 1); c->ip = 0x08B0u; break;                     /* CMP BYTE [BP-2],14h */
        case 0x08B0u: c->ip = (c->flags & 0x0040u) == 0u ? 0x083Fu : 0x08B2u; break;                              /* JNE 083F */
        case 0x08B2u: (void)arith(c, local(ram, c, -1), 0x13u, 8u, 1); c->ip = 0x08B6u; break;                     /* CMP BYTE [BP-1],13h */
        case 0x08B6u: c->ip = (c->flags & 0x0040u) != 0u ? 0x08BBu : 0x08B8u; break;                              /* JE 08BB */
        case 0x08B8u: c->ip = 0x0830u; break;                                                                      /* JMP 0830 */
        case 0x08BBu: c->sp = c->bp; c->ip = 0x08BDu; break;                                                       /* MOV SP,BP */
        case 0x08BDu: c->bp = pop(ram, c); c->ip = 0x08BEu; break;                                                 /* POP BP */
        case 0x08BEu: c->ip = pop(ram, c); c->cs = pop(ram, c);                                                    /* RETF */
            if (c->ip != ret_ip || c->cs != ret_cs) return PR_U6A_0824_RAW_TRANSFER;
            return PR_U6A_0824_RAW_RETURNED;
        default: ++*budget; return PR_U6A_0824_RAW_TRANSFER;                                                           /* control left the span (cannot happen: the span has no indirect jump) */
        }
    }
}
