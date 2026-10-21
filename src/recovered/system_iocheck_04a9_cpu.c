/* START.EXE 0AF8:04A9, Turbo Pascal SYSTEM I/O check, raw CPU (docs/re/SYSTEM_IOCHECK_04A9_CPU.md). */
#include "poolrad/recovered/system_iocheck_04a9_cpu.h"
#define MASK (PR_SYSTEM_IOCHECK_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word(const uint8_t *r, size_t at) { return (uint16_t)(r[at] | ((uint16_t)r[(at + 1u) & MASK] << 8)); }
static unsigned even(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) == 0; }
pr_system_iocheck_status pr_system_iocheck_04a9(uint8_t *ram, size_t size, pr_system_iocheck_cpu *c)
{
    uint16_t in_out_res;
    if (!ram || !c || size != PR_SYSTEM_IOCHECK_RAM_SIZE || c->ip != 0x04A9u || c->sp > 0xFFFCu) return PR_SYSTEM_IOCHECK_ARGUMENT;
    in_out_res = word(ram, physical(c->ds, 0x4426u));                               /* 04A9 CMP WORD [4426],0 */
    c->flags &= (uint16_t)~0x08D5u;                                                 /* CF, OF, AF clear: nothing to borrow from zero */
    if (even((uint8_t)in_out_res)) c->flags |= 0x04;
    if (in_out_res == 0) c->flags |= 0x40;
    if (in_out_res & 0x8000u) c->flags |= 0x80;
    if (in_out_res != 0) {                                                          /* 04AE JNE 04B1 taken */
        c->ax = word(ram, physical(c->ds, 0x4426u));                                /* 04B1 MOV AX,[4426] (reread) */
        c->ip = 0x00E2u;                                                            /* 04B4 JMP 00E2: RunError, never returns */
        return PR_SYSTEM_IOCHECK_RUNERROR;
    }
    c->ip = word(ram, physical(c->ss, c->sp)); c->cs = word(ram, physical(c->ss, (uint16_t)(c->sp + 2u)));   /* 04B0 RETF */
    c->sp = (uint16_t)(c->sp + 4u);
    return PR_SYSTEM_IOCHECK_RETURNED;
}
