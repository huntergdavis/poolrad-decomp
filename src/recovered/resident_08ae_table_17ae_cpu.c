/* RES_08AE [17AE,181A) raw CPU; docs/re/RES_08AE_TABLE_17AE_CPU.md. The offsets and effects follow the merged
 * resident_8ae_table_17ae.c; this file adds the CPU frame, the stack, the saved registers and the flags it lacks. */
#include "poolrad/recovered/resident_08ae_table_17ae_cpu.h"
#define MASK (PR_R08AE_17AE_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word(const uint8_t *r, size_t at) { return (uint16_t)(r[at] | ((uint16_t)r[(at + 1u) & MASK] << 8)); }
static void put(uint8_t *r, size_t at, uint16_t v) { r[at] = (uint8_t)v; r[(at + 1u) & MASK] = (uint8_t)(v >> 8); }
static void push(uint8_t *r, pr_r08ae_17ae_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); put(r, physical(c->ss, c->sp), v); }
static uint16_t pop(const uint8_t *r, pr_r08ae_17ae_cpu *c) { uint16_t v = word(r, physical(c->ss, c->sp)); c->sp = (uint16_t)(c->sp + 2u); return v; }
static unsigned even(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) == 0; }
static void szp(pr_r08ae_17ae_cpu *c, uint16_t r, uint16_t sign) {
    if (even((uint8_t)r)) c->flags |= 0x04;
    if (r == 0) c->flags |= 0x40;
    if (r & sign) c->flags |= 0x80;
}
/* ADD, and SUB/CMP, on bytes (bits 8) or words (bits 16): the six arithmetic flags. */
static uint16_t arith(pr_r08ae_17ae_cpu *c, uint16_t a, uint16_t b, unsigned bits, int sub) {
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
static uint8_t dec8(pr_r08ae_17ae_cpu *c, uint8_t v) {
    uint8_t r = (uint8_t)(v - 1u);
    uint16_t carry = (uint16_t)(c->flags & 1u);
    c->flags &= (uint16_t)~0x08D5u;
    if ((r & 0x0Fu) == 0x0Fu) c->flags |= 0x10;
    if (r == 0x7Fu) c->flags |= 0x800;
    szp(c, r, 0x80);
    c->flags |= carry;
    return r;
}
pr_r08ae_17ae_status pr_r08ae_table_17ae_cpu(uint8_t *ram, size_t size, pr_r08ae_17ae_cpu *c)
{
    size_t at;
    uint8_t bl, bh;
    if (!ram || !c || size != PR_R08AE_17AE_RAM_SIZE || c->ip != 0x17AEu || c->sp < 0x000Cu || c->sp > 0xFFFAu)
        return PR_R08AE_17AE_ARGUMENT;
    push(ram, c, c->bp); c->bp = c->sp;                                                  /* 17AE PUSH BP; MOV BP,SP */
    push(ram, c, c->ax); push(ram, c, c->bx); push(ram, c, c->di); push(ram, c, c->si); push(ram, c, c->ds); /* 17B1..17B5 */
    c->si = word(ram, physical(c->cs, 0x17B7u)); c->ds = c->si;                          /* 17B6 MOV SI,imm (relocated); 17B9 MOV DS,SI */
    c->si = word(ram, physical(c->ss, (uint16_t)(c->bp + 6u)));                          /* 17BB MOV SI,[BP+6] */
    put(ram, physical(c->ds, 0x01FFu), c->si);                                           /* 17BE */
    c->si = arith(c, c->si, c->si, 16, 0); c->si = arith(c, c->si, c->si, 16, 0); c->si = arith(c, c->si, c->si, 16, 0); /* 17C2..17C6 */
    ram[physical(c->ds, 0x01D8u)] = 1u;                                                  /* 17C8 */
    ram[physical(c->ds, 0x01D9u)] = 4u;                                                  /* 17CD */
    c->di = 0x03C7u;                                                                     /* 17D2 */
    for (;;) {
        c->ax = word(ram, physical(c->ds, (uint16_t)(c->si + 0x0607u)));                /* 17D5 */
        arith(c, c->ax, 0, 16, 1);                                                       /* 17D9 CMP AX,0 */
        if (!(c->flags & 0x40u)) {                                                       /* 17DC JE 1800 not taken */
            c->bx = 0x002Eu;                                                             /* 17DE */
            do {
                put(ram, physical(c->ds, (uint16_t)(c->bx + c->di)), 0u);               /* 17E1 */
                c->bx = arith(c, c->bx, 2u, 16, 1);                                      /* 17E5 SUB BX,2 */
            } while (((c->flags >> 7) & 1u) == ((c->flags >> 11) & 1u));                 /* 17E8 JGE 17E1: SF == OF */
            put(ram, physical(c->ds, (uint16_t)(c->di + 2u)), c->ax);                    /* 17EA */
            bl = 4u; bh = (uint8_t)(c->bx >> 8);                                         /* 17ED MOV BL,4 */
            bl = (uint8_t)arith(c, bl, ram[physical(c->ds, 0x01D9u)], 8, 1);             /* 17EF SUB BL,[01D9] */
            bh = (uint8_t)arith(c, bh, bh, 8, 1);                                        /* 17F3 SUB BH,BH */
            c->bx = (uint16_t)(bl | ((uint16_t)bh << 8));
            c->ax = word(ram, physical(c->ds, 0x01FFu));                                 /* 17F5 */
            ram[physical(c->ds, (uint16_t)(c->bx + 0x0209u))] = (uint8_t)c->ax;          /* 17F8 */
            put(ram, physical(c->ds, c->di), 1u);                                        /* 17FC */
        }
        c->di = arith(c, c->di, 0x30u, 16, 0);                                           /* 1800 */
        c->si = arith(c, c->si, 2u, 16, 0);                                              /* 1803 */
        at = physical(c->ds, 0x01D9u);
        ram[at] = dec8(c, ram[at]);                                                      /* 1806 DEC BYTE [01D9] */
        if (c->flags & 0x40u) break;                                                     /* 180A JNE 17D5 */
    }
    ram[physical(c->ds, 0x01D8u)] = 0u;                                                  /* 180C */
    c->ds = pop(ram, c); c->si = pop(ram, c); c->di = pop(ram, c); c->bx = pop(ram, c); c->ax = pop(ram, c); c->bp = pop(ram, c);
    at = physical(c->ss, c->sp);
    c->ip = word(ram, at); c->cs = word(ram, (at + 2u) & MASK);                          /* 1817 RETF 2 */
    c->sp = (uint16_t)(c->sp + 6u);
    return PR_R08AE_17AE_OK;
}
