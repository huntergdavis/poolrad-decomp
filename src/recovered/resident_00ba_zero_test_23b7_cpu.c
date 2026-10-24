/* START.EXE resident 00BA +23B7: the character field-zero test, full CPU state (docs/re/RES_00BA_23B7_CPU.md). */
#include "poolrad/recovered/resident_00ba_zero_test_23b7_cpu.h"

#define MASK (PR_RES_23B7_RAM_SIZE - 1u)
#define ARITH 0x08D5u
typedef pr_res_23b7_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 0x0004u; }
static void set_arith(cpu_t *c, uint16_t f) { c->flags = (uint16_t)((c->flags & ~ARITH) | f); }
static uint16_t sub16(cpu_t *c, uint16_t a, uint16_t b)
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
static void les(const uint8_t *ram, cpu_t *c, uint16_t seg, uint16_t off)   /* LES as pinned DOSBox-X (prefix_none.h C4h): segment word at (off+2) mod 64K */
{ uint16_t o = rd16(ram, seg, off); c->es = rd16(ram, seg, (uint16_t)(off + 2u)); c->di = o; }

pr_res_23b7_status pr_res_00ba_zero_test_23b7_cpu(uint8_t *ram, size_t ram_size, pr_res_23b7_cpu *c)
{
    uint16_t ret_ip, ret_cs; uint8_t value;
    if (ram == NULL || c == NULL || ram_size != PR_RES_23B7_RAM_SIZE || c->ip != 0x23B7u || c->sp < 0x0004u || c->sp > 0xFFF8u)
        return PR_RES_23B7_ARGUMENT;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    push(ram, c, c->bp); c->bp = c->sp;                                                     /* 23B7 PUSH BP; 23B8 MOV BP,SP */
    c->sp = sub16(c, c->sp, 2u);                                                            /* 23BA SUB SP,2 */
    les(ram, c, c->ss, (uint16_t)(c->bp + 6u));                                             /* 23BD LES DI,[BP+6] */
    value = rd8(ram, c->es, (uint16_t)(c->di + 0x010Eu)); cmp8(c, value, 0u);               /* 23C0 CMP BYTE ES:[DI+10E],0 */
    wr8(ram, c->ss, (uint16_t)(c->bp - 1u), (c->flags & 0x0040u) ? 1u : 0u);                /* 23C6 JNE 23CE; 23C8 [BP-1]:=1 / 23CE [BP-1]:=0 */
    c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ss, (uint16_t)(c->bp - 1u)));         /* 23D2 MOV AL,[BP-1] */
    c->sp = c->bp; c->bp = pop(ram, c);                                                     /* 23D5 MOV SP,BP; 23D7 POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u);               /* 23D8 RETF 4 */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_RES_23B7_TRANSFER;
    return PR_RES_23B7_RETURNED;
}
