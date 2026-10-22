/* Turbo Pascal SYSTEM 6-byte Real entry points 0AF8:1186 (multiply), 119A (LongInt to Real) and
 * 119E (Real to LongInt, truncating), raw CPU; see docs/re/SYSTEM_REAL_CPU.md. */
#include "poolrad/recovered/system_real_cpu.h"

#define MASK (PR_SYSTEM_REAL_RAM_SIZE - 1u)

typedef struct { uint8_t *r; pr_system_real_cpu *c; } machine;

enum { OP_ADD, OP_ADC, OP_SUB, OP_CMP, OP_OR, OP_AND, OP_XOR, OP_TEST };

static size_t physical(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & MASK; }
static uint16_t rd16(const machine *m, uint16_t s, uint16_t o)
{
    size_t p = physical(s, o);
    return (uint16_t)(m->r[p] | ((uint16_t)m->r[(p + 1u) & MASK] << 8));
}
static uint8_t rd8(const machine *m, uint16_t s, uint16_t o) { return m->r[physical(s, o)]; }
static void push16(machine *m, uint16_t v)
{
    size_t p;
    m->c->sp = (uint16_t)(m->c->sp - 2u);
    p = physical(m->c->ss, m->c->sp);
    m->r[p] = (uint8_t)v;
    m->r[(p + 1u) & MASK] = (uint8_t)(v >> 8);
}
static uint16_t pop16(machine *m)
{
    uint16_t v = rd16(m, m->c->ss, m->c->sp);
    m->c->sp = (uint16_t)(m->c->sp + 2u);
    return v;
}
static uint8_t lo(uint16_t r) { return (uint8_t)r; }
static uint8_t hi(uint16_t r) { return (uint8_t)(r >> 8); }
static void setlo(uint16_t *r, uint8_t v) { *r = (uint16_t)((*r & 0xFF00u) | v); }
static void sethi(uint16_t *r, uint8_t v) { *r = (uint16_t)((*r & 0x00FFu) | ((uint16_t)v << 8)); }
static void swap16(uint16_t *a, uint16_t *b) { uint16_t t = *a; *a = *b; *b = t; }

static unsigned parity_even8(uint8_t v)
{
    v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1);
    return (v & 1u) == 0;
}
static unsigned popcount16(uint16_t v)
{
    unsigned n = 0;
    while (v) { n += v & 1u; v >>= 1; }
    return n;
}
static void szp(pr_system_real_cpu *c, uint16_t r, uint16_t sign)
{
    if (parity_even8((uint8_t)r)) c->flags |= 0x04;
    if (r == 0) c->flags |= 0x40;
    if (r & sign) c->flags |= 0x80;
}

/* ADD/ADC/SUB/CMP/OR/AND/XOR/TEST on bytes (bits 8) or words (bits 16). The result is returned; the
 * caller stores it unless the operation is CMP or TEST. */
static uint16_t alu(pr_system_real_cpu *c, int op, uint16_t a, uint16_t b, unsigned bits)
{
    uint32_t mask = bits == 8 ? 0xFFu : 0xFFFFu, full;
    uint16_t sign = bits == 8 ? 0x80u : 0x8000u, r;
    unsigned carry_in = c->flags & 1u;
    a = (uint16_t)(a & mask);
    b = (uint16_t)(b & mask);
    switch (op) {
    case OP_ADD: full = (uint32_t)a + b; break;
    case OP_ADC: full = (uint32_t)a + b + carry_in; break;
    case OP_SUB: case OP_CMP: full = (uint32_t)a - b; break;
    case OP_OR: full = (uint32_t)(a | b); break;
    case OP_AND: case OP_TEST: full = (uint32_t)(a & b); break;
    default: full = (uint32_t)(a ^ b); break;
    }
    r = (uint16_t)(full & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (op <= OP_CMP) {
        if (op == OP_SUB || op == OP_CMP) {
            if (a < b) c->flags |= 0x01;
            if ((a ^ b) & (a ^ r) & sign) c->flags |= 0x800;
        } else {
            if (full > mask) c->flags |= 0x01;
            if ((uint16_t)~(a ^ b) & (a ^ r) & sign) c->flags |= 0x800;
        }
        if ((a ^ b ^ r) & 0x10) c->flags |= 0x10;
    }
    szp(c, r, sign);
    return r;
}

/* INC/DEC: CF is kept. */
static uint16_t incdec(pr_system_real_cpu *c, uint16_t v, unsigned bits, int dec)
{
    uint16_t mask = bits == 8 ? 0xFFu : 0xFFFFu, sign = bits == 8 ? 0x80u : 0x8000u;
    uint16_t r = (uint16_t)((dec ? v - 1u : v + 1u) & mask), carry = (uint16_t)(c->flags & 1u);
    c->flags &= (uint16_t)~0x08D5u;
    if (dec ? (r & 0x0Fu) == 0x0Fu : (r & 0x0Fu) == 0u) c->flags |= 0x10;
    if (dec ? r == (uint16_t)(sign - 1u) : r == sign) c->flags |= 0x800;
    szp(c, r, sign);
    c->flags |= carry;
    return r;
}

/* Shifts and rotates by one. SHL/SHR set AF; RCL/RCR change only CF and OF. */
static uint16_t shl16(pr_system_real_cpu *c, uint16_t v)
{
    uint16_t r = (uint16_t)(v << 1);
    c->flags &= (uint16_t)~0x08D5u;
    c->flags |= 0x10;
    if (v & 0x8000u) c->flags |= 0x01;
    if ((r ^ v) & 0x8000u) c->flags |= 0x800;
    szp(c, r, 0x8000u);
    return r;
}
static uint16_t shr1(pr_system_real_cpu *c, uint16_t v, unsigned bits)
{
    uint16_t sign = bits == 8 ? 0x80u : 0x8000u, r = (uint16_t)(v >> 1);
    c->flags &= (uint16_t)~0x08D5u;
    c->flags |= 0x10;
    if (v & 1u) c->flags |= 0x01;
    if (v > sign) c->flags |= 0x800; /* pinned SHR count 1: OF = (var1 > sign mask) */
    szp(c, r, sign);
    return r;
}
static uint16_t rcl16(pr_system_real_cpu *c, uint16_t v)
{
    unsigned carry = c->flags & 1u, out = (v >> 15) & 1u;
    uint16_t r = (uint16_t)((v << 1) | carry);
    c->flags &= (uint16_t)~0x0801u;
    if (out) c->flags |= 0x01;
    if (out ^ ((r >> 15) & 1u)) c->flags |= 0x800;
    return r;
}
static uint16_t rcr16(pr_system_real_cpu *c, uint16_t v)
{
    unsigned carry = c->flags & 1u, out = v & 1u;
    uint16_t r = (uint16_t)((v >> 1) | (carry << 15));
    c->flags &= (uint16_t)~0x0801u;
    if (out) c->flags |= 0x01;
    if (((r >> 15) ^ (r >> 14)) & 1u) c->flags |= 0x800;
    return r;
}

/* MUL: SF and AF are kept. */
static void mul8(pr_system_real_cpu *c, uint8_t operand)
{
    uint16_t product = (uint16_t)(lo(c->ax) * operand);
    c->ax = product;
    c->flags &= (uint16_t)~0x0845u;
    if (lo(product) == 0) c->flags |= 0x40;
    if ((popcount16(product) & 1u) == 0) c->flags |= 0x04;
    if (hi(product)) c->flags |= 0x801;
}
static void mul16(pr_system_real_cpu *c, uint16_t operand)
{
    uint32_t product = (uint32_t)c->ax * operand;
    c->ax = (uint16_t)product;
    c->dx = (uint16_t)(product >> 16);
    c->flags &= (uint16_t)~0x0845u;
    if (c->ax == 0) c->flags |= 0x40;
    if (((popcount16(c->ax) + popcount16(c->dx)) & 1u) == 0) c->flags |= 0x04;
    if (c->dx) c->flags |= 0x801;
}

static int zf(const pr_system_real_cpu *c) { return (c->flags & 0x40u) != 0; }
static int sf(const pr_system_real_cpu *c) { return (c->flags & 0x80u) != 0; }
static int cf(const pr_system_real_cpu *c) { return (c->flags & 0x01u) != 0; }

/* 0AF8:0F33 (reached through the +0F30 zero tail): Real multiply; operands in AX/BX/DX and
 * CX/SI/DI, result in AL/BX/DX with the exponent in AL; CF is set on overflow by the caller's
 * check (this core clears nothing on its own: the final flags are the last instruction's). */
static void core_mul(machine *m)
{
    pr_system_real_cpu *c = m->c;
    uint16_t t;
    alu(c, OP_OR, lo(c->ax), lo(c->ax), 8);                                      /* 0F33 */
    if (zf(c)) goto zero;
    alu(c, OP_OR, lo(c->cx), lo(c->cx), 8);                                      /* 0F37 */
    if (zf(c)) goto zero;
    push16(m, c->bp);                                                            /* 0F3B */
    c->bp = c->dx;                                                               /* 0F3C */
    c->dx = alu(c, OP_XOR, c->dx, c->di, 16);                                    /* 0F3E */
    c->dx = alu(c, OP_AND, c->dx, 0x8000u, 16);                                  /* 0F40 */
    t = lo(c->ax); setlo(&c->ax, lo(c->dx)); setlo(&c->dx, (uint8_t)t);          /* 0F44 XCHG AL,DL */
    setlo(&c->dx, (uint8_t)alu(c, OP_ADD, lo(c->dx), lo(c->cx), 8));             /* 0F46 */
    sethi(&c->dx, (uint8_t)alu(c, OP_ADC, hi(c->dx), lo(c->ax), 8));             /* 0F48 */
    setlo(&c->cx, lo(c->ax));                                                    /* 0F4A */
    c->bp = alu(c, OP_OR, c->bp, 0x8000u, 16);                                   /* 0F4C */
    c->di = alu(c, OP_OR, c->di, 0x8000u, 16);                                   /* 0F50 */
    push16(m, c->dx);                                                            /* 0F54 */
    alu(c, OP_OR, hi(c->ax), hi(c->ax), 8);                                      /* 0F55 */
    if (!zf(c)) goto general_check;
    alu(c, OP_OR, c->bx, c->bx, 16);                                             /* 0F59 */
    if (zf(c)) goto short_path;
general_check:
    alu(c, OP_OR, hi(c->cx), hi(c->cx), 8);                                      /* 0F5D */
    if (!zf(c)) goto general;
    alu(c, OP_OR, c->si, c->si, 16);                                             /* 0F61 */
    if (!zf(c)) goto general;
    swap16(&c->cx, &c->ax);                                                      /* 0F65 */
    swap16(&c->si, &c->bx);                                                      /* 0F66 */
    swap16(&c->di, &c->bp);                                                      /* 0F68 */
short_path:
    c->ax = c->bp;                                                               /* 0F6A */
    setlo(&c->ax, hi(c->ax));                                                    /* 0F6C */
    mul8(c, hi(c->cx));                                                          /* 0F6E MUL CH */
    c->bx = c->ax;                                                               /* 0F70 */
    c->ax = c->bp;                                                               /* 0F72 */
    mul16(c, c->si);                                                             /* 0F74 */
    c->bx = alu(c, OP_ADD, c->bx, c->ax, 16);                                    /* 0F76 */
    c->dx = alu(c, OP_ADC, c->dx, 0, 16);                                        /* 0F78 */
    c->cx = c->dx;                                                               /* 0F7B */
    c->ax = c->bp;                                                               /* 0F7D */
    mul16(c, c->di);                                                             /* 0F7F */
    c->ax = alu(c, OP_ADD, c->ax, c->cx, 16);                                    /* 0F81 */
    c->dx = alu(c, OP_ADC, c->dx, 0, 16);                                        /* 0F83 */
    goto normalise;                                                              /* 0F86 */
general:
    push16(m, c->di);                                                            /* 0F89 */
    push16(m, c->si);
    push16(m, c->cx);
    push16(m, c->bp);
    push16(m, c->bx);
    push16(m, c->ax);
    c->bp = c->sp;                                                               /* 0F8F */
    c->cx = alu(c, OP_XOR, c->cx, c->cx, 16);                                    /* 0F91 */
    setlo(&c->ax, rd8(m, c->ss, (uint16_t)(c->bp + 1u)));                        /* 0F93 */
    mul8(c, rd8(m, c->ss, (uint16_t)(c->bp + 7u)));                              /* 0F96 */
    c->si = c->ax;                                                               /* 0F99 */
    c->di = c->cx;                                                               /* 0F9B */
    c->bx = c->cx;                                                               /* 0F9D */
    c->ax = rd16(m, c->ss, c->bp);                                               /* 0F9F */
    mul16(c, rd16(m, c->ss, (uint16_t)(c->bp + 8u)));                            /* 0FA2 */
    c->si = alu(c, OP_ADD, c->si, c->ax, 16);                                    /* 0FA5 */
    c->di = alu(c, OP_ADC, c->di, c->dx, 16);                                    /* 0FA7 */
    c->bx = alu(c, OP_ADC, c->bx, c->cx, 16);                                    /* 0FA9 */
    c->ax = rd16(m, c->ss, (uint16_t)(c->bp + 2u));                              /* 0FAB */
    mul16(c, rd16(m, c->ss, (uint16_t)(c->bp + 6u)));                            /* 0FAE */
    c->si = alu(c, OP_ADD, c->si, c->ax, 16);                                    /* 0FB1 */
    c->di = alu(c, OP_ADC, c->di, c->dx, 16);                                    /* 0FB3 */
    c->bx = alu(c, OP_ADC, c->bx, c->cx, 16);                                    /* 0FB5 */
    c->si = c->cx;                                                               /* 0FB7 */
    c->ax = rd16(m, c->ss, c->bp);                                               /* 0FB9 */
    mul16(c, rd16(m, c->ss, (uint16_t)(c->bp + 10u)));                           /* 0FBC */
    c->di = alu(c, OP_ADD, c->di, c->ax, 16);                                    /* 0FBF */
    c->bx = alu(c, OP_ADC, c->bx, c->dx, 16);                                    /* 0FC1 */
    c->si = alu(c, OP_ADC, c->si, c->cx, 16);                                    /* 0FC3 */
    c->ax = rd16(m, c->ss, (uint16_t)(c->bp + 2u));                              /* 0FC5 */
    mul16(c, rd16(m, c->ss, (uint16_t)(c->bp + 8u)));                            /* 0FC8 */
    c->di = alu(c, OP_ADD, c->di, c->ax, 16);                                    /* 0FCB */
    c->bx = alu(c, OP_ADC, c->bx, c->dx, 16);                                    /* 0FCD */
    c->si = alu(c, OP_ADC, c->si, c->cx, 16);                                    /* 0FCF */
    c->ax = rd16(m, c->ss, (uint16_t)(c->bp + 4u));                              /* 0FD1 */
    mul16(c, rd16(m, c->ss, (uint16_t)(c->bp + 6u)));                            /* 0FD4 */
    c->di = alu(c, OP_ADD, c->di, c->ax, 16);                                    /* 0FD7 */
    c->bx = alu(c, OP_ADC, c->bx, c->dx, 16);                                    /* 0FD9 */
    c->si = alu(c, OP_ADC, c->si, c->cx, 16);                                    /* 0FDB */
    c->di = c->cx;                                                               /* 0FDD */
    c->ax = rd16(m, c->ss, (uint16_t)(c->bp + 2u));                              /* 0FDF */
    mul16(c, rd16(m, c->ss, (uint16_t)(c->bp + 10u)));                           /* 0FE2 */
    c->bx = alu(c, OP_ADD, c->bx, c->ax, 16);                                    /* 0FE5 */
    c->si = alu(c, OP_ADC, c->si, c->dx, 16);                                    /* 0FE7 */
    c->di = alu(c, OP_ADC, c->di, c->cx, 16);                                    /* 0FE9 */
    c->ax = rd16(m, c->ss, (uint16_t)(c->bp + 4u));                              /* 0FEB */
    mul16(c, rd16(m, c->ss, (uint16_t)(c->bp + 8u)));                            /* 0FEE */
    c->bx = alu(c, OP_ADD, c->bx, c->ax, 16);                                    /* 0FF1 */
    c->si = alu(c, OP_ADC, c->si, c->dx, 16);                                    /* 0FF3 */
    c->di = alu(c, OP_ADC, c->di, c->cx, 16);                                    /* 0FF5 */
    c->ax = rd16(m, c->ss, (uint16_t)(c->bp + 4u));                              /* 0FF7 */
    mul16(c, rd16(m, c->ss, (uint16_t)(c->bp + 10u)));                           /* 0FFA */
    c->ax = alu(c, OP_ADD, c->ax, c->si, 16);                                    /* 0FFD */
    c->dx = alu(c, OP_ADC, c->dx, c->di, 16);                                    /* 0FFF */
    c->sp = alu(c, OP_ADD, c->sp, 0x0Cu, 16);                                    /* 1001 */
normalise:
    swap16(&c->bx, &c->ax);                                                      /* 1004 */
    c->cx = pop16(m);                                                            /* 1005 */
    c->bp = pop16(m);                                                            /* 1006 */
    alu(c, OP_OR, hi(c->dx), hi(c->dx), 8);                                      /* 1007 */
    if (!sf(c)) {
        c->ax = shl16(c, c->ax);                                                 /* 100B */
        c->bx = rcl16(c, c->bx);                                                 /* 100D */
        c->dx = rcl16(c, c->dx);                                                 /* 100F */
        c->cx = incdec(c, c->cx, 16, 1);                                         /* 1011 */
    }
    c->cx = alu(c, OP_SUB, c->cx, 0x8081u, 16);                                  /* 1012 */
    c->ax = alu(c, OP_ADD, c->ax, 0x80u, 16);                                    /* 1016 */
    c->bx = alu(c, OP_ADC, c->bx, 0, 16);                                        /* 1019 */
    c->dx = alu(c, OP_ADC, c->dx, 0, 16);                                        /* 101C */
    if (cf(c)) {
        c->dx = rcr16(c, c->dx);                                                 /* 1021 */
        c->cx = incdec(c, c->cx, 16, 0);                                         /* 1023 */
    }
    alu(c, OP_TEST, hi(c->cx), 0x40u, 8);                                        /* 1024 */
    if (zf(c)) {
        c->cx = incdec(c, c->cx, 16, 0);                                         /* 1029 */
        setlo(&c->ax, lo(c->cx));                                                /* 102A */
        sethi(&c->dx, (uint8_t)alu(c, OP_XOR, hi(c->dx), hi(c->cx), 8));         /* 102C */
        sethi(&c->cx, (uint8_t)shr1(c, hi(c->cx), 8));                           /* 102E */
        return;                                                                  /* 1030 RET */
    }
zero:
    c->ax = alu(c, OP_XOR, c->ax, c->ax, 16);                                    /* 1031 */
    c->bx = c->ax;
    c->dx = c->ax;
}                                                                                /* 1037 RET */

/* 0AF8:10D9: LongInt DX:AX to Real (exponent in AL, mantissa BX/DX). */
static void core_from_long(machine *m)
{
    pr_system_real_cpu *c = m->c;
    c->bx = c->ax;                                                               /* 10D9 */
    c->bx = alu(c, OP_OR, c->bx, c->dx, 16);                                     /* 10DB */
    if (zf(c)) return;                                                           /* 10DD JE 1117 */
    sethi(&c->cx, hi(c->dx));                                                    /* 10DF */
    c->dx = alu(c, OP_OR, c->dx, c->dx, 16);                                     /* 10E1 */
    if (sf(c)) {                                                                 /* 10E3 JNS 10EF not taken */
        c->ax = (uint16_t)~c->ax;                                                /* 10E5 */
        c->dx = (uint16_t)~c->dx;                                                /* 10E7 */
        c->ax = alu(c, OP_ADD, c->ax, 1u, 16);                                   /* 10E9 */
        c->dx = alu(c, OP_ADC, c->dx, 0, 16);                                    /* 10EC */
    }
    c->bx = c->ax;                                                               /* 10EF */
    c->ax = 0xA0u;                                                               /* 10F1 */
    c->dx = alu(c, OP_OR, c->dx, c->dx, 16);                                     /* 10F4 */
    if (zf(c)) {                                                                 /* 10F6 JNE 1104 not taken */
        swap16(&c->bx, &c->dx);                                                  /* 10F8 */
        setlo(&c->ax, 0x90u);                                                    /* 10FA */
        sethi(&c->dx, (uint8_t)alu(c, OP_OR, hi(c->dx), hi(c->dx), 8));          /* 10FC */
        if (zf(c)) {                                                             /* 10FE JNE 1104 not taken */
            uint8_t t = lo(c->dx);                                               /* 1100 XCHG DL,DH */
            setlo(&c->dx, hi(c->dx));
            sethi(&c->dx, t);
            setlo(&c->ax, 0x88u);                                                /* 1102 */
        }
    }
    c->dx = alu(c, OP_OR, c->dx, c->dx, 16);                                     /* 1104 */
    if (!sf(c)) {                                                                /* 1106 JS 1110 */
        do {
            setlo(&c->ax, (uint8_t)incdec(c, lo(c->ax), 8, 1));                  /* 1108 */
            c->bx = alu(c, OP_ADD, c->bx, c->bx, 16);                            /* 110A */
            c->dx = alu(c, OP_ADC, c->dx, c->dx, 16);                            /* 110C */
        } while (!sf(c));                                                        /* 110E JNS 1108 */
    }
    alu(c, OP_OR, hi(c->cx), hi(c->cx), 8);                                      /* 1110 */
    if (sf(c)) return;                                                           /* 1112 JS 1117 */
    sethi(&c->dx, (uint8_t)alu(c, OP_AND, hi(c->dx), 0x7Fu, 8));                 /* 1114 */
}                                                                                /* 1117 RET */

/* 0AF8:1118: Real to LongInt. CF set on return means overflow; CH selects rounding (0 truncates). */
static void core_to_long(machine *m)
{
    pr_system_real_cpu *c = m->c;
    swap16(&c->bx, &c->ax);                                                      /* 1118 */
    setlo(&c->cx, 0x9Fu);                                                        /* 1119 */
    setlo(&c->cx, (uint8_t)alu(c, OP_SUB, lo(c->cx), lo(c->bx), 8));             /* 111B */
    if (cf(c)) goto overflow;                                                    /* 111D JB 116D */
    alu(c, OP_CMP, lo(c->cx), 0x1Fu, 8);                                         /* 111F */
    if (!cf(c) && !zf(c)) goto underflow;                                        /* 1122 JA 116F */
    setlo(&c->cx, (uint8_t)incdec(c, lo(c->cx), 8, 0));                          /* 1124 */
    sethi(&c->bx, hi(c->dx));                                                    /* 1126 */
    sethi(&c->dx, (uint8_t)alu(c, OP_OR, hi(c->dx), 0x80u, 8));                  /* 1128 */
    alu(c, OP_CMP, lo(c->cx), 0x11u, 8);                                         /* 112B */
    if (!cf(c)) {                                                                /* 112E JB 1137 */
        c->ax = c->dx;                                                           /* 1130 */
        c->dx = alu(c, OP_XOR, c->dx, c->dx, 16);                                /* 1132 */
        setlo(&c->cx, (uint8_t)alu(c, OP_SUB, lo(c->cx), 0x10u, 8));             /* 1134 */
    }
    alu(c, OP_CMP, lo(c->cx), 9u, 8);                                            /* 1137 */
    if (!cf(c)) {                                                                /* 113A JB 1147 */
        setlo(&c->ax, hi(c->ax));                                                /* 113C */
        sethi(&c->ax, lo(c->dx));                                                /* 113E */
        setlo(&c->dx, hi(c->dx));                                                /* 1140 */
        sethi(&c->dx, (uint8_t)alu(c, OP_XOR, hi(c->dx), hi(c->dx), 8));         /* 1142 */
        setlo(&c->cx, (uint8_t)alu(c, OP_SUB, lo(c->cx), 8u, 8));                /* 1144 */
    }
    do {
        c->dx = shr1(c, c->dx, 16);                                              /* 1147 */
        c->ax = rcr16(c, c->ax);                                                 /* 1149 */
        setlo(&c->cx, (uint8_t)incdec(c, lo(c->cx), 8, 1));                      /* 114B */
    } while (!zf(c));                                                            /* 114D JNE 1147 */
    if (cf(c)) {                                                                 /* 114F JAE 115D */
        alu(c, OP_OR, hi(c->cx), hi(c->cx), 8);                                  /* 1151 */
        if (!zf(c)) {                                                            /* 1153 JE 115D */
            c->ax = alu(c, OP_ADD, c->ax, 1u, 16);                               /* 1155 */
            c->dx = alu(c, OP_ADC, c->dx, 0, 16);                                /* 1158 */
            if (sf(c)) goto overflow;                                            /* 115B JS 116D */
        }
    }
    sethi(&c->bx, (uint8_t)alu(c, OP_OR, hi(c->bx), hi(c->bx), 8));              /* 115D */
    if (sf(c)) {                                                                 /* 115F JNS 116B */
        c->ax = (uint16_t)~c->ax;                                                /* 115F */
        c->dx = (uint16_t)~c->dx;                                                /* 1161 */
        c->ax = alu(c, OP_ADD, c->ax, 1u, 16);                                   /* 1165 */
        c->dx = alu(c, OP_ADC, c->dx, 0, 16);                                    /* 1168 */
    }
    c->flags &= (uint16_t)~0x0001u;                                              /* 116B CLC */
    return;                                                                      /* 116C RET */
overflow:
    c->flags |= 0x0001u;                                                         /* 116D STC */
    return;                                                                      /* 116E RET */
underflow:
    c->ax = alu(c, OP_XOR, c->ax, c->ax, 16);                                    /* 116F */
    c->dx = alu(c, OP_XOR, c->dx, c->dx, 16);                                    /* 1171 */
}                                                                                /* 1173 RET */

static int valid(const uint8_t *ram, size_t size, const pr_system_real_cpu *c, uint16_t entry)
{
    return ram != NULL && c != NULL && size == PR_SYSTEM_REAL_RAM_SIZE && c->ip == entry &&
           c->sp >= 0x0040u && c->sp <= 0xFFFCu;
}

static pr_system_real_status near_return(machine *m, uint16_t expected)
{
    uint16_t ip = pop16(m);
    if (ip != expected) { m->c->ip = ip; return PR_SYSTEM_REAL_TRANSFER; }
    return PR_SYSTEM_REAL_RETURNED;
}

static pr_system_real_status far_return(machine *m)
{
    size_t p = physical(m->c->ss, m->c->sp);
    m->c->ip = (uint16_t)(m->r[p] | ((uint16_t)m->r[(p + 1u) & MASK] << 8));
    m->c->cs = (uint16_t)(m->r[(p + 2u) & MASK] | ((uint16_t)m->r[(p + 3u) & MASK] << 8));
    m->c->sp = (uint16_t)(m->c->sp + 4u);
    return PR_SYSTEM_REAL_RETURNED;
}

static pr_system_real_status run_error(machine *m, uint16_t code)
{
    m->c->ax = code;
    m->c->ip = 0x00E2u;
    return PR_SYSTEM_REAL_RUNERROR;
}

pr_system_real_status pr_system_real_mul_1186(uint8_t *ram, size_t ram_size, pr_system_real_cpu *cpu)
{
    machine m;
    pr_system_real_status status;
    if (!valid(ram, ram_size, cpu, 0x1186u)) return PR_SYSTEM_REAL_ARGUMENT;
    m.r = ram; m.c = cpu;
    push16(&m, 0x1189u);                                                         /* 1186 CALL 0F33 */
    core_mul(&m);
    status = near_return(&m, 0x1189u);
    if (status != PR_SYSTEM_REAL_RETURNED) return status;
    if (cf(cpu)) return run_error(&m, 0x00CDu);                                  /* 1189 JB 11B4 */
    return far_return(&m);                                                       /* 118B RETF */
}

pr_system_real_status pr_system_real_from_long_119a(uint8_t *ram, size_t ram_size, pr_system_real_cpu *cpu)
{
    machine m;
    pr_system_real_status status;
    if (!valid(ram, ram_size, cpu, 0x119Au)) return PR_SYSTEM_REAL_ARGUMENT;
    m.r = ram; m.c = cpu;
    push16(&m, 0x119Du);                                                         /* 119A CALL 10D9 */
    core_from_long(&m);
    status = near_return(&m, 0x119Du);
    if (status != PR_SYSTEM_REAL_RETURNED) return status;
    return far_return(&m);                                                       /* 119D RETF */
}

pr_system_real_status pr_system_real_trunc_119e(uint8_t *ram, size_t ram_size, pr_system_real_cpu *cpu)
{
    machine m;
    pr_system_real_status status;
    if (!valid(ram, ram_size, cpu, 0x119Eu)) return PR_SYSTEM_REAL_ARGUMENT;
    m.r = ram; m.c = cpu;
    sethi(&cpu->cx, 0);                                                          /* 119E MOV CH,0 */
    push16(&m, 0x11A3u);                                                         /* 11A0 CALL 1118 */
    core_to_long(&m);
    status = near_return(&m, 0x11A3u);
    if (status != PR_SYSTEM_REAL_RETURNED) return status;
    if (cf(cpu)) return run_error(&m, 0x00CFu);                                  /* 11A3 JB 11AE */
    return far_return(&m);                                                       /* 11A5 RETF */
}
