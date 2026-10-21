/* RES_08AE [017B,01A5) raw CPU, stops at the four callees; docs/re/RES_08AE_ENTRY_017B_CPU.md. */
#include "poolrad/recovered/resident_08ae_entry_017b_cpu.h"
#define MASK (PR_R08AE_017B_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word(const uint8_t *r, size_t at) { return (uint16_t)(r[at] | ((uint16_t)r[(at + 1u) & MASK] << 8)); }
static void put(uint8_t *r, size_t at, uint16_t v) { r[at] = (uint8_t)v; r[(at + 1u) & MASK] = (uint8_t)(v >> 8); }
static void push(uint8_t *r, pr_r08ae_017b_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); put(r, physical(c->ss, c->sp), v); }
static uint16_t pop(const uint8_t *r, pr_r08ae_017b_cpu *c) { uint16_t v = word(r, physical(c->ss, c->sp)); c->sp = (uint16_t)(c->sp + 2u); return v; }
static unsigned even(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) == 0; }
/* CMP byte a,b: the six arithmetic flags of SUB. */
static void cmp8(pr_r08ae_017b_cpu *c, uint8_t a, uint8_t b) {
    uint8_t r = (uint8_t)(a - b);
    c->flags &= (uint16_t)~0x08D5u;
    if (a < b) c->flags |= 0x01;
    if ((a ^ b ^ r) & 0x10) c->flags |= 0x10;
    if ((a ^ b) & (a ^ r) & 0x80) c->flags |= 0x800;
    if (even(r)) c->flags |= 0x04;
    if (r == 0) c->flags |= 0x40;
    if (r & 0x80) c->flags |= 0x80;
}
/* PUSH CS; CALL near: the far-return frame, then IP = the callee. */
static void call(uint8_t *r, pr_r08ae_017b_cpu *c, uint16_t target, uint16_t return_ip) {
    push(r, c, c->cs); push(r, c, return_ip); c->ip = target;
}
static void zero_ax_push(uint8_t *r, pr_r08ae_017b_cpu *c) {
    c->ax = 0;                                                       /* XOR AX,AX */
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | 0x0044u);
    push(r, c, c->ax);                                               /* PUSH AX */
}
pr_r08ae_017b_status pr_r08ae_entry_017b_cpu(uint8_t *ram, size_t size, pr_r08ae_017b_cpu *c)
{
    uint8_t al;
    if (!ram || !c || size != PR_R08AE_017B_RAM_SIZE || c->sp < 0x0006u || c->sp > 0xFFFAu) return PR_R08AE_017B_ARGUMENT;
    switch (c->ip) {
    case 0x017B:
        push(ram, c, c->bp); c->bp = c->sp;                                          /* 017B PUSH BP; MOV BP,SP */
        al = ram[physical(c->ds, 0x5372u)];                                          /* 017E MOV AL,[5372] */
        c->ax = (uint16_t)((c->ax & 0xFF00u) | al);
        ram[physical(c->ds, 0x5373u)] = al;                                          /* 0181 MOV [5373],AL */
        cmp8(c, ram[physical(c->ds, 0x5372u)], 2u);                                  /* 0184 CMP BYTE [5372],2 */
        if (c->flags & 0x40u) goto epilogue;                                         /* 0189 JNE 018D not taken -> 018B JMP 01A3 */
        call(ram, c, 0x137Cu, 0x0191u); return PR_R08AE_017B_CALL_137C;              /* 018D PUSH CS; 018E CALL 137C */
    case 0x0191:
        zero_ax_push(ram, c); call(ram, c, 0x1742u, 0x0198u);                        /* 0191 XOR AX,AX; PUSH AX; 0194 PUSH CS; 0195 CALL 1742 */
        return PR_R08AE_017B_CALL_1742;
    case 0x0198:
        zero_ax_push(ram, c); call(ram, c, 0x17AEu, 0x019Fu);                        /* 0198 XOR AX,AX; PUSH AX; 019B PUSH CS; 019C CALL 17AE */
        return PR_R08AE_017B_CALL_17AE;
    case 0x019F:
        call(ram, c, 0x13B1u, 0x01A3u); return PR_R08AE_017B_CALL_13B1;              /* 019F PUSH CS; 01A0 CALL 13B1 */
    case 0x01A3:
    epilogue:
        c->bp = pop(ram, c);                                                         /* 01A3 POP BP */
        c->ip = pop(ram, c); c->cs = pop(ram, c);                                    /* 01A4 RETF */
        return PR_R08AE_017B_RETURN;
    default:
        return PR_R08AE_017B_ARGUMENT;
    }
}
