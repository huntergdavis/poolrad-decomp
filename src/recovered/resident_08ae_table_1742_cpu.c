/* RES_08AE [1742,17AE) raw CPU; docs/re/RES_08AE_TABLE_1742_CPU.md. The offsets and effects follow the merged
 * resident_8ae_table_1742.c; this file adds the CPU frame, the stack, the saved registers and the flags it lacks. */
#include "poolrad/recovered/resident_08ae_table_1742_cpu.h"
#define MASK (PR_R08AE_1742_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word(const uint8_t *r, size_t at) { return (uint16_t)(r[at] | ((uint16_t)r[(at + 1u) & MASK] << 8)); }
static void put(uint8_t *r, size_t at, uint16_t v) { r[at] = (uint8_t)v; r[(at + 1u) & MASK] = (uint8_t)(v >> 8); }
static void push(uint8_t *r, pr_r08ae_1742_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); put(r, physical(c->ss, c->sp), v); }
static uint16_t pop(const uint8_t *r, pr_r08ae_1742_cpu *c) { uint16_t v = word(r, physical(c->ss, c->sp)); c->sp = (uint16_t)(c->sp + 2u); return v; }
static unsigned even(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) == 0; }
static void szp(pr_r08ae_1742_cpu *c, uint16_t r, uint16_t sign) {
    if (even((uint8_t)r)) c->flags |= 0x04;
    if (r == 0) c->flags |= 0x40;
    if (r & sign) c->flags |= 0x80;
}
/* ADD, and SUB/CMP, on bytes (bits 8) or words (bits 16): the six arithmetic flags. */
static uint16_t arith(pr_r08ae_1742_cpu *c, uint16_t a, uint16_t b, unsigned bits, int sub) {
    uint32_t mask = bits == 8 ? 0xFFu : 0xFFFFu;
    uint16_t sign = bits == 8 ? 0x80u : 0x8000u, r;
    a = (uint16_t)(a & mask); b = (uint16_t)(b & mask);
    r = (uint16_t)((sub ? (uint32_t)a - b : (uint32_t)a + b) & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (sub ? a < b : ((uint32_t)a + b) > mask) c->flags |= 0x01;
    if ((a ^ b ^ r) & 0x10) c->flags |= 0x10;
    if (sub ? ((a ^ b) & (a ^ r) & sign) : ((uint16_t)~(a ^ b) & (a ^ r) & sign)) c->flags |= 0x800;
    szp(c, r, sign);
    return r;
}
/* DEC byte: CF is kept. */
static uint8_t dec8(pr_r08ae_1742_cpu *c, uint8_t v) {
    uint8_t r = (uint8_t)(v - 1u);
    uint16_t carry = (uint16_t)(c->flags & 1u);
    c->flags &= (uint16_t)~0x08D5u;
    if ((r & 0x0Fu) == 0x0Fu) c->flags |= 0x10;
    if (r == 0x7Fu) c->flags |= 0x800;
    szp(c, r, 0x80);
    c->flags |= carry;
    return r;
}
pr_r08ae_1742_status pr_r08ae_table_1742_cpu(uint8_t *ram, size_t size, pr_r08ae_1742_cpu *c)
{
    size_t at;
    uint8_t bl, bh;
    if (!ram || !c || size != PR_R08AE_1742_RAM_SIZE || c->ip != 0x1742u || c->sp < 0x000Cu || c->sp > 0xFFFAu)
        return PR_R08AE_1742_ARGUMENT;
    push(ram, c, c->bp); c->bp = c->sp;                                                  /* 1742 PUSH BP; MOV BP,SP */
    push(ram, c, c->ax); push(ram, c, c->bx); push(ram, c, c->di); push(ram, c, c->si); push(ram, c, c->ds); /* 1745..1749 */
    c->si = word(ram, physical(c->cs, 0x174Bu)); c->ds = c->si;                          /* 174A MOV SI,imm (relocated); 174D MOV DS,SI */
    c->si = word(ram, physical(c->ss, (uint16_t)(c->bp + 6u)));                          /* 174F MOV SI,[BP+6] */
    put(ram, physical(c->ds, 0x01FFu), c->si);                                           /* 1752 */
    c->si = arith(c, c->si, c->si, 16, 0); c->si = arith(c, c->si, c->si, 16, 0); c->si = arith(c, c->si, c->si, 16, 0); /* 1756..175A */
    ram[physical(c->ds, 0x01D8u)] = 1u;                                                  /* 175C */
    ram[physical(c->ds, 0x01D9u)] = 4u;                                                  /* 1761 */
    c->di = 0x022Du;                                                                     /* 1766 */
    for (;;) {
        c->ax = word(ram, physical(c->ds, (uint16_t)(c->si + 0x055Fu)));                /* 1769 */
        arith(c, c->ax, 0, 16, 1);                                                       /* 176D CMP AX,0 */
        if (!(c->flags & 0x40u)) {                                                       /* 1770 JE 1794 not taken */
            c->bx = 0x002Eu;                                                             /* 1772 */
            do {
                put(ram, physical(c->ds, (uint16_t)(c->bx + c->di)), 0u);               /* 1775 */
                c->bx = arith(c, c->bx, 2u, 16, 1);                                      /* 1779 SUB BX,2 */
            } while (((c->flags >> 7) & 1u) == ((c->flags >> 11) & 1u));                 /* 177C JGE 1775: SF == OF */
            put(ram, physical(c->ds, (uint16_t)(c->di + 2u)), c->ax);                    /* 177E */
            bl = 4u; bh = (uint8_t)(c->bx >> 8);                                         /* 1781 MOV BL,4 */
            bl = (uint8_t)arith(c, bl, ram[physical(c->ds, 0x01D9u)], 8, 1);             /* 1783 SUB BL,[01D9] */
            bh = (uint8_t)arith(c, bh, bh, 8, 1);                                        /* 1787 SUB BH,BH */
            c->bx = (uint16_t)(bl | ((uint16_t)bh << 8));
            c->ax = word(ram, physical(c->ds, 0x01FFu));                                 /* 1789 */
            ram[physical(c->ds, (uint16_t)(c->bx + 0x0209u))] = (uint8_t)c->ax;          /* 178C */
            put(ram, physical(c->ds, c->di), 1u);                                        /* 1790 */
        }
        c->di = arith(c, c->di, 0x30u, 16, 0);                                           /* 1794 */
        c->si = arith(c, c->si, 2u, 16, 0);                                              /* 1797 */
        at = physical(c->ds, 0x01D9u);
        ram[at] = dec8(c, ram[at]);                                                      /* 179A DEC BYTE [01D9] */
        if (c->flags & 0x40u) break;                                                     /* 179E JNE 1769 */
    }
    ram[physical(c->ds, 0x01D8u)] = 0u;                                                  /* 17A0 */
    c->ds = pop(ram, c); c->si = pop(ram, c); c->di = pop(ram, c); c->bx = pop(ram, c); c->ax = pop(ram, c); c->bp = pop(ram, c);
    at = physical(c->ss, c->sp);
    c->ip = word(ram, at); c->cs = word(ram, (at + 2u) & MASK);                          /* 17AB RETF 2 */
    c->sp = (uint16_t)(c->sp + 6u);
    return PR_R08AE_1742_OK;
}
