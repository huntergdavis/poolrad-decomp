/* RES_08AE [01A5,01D2) raw CPU, stops at the two callees; docs/re/RES_08AE_STATE_01A5_CPU.md. */
#include "poolrad/recovered/resident_08ae_state_01a5_cpu.h"
#define MASK (PR_R08AE_01A5_RAM_SIZE - 1u)
typedef pr_r08ae_01a5_cpu cpu_t;
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word(const uint8_t *r, size_t at) { return (uint16_t)(r[at] | ((uint16_t)r[(at + 1u) & MASK] << 8)); }
static void put(uint8_t *r, size_t at, uint16_t v) { r[at] = (uint8_t)v; r[(at + 1u) & MASK] = (uint8_t)(v >> 8); }
static void push(uint8_t *r, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); put(r, physical(c->ss, c->sp), v); }
static uint16_t pop(const uint8_t *r, cpu_t *c) { uint16_t v = word(r, physical(c->ss, c->sp)); c->sp = (uint16_t)(c->sp + 2u); return v; }
static unsigned even(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) == 0; }
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
    if (even((uint8_t)r)) c->flags |= 0x04;
    if (r == 0) c->flags |= 0x40;
    if (r & sign) c->flags |= 0x80;
}
static int zf(const cpu_t *c) { return (c->flags & 0x40u) != 0; }
/* PUSH CS; CALL near: the far-return frame, then IP = the callee. */
static void call(uint8_t *r, cpu_t *c, uint16_t target, uint16_t return_ip) { push(r, c, c->cs); push(r, c, return_ip); c->ip = target; }

pr_r08ae_01a5_status pr_r08ae_state_01a5_cpu(uint8_t *ram, size_t size, cpu_t *c)
{
    if (!ram || !c || size != PR_R08AE_01A5_RAM_SIZE || c->sp < 0x0006u || c->sp > 0xFFFAu) return PR_R08AE_01A5_ARGUMENT;
    switch (c->ip) {
    case 0x01A5: goto L01A5;
    case 0x01C2: goto L01C8;
    case 0x01C8: goto L01C8;
    default: return PR_R08AE_01A5_ARGUMENT;
    }
L01A5:
    push(ram, c, c->bp); c->bp = c->sp;                                              /* 01A5 PUSH BP; MOV BP,SP */
    cmp(c, ram[physical(c->ds, 0x5372u)], 2u, 8);                                    /* 01A8 CMP BYTE [5372],2 */
    if (zf(c)) goto L01CE;                                                           /* 01AD JNE 01B1 not taken; 01AF JMP 01CE */
    cmp(c, word(ram, physical(c->ss, (uint16_t)(c->bp + 6u))), 0u, 16);              /* 01B1 CMP WORD [BP+6],0 */
    if (zf(c)) {                                                                     /* 01B5 JBE 01C4 (CF is clear after CMP x,0) */
        call(ram, c, 0x137Cu, 0x01C8u); return PR_R08AE_01A5_CALL_137C;              /* 01C4 PUSH CS; 01C5 CALL 137C */
    }
    cmp(c, word(ram, physical(c->ds, 0x5374u)), 0u, 16);                             /* 01B7 CMP WORD [5374],0 */
    if (!zf(c)) goto L01C8;                                                          /* 01BC JNE 01C2; 01C2 JMP 01C8 */
    call(ram, c, 0x13B1u, 0x01C2u); return PR_R08AE_01A5_CALL_13B1;                  /* 01BE PUSH CS; 01BF CALL 13B1 */
L01C8:
    c->ax = word(ram, physical(c->ss, (uint16_t)(c->bp + 6u)));                      /* 01C8 MOV AX,[BP+6] */
    put(ram, physical(c->ds, 0x5374u), c->ax);                                       /* 01CB MOV [5374],AX */
L01CE:
    c->bp = pop(ram, c);                                                             /* 01CE POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                                        /* 01CF RETF 2 */
    c->sp = (uint16_t)(c->sp + 2u);
    return PR_R08AE_01A5_RETURN;
}
