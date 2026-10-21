/* START.EXE 0AF8:16BC + 1712, Turbo Pascal SYSTEM ParamStr (n <> 0), raw CPU (docs/re/SYSTEM_PARAMSTR_16BC_CPU.md). */
#include "poolrad/recovered/system_paramstr_16bc_cpu.h"

#define MASK (PR_PARAMSTR_RAM_SIZE - 1u)
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)v; ram[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static void push(uint8_t *ram, pr_paramstr_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, pr_paramstr_cpu *c)
{
    uint16_t v = rd16(ram, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    return v;
}
static uint16_t parity(unsigned v)
{
    v &= 0xFFu; v ^= v >> 4u; v ^= v >> 2u; v ^= v >> 1u;
    return (v & 1u) ? 0u : PF;
}
static uint16_t szp16(unsigned r) { return (uint16_t)((r == 0u ? ZF : 0u) | ((r & 0x8000u) ? SF : 0u) | parity(r)); }
static uint16_t szp8(unsigned r) { return (uint16_t)((r == 0u ? ZF : 0u) | ((r & 0x80u) ? SF : 0u) | parity(r)); }
#define ARITH (CF | PF | AF | ZF | SF | OF)

/* OR/XOR r16 (CF OF AF clear). */
static void logic16(pr_paramstr_cpu *c, unsigned r) { c->flags = (uint16_t)((c->flags & ~ARITH) | szp16(r)); }
/* CMP BYTE ES:[DI],20h */
static void cmp8_20(pr_paramstr_cpu *c, unsigned a)
{
    unsigned r = (a - 0x20u) & 0xFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | szp8(r) | (a < 0x20u ? CF : 0u) | ((a ^ 0x20u ^ r) & AF) |
                          (((a ^ 0x20u) & (a ^ r) & 0x80u) ? OF : 0u));
}
/* SUB r16,r16 */
static uint16_t sub16(pr_paramstr_cpu *c, unsigned a, unsigned b)
{
    unsigned r = (a - b) & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | szp16(r) | (a < b ? CF : 0u) | ((a ^ b ^ r) & AF) |
                          (((a ^ b) & (a ^ r) & 0x8000u) ? OF : 0u));
    return (uint16_t)r;
}
/* INC/DEC r16: CF kept. */
static uint16_t incdec16(pr_paramstr_cpu *c, unsigned a, int dec)
{
    unsigned r = (dec ? a - 1u : a + 1u) & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~(ARITH & ~CF)) | szp16(r) | ((a ^ 1u ^ r) & AF) |
                          ((dec ? a == 0x8000u : a == 0x7FFFu) ? OF : 0u));
    return (uint16_t)r;
}

pr_paramstr_status pr_system_paramstr_16bc(uint8_t *ram, size_t ram_size, pr_paramstr_cpu *c)
{
    if (ram == NULL || c == NULL || ram_size != PR_PARAMSTR_RAM_SIZE || c->ip != 0x16BCu || c->sp < 0x0006u ||
        c->sp > 0xFFF6u || rd16(ram, c->ss, (uint16_t)(c->sp + 4u)) == 0u)
        return PR_PARAMSTR_ARGUMENT;
    push(ram, c, c->bp);                                              /* 16BC PUSH BP */
    c->bp = c->sp;                                                    /* 16BD MOV BP,SP */
    c->dx = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));                 /* 16BF MOV DX,[BP+6] */
    logic16(c, c->dx);                                                /* 16C2 OR DX,DX; 16C4 JE 16CB (not taken: n <> 0) */
    push(ram, c, 0x16C9u);                                            /* 16C6 CALL 1712 */
    c->es = rd16(ram, c->ds, 0x4422u);                                /* 1712 MOV ES,[4422] */
    c->di = 0x0080u;                                                  /* 1716 MOV DI,80h */
    c->cx = ram[physical(c->es, c->di)];                              /* 1719 MOV CL,ES:[DI]; 171C XOR CH,CH */
    c->flags = (uint16_t)((c->flags & ~ARITH) | ZF | PF);
    c->di = incdec16(c, c->di, 0);                                    /* 171E INC DI */
    c->bx = 0u; c->flags = (uint16_t)((c->flags & ~ARITH) | ZF | PF); /* 171F XOR BX,BX */
    for (;;) {
        /* 1721 JCXZ 172C; 1723 CMP ES:[DI],20h; 1727 JA 172C; 1729 INC DI; 172A LOOP 1723: skip blanks */
        while (c->cx != 0u) {
            cmp8_20(c, ram[physical(c->es, c->di)]);
            if ((c->flags & (CF | ZF)) == 0u)
                break;
            c->di = incdec16(c, c->di, 0);
            c->cx = (uint16_t)(c->cx - 1u);
        }
        c->si = c->di;                                                /* 172C MOV SI,DI */
        /* 172E JCXZ 1739; 1730 CMP ES:[DI],20h; 1734 JBE 1739; 1736 INC DI; 1737 LOOP 1730: the word */
        while (c->cx != 0u) {
            cmp8_20(c, ram[physical(c->es, c->di)]);
            if ((c->flags & (CF | ZF)) != 0u)
                break;
            c->di = incdec16(c, c->di, 0);
            c->cx = (uint16_t)(c->cx - 1u);
        }
        c->ax = c->di;                                                /* 1739 MOV AX,DI */
        c->ax = sub16(c, c->ax, c->si);                               /* 173B SUB AX,SI */
        if ((c->flags & ZF) != 0u)                                    /* 173D JE 1743 */
            break;
        c->bx = incdec16(c, c->bx, 0);                                /* 173F INC BX */
        c->dx = incdec16(c, c->dx, 1);                                /* 1740 DEC DX */
        if ((c->flags & ZF) != 0u)                                    /* 1741 JNE 1721 */
            break;
    }
    c->ip = pop(ram, c);                                              /* 1743 RET (to 16C9: nothing wrote the stack) */
    push(ram, c, c->ds);                                              /* 16C9 JMP 16FB; 16FB PUSH DS */
    push(ram, c, c->es);                                              /* 16FC PUSH ES */
    c->ds = pop(ram, c);                                              /* 16FD POP DS */
    {   /* 16FE LES DI,[BP+8] */
        uint16_t off = rd16(ram, c->ss, (uint16_t)(c->bp + 8u)), seg = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au));
        c->di = off; c->es = seg;
    }
    c->flags &= (uint16_t)~0x0400u;                                   /* 1701 CLD */
    ram[physical(c->es, c->di)] = (uint8_t)c->ax;                     /* 1702 STOSB */
    c->di = (uint16_t)(c->di + 1u);
    {   uint16_t t = c->cx; c->cx = c->ax; c->ax = t; }               /* 1703 XCHG CX,AX */
    while (c->cx != 0u) {                                             /* 1704 REP MOVSB */
        ram[physical(c->es, c->di)] = ram[physical(c->ds, c->si)];
        c->si = (uint16_t)(c->si + 1u); c->di = (uint16_t)(c->di + 1u); c->cx = (uint16_t)(c->cx - 1u);
    }
    c->ds = pop(ram, c);                                              /* 1706 POP DS */
    c->bp = pop(ram, c);                                              /* 1707 POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 1708 RETF 2 */
    c->sp = (uint16_t)(c->sp + 2u);
    return PR_PARAMSTR_RETURNED;
}
