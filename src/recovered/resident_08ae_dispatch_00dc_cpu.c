/* RES_08AE [00DC,017B) raw CPU, stops at the six callees; docs/re/RES_08AE_DISPATCH_00DC_CPU.md. */
#include "poolrad/recovered/resident_08ae_dispatch_00dc_cpu.h"
#define MASK (PR_R08AE_00DC_RAM_SIZE - 1u)
typedef pr_r08ae_00dc_cpu cpu_t;
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word(const uint8_t *r, size_t at) { return (uint16_t)(r[at] | ((uint16_t)r[(at + 1u) & MASK] << 8)); }
static void put(uint8_t *r, size_t at, uint16_t v) { r[at] = (uint8_t)v; r[(at + 1u) & MASK] = (uint8_t)(v >> 8); }
static void push(uint8_t *r, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); put(r, physical(c->ss, c->sp), v); }
static uint16_t pop(const uint8_t *r, cpu_t *c) { uint16_t v = word(r, physical(c->ss, c->sp)); c->sp = (uint16_t)(c->sp + 2u); return v; }
static unsigned even(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) == 0; }
static void szp(cpu_t *c, uint16_t r, uint16_t sign) {
    if (even((uint8_t)r)) c->flags |= 0x04;
    if (r == 0) c->flags |= 0x40;
    if (r & sign) c->flags |= 0x80;
}
/* CMP a,b on bytes (bits 8) or words (bits 16): the six arithmetic flags of SUB. */
static void cmp(cpu_t *c, uint16_t a, uint16_t b, unsigned bits) {
    uint32_t mask = bits == 8 ? 0xFFu : 0xFFFFu;
    uint16_t sign = bits == 8 ? 0x80u : 0x8000u, r;
    a = (uint16_t)(a & mask); b = (uint16_t)(b & mask);
    r = (uint16_t)(((uint32_t)a - b) & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (a < b) c->flags |= 0x01;
    if ((a ^ b ^ r) & 0x10) c->flags |= 0x10;
    if ((a ^ b) & (a ^ r) & sign) c->flags |= 0x800;
    szp(c, r, sign);
}
static uint16_t dec16(cpu_t *c, uint16_t v) {                     /* DEC word: CF is kept */
    uint16_t r = (uint16_t)(v - 1u), carry = (uint16_t)(c->flags & 1u);
    c->flags &= (uint16_t)~0x08D5u;
    if ((r & 0x0Fu) == 0x0Fu) c->flags |= 0x10;
    if (r == 0x7FFFu) c->flags |= 0x800;
    szp(c, r, 0x8000);
    c->flags |= carry;
    return r;
}
static int zf(const cpu_t *c) { return (c->flags & 0x40u) != 0; }
static int jbe(const cpu_t *c) { return (c->flags & 0x41u) != 0; }     /* CF or ZF */
/* PUSH CS; CALL near: the far-return frame, then IP = the callee. */
static void call(uint8_t *r, cpu_t *c, uint16_t target, uint16_t return_ip) { push(r, c, c->cs); push(r, c, return_ip); c->ip = target; }
static void zero_push(uint8_t *r, cpu_t *c) {                      /* XOR AX,AX; PUSH AX */
    c->ax = 0; c->flags = (uint16_t)((c->flags & ~0x08D5u) | 0x0044u); push(r, c, c->ax);
}
static uint16_t arg(const uint8_t *r, const cpu_t *c) { return word(r, physical(c->ss, (uint16_t)(c->bp + 6u))); }   /* MOV AX,[BP+6] */

pr_r08ae_00dc_status pr_r08ae_dispatch_00dc_cpu(uint8_t *ram, size_t size, cpu_t *c)
{
    if (!ram || !c || size != PR_R08AE_00DC_RAM_SIZE || c->sp < 0x0008u || c->sp > 0xFFFAu) return PR_R08AE_00DC_ARGUMENT;
    switch (c->ip) {
    case 0x00DC: goto L00DC;
    case 0x00F4: goto L00F4;
    case 0x0103: goto L0103;
    case 0x010A: goto L010A;
    case 0x0111: goto L0156;
    case 0x0120: goto L0156;
    case 0x012E: goto L012E;
    case 0x0135: goto L0156;
    case 0x0147: goto L0156;
    case 0x0156: goto L0156;
    case 0x0165: goto L0165;
    case 0x016C: goto L016C;
    case 0x0173: goto L0173;
    case 0x0177: goto L0177;
    default: return PR_R08AE_00DC_ARGUMENT;
    }
L00DC:
    push(ram, c, c->bp); c->bp = c->sp;                                              /* 00DC PUSH BP; MOV BP,SP */
    cmp(c, ram[physical(c->ds, 0x5372u)], 2u, 8);                                    /* 00DF CMP BYTE [5372],2 */
    if (zf(c)) goto L0177;                                                           /* 00E4 JNE 00E9 not taken; 00E6 JMP 0177 */
    cmp(c, word(ram, physical(c->ds, 0x5374u)), 0u, 16);                             /* 00E9 CMP WORD [5374],0 */
    if (jbe(c)) goto L00F4;                                                          /* 00EE JBE 00F4 */
    call(ram, c, 0x137Cu, 0x00F4u); return PR_R08AE_00DC_CALL_137C;                  /* 00F0 PUSH CS; 00F1 CALL 137C */
L00F4:
    c->ax = arg(ram, c);                                                             /* 00F4 MOV AX,[BP+6] */
    cmp(c, c->ax, 0u, 16);                                                           /* 00F7 CMP AX,0 */
    if (!zf(c)) goto L0113;                                                          /* 00FA JNE 0113 */
    zero_push(ram, c); call(ram, c, 0x17AEu, 0x0103u); return PR_R08AE_00DC_CALL_17AE;   /* 00FC..0100 */
L0103:
    zero_push(ram, c); call(ram, c, 0x1742u, 0x010Au); return PR_R08AE_00DC_CALL_1742;   /* 0103..0107 */
L010A:
    zero_push(ram, c); call(ram, c, 0x181Au, 0x0111u); return PR_R08AE_00DC_CALL_181A;   /* 010A..010E; 0111 JMP 0156 */
L0113:
    cmp(c, c->ax, 1u, 16);                                                           /* 0113 CMP AX,1 */
    if (!zf(c)) goto L0122;                                                          /* 0116 JNE 0122 */
    c->ax = 1u; push(ram, c, c->ax); call(ram, c, 0x181Au, 0x0120u); return PR_R08AE_00DC_CALL_181A;   /* 0118..011D; 0120 JMP 0156 */
L0122:
    cmp(c, c->ax, 0x00FFu, 16);                                                      /* 0122 CMP AX,00FF */
    if (!zf(c)) goto L0137;                                                          /* 0125 JNE 0137 */
    zero_push(ram, c); call(ram, c, 0x17AEu, 0x012Eu); return PR_R08AE_00DC_CALL_17AE;   /* 0127..012B */
L012E:
    zero_push(ram, c); call(ram, c, 0x1742u, 0x0135u); return PR_R08AE_00DC_CALL_1742;   /* 012E..0132; 0135 JMP 0156 */
L0137:
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, 0x5372u)]);            /* 0137 MOV AL,[5372] */
    cmp(c, c->ax, 1u, 8);                                                            /* 013A CMP AL,1 */
    if (!zf(c)) goto L0149;                                                          /* 013C JNE 0149 */
    c->ax = arg(ram, c); c->ax = dec16(c, c->ax); push(ram, c, c->ax);               /* 013E MOV AX,[BP+6]; DEC AX; PUSH AX */
    call(ram, c, 0x1742u, 0x0147u); return PR_R08AE_00DC_CALL_1742;                  /* 0143 PUSH CS; 0144 CALL 1742; 0147 JMP 0156 */
L0149:
    cmp(c, c->ax, 0u, 8);                                                            /* 0149 CMP AL,0 */
    if (!zf(c)) goto L0156;                                                          /* 014B JNE 0156 */
    c->ax = arg(ram, c); c->ax = dec16(c, c->ax); push(ram, c, c->ax);               /* 014D MOV AX,[BP+6]; DEC AX; PUSH AX */
    call(ram, c, 0x17AEu, 0x0156u); return PR_R08AE_00DC_CALL_17AE;                  /* 0152 PUSH CS; 0153 CALL 17AE */
L0156:
    cmp(c, word(ram, physical(c->ds, 0x5374u)), 0u, 16);                             /* 0156 CMP WORD [5374],0 */
    if (jbe(c)) goto L0177;                                                          /* 015B JBE 0177 */
    c->ax = 0x000Au; push(ram, c, c->ax); call(ram, c, 0x006Cu, 0x0165u); return PR_R08AE_00DC_CALL_006C;   /* 015D..0162 */
L0165:
    zero_push(ram, c); call(ram, c, 0x17AEu, 0x016Cu); return PR_R08AE_00DC_CALL_17AE;   /* 0165..0169 */
L016C:
    zero_push(ram, c); call(ram, c, 0x1742u, 0x0173u); return PR_R08AE_00DC_CALL_1742;   /* 016C..0170 */
L0173:
    call(ram, c, 0x13B1u, 0x0177u); return PR_R08AE_00DC_CALL_13B1;                  /* 0173 PUSH CS; 0174 CALL 13B1 */
L0177:
    c->bp = pop(ram, c);                                                             /* 0177 POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                                        /* 0178 RETF 2 */
    c->sp = (uint16_t)(c->sp + 2u);
    return PR_R08AE_00DC_RETURN;
}
