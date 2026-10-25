/* START.EXE resident 00BA +2E46: the character link flags test, full CPU state (docs/re/RES_00BA_2E46_CPU.md). */
#include "poolrad/recovered/resident_00ba_link_flags_2e46_cpu.h"

#define MASK (PR_RES_2E03_RAM_SIZE - 1u)
#define ARITH 0x08D5u
typedef pr_res_2e46_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static void les(const uint8_t *ram, cpu_t *c, uint16_t seg, uint16_t off)   /* LES as pinned DOSBox-X (prefix_none.h C4h): the offset word at the EA,
                                                                              * the segment word at EA+2-10000h when the offset is above FFFCh, i.e. at
                                                                              * (offset+2) mod 64K; each word itself is read physically contiguous */
{ uint16_t o = rd16(ram, seg, off); c->es = rd16(ram, seg, (uint16_t)(off + 2u)); c->di = o; }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 0x0004u; }
static void set_arith(cpu_t *c, uint16_t f) { c->flags = (uint16_t)((c->flags & ~ARITH) | f); }
static uint16_t sub16(cpu_t *c, uint16_t a, uint16_t b)                     /* SUB r16,imm: the six arithmetic flags (stored rule, PF of the low byte) */
{
    uint16_t r = (uint16_t)(a - b), f = parity((uint8_t)r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;
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
static void logic8(cpu_t *c, uint8_t r)
{ uint16_t f = parity(r); if (!r) f |= 0x0040u; if (r & 0x80u) f |= 0x0080u; set_arith(c, f); }
static uint16_t shl16(cpu_t *c, uint16_t v, uint8_t count)                 /* SHL r16,CL; the masked count is non-zero here (CL = 4) */
{
    unsigned n = count & 0x1Fu; uint16_t r = (uint16_t)((uint32_t)v << n), f = parity((uint8_t)r);
    unsigned cf = n <= 16u ? (unsigned)((v >> (16u - n)) & 1u) : 0u;
    if (!n) return v;
    if (cf) f |= 0x0001u;
    f |= 0x0010u;                                                          /* pinned: AF = (count & 1Fh) != 0 */
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (((r >> 15u) & 1u) ^ cf) f |= 0x0800u;                              /* stored rule: OF = MSB(result) XOR CF */
    set_arith(c, f); return r;
}

pr_res_2e46_status pr_res_00ba_link_flags_2e46_cpu(uint8_t *ram, size_t ram_size, pr_res_2e46_cpu *c)
{
    uint16_t ret_ip, ret_cs, cs;
    pr_res_2e03_status s;
    if (ram == NULL || c == NULL || ram_size != PR_RES_2E03_RAM_SIZE || c->ip != 0x2E46u || c->sp < 0x0010u || c->sp > 0xFFF8u)
        return PR_RES_2E46_ARGUMENT;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u)); cs = c->cs;
    push(ram, c, c->bp); c->bp = c->sp;                                                     /* 2E46 PUSH BP; 2E47 MOV BP,SP */
    c->sp = sub16(c, c->sp, 2u);                                                            /* 2E49 SUB SP,2 */
    push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + 8u)));                                 /* 2E4C PUSH [BP+8] */
    push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + 6u)));                                 /* 2E4F PUSH [BP+6] */
    push(ram, c, c->cs);                                                                    /* 2E52 PUSH CS */
    push(ram, c, 0x2E56u); c->ip = 0x2E03u;                                                 /* 2E53 CALL 2E03 */
    s = pr_res_00ba_link_check_2e03_cpu(ram, ram_size, c);
    if (s == PR_RES_2E03_TRANSFER) return PR_RES_2E46_TRANSFER;
    if (s != PR_RES_2E03_RETURNED) return PR_RES_2E46_CALLEE;
    if (c->cs != cs || c->ip != 0x2E56u) return PR_RES_2E46_TRANSFER;
    logic8(c, (uint8_t)c->ax);                                                              /* 2E56 OR AL,AL */
    if ((uint8_t)c->ax == 0u) {                                                             /* 2E58 JE 2E7E */
        wr8(ram, c->ss, (uint16_t)(c->bp - 1u), 0u);                                        /* 2E7E MOV BYTE [BP-1],0 */
    } else {
        les(ram, c, c->ss, (uint16_t)(c->bp + 6u));                                         /* 2E5A LES DI,[BP+6] */
        les(ram, c, c->es, (uint16_t)(c->di + 0x00CCu));                                    /* 2E5D LES DI,ES:[DI+CC] */
        c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->es, (uint16_t)(c->di + 0x2Eu)));  /* 2E62 MOV AL,ES:[DI+2E] */
        c->ax &= 0x00FFu; logic8(c, 0u);                                                    /* 2E66 XOR AH,AH */
        c->di = c->ax;                                                                      /* 2E68 MOV DI,AX */
        c->cx = (uint16_t)((c->cx & 0xFF00u) | 4u);                                         /* 2E6A MOV CL,4 */
        c->di = shl16(c, c->di, (uint8_t)c->cx);                                            /* 2E6C SHL DI,CL */
        c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ds, (uint16_t)(c->di + 0x5590u)));   /* 2E6E MOV AL,[DI+5590] */
        c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint8_t)c->ax & 0x14u)); logic8(c, (uint8_t)c->ax);   /* 2E72 AND AL,14h */
        cmp8(c, (uint8_t)c->ax, 0x14u);                                                     /* 2E74 CMP AL,14h */
        if ((c->flags & 0x0040u) == 0u)                                                     /* 2E76 JNE 2E7E */
            wr8(ram, c->ss, (uint16_t)(c->bp - 1u), 0u);                                    /* 2E7E MOV BYTE [BP-1],0 */
        else
            wr8(ram, c->ss, (uint16_t)(c->bp - 1u), 1u);                                    /* 2E78 MOV BYTE [BP-1],1; 2E7C JMP 2E82 */
    }
    c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ss, (uint16_t)(c->bp - 1u)));         /* 2E82 MOV AL,[BP-1] */
    c->sp = c->bp; c->bp = pop(ram, c);                                                     /* 2E85 MOV SP,BP; 2E87 POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u);               /* 2E88 RETF 4 */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_RES_2E46_TRANSFER;
    return PR_RES_2E46_RETURNED;
}
