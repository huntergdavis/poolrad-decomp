/* Original unit 0x7F +0000: the far-pointer wrapper around resident 00BA:13AB that adds 05DCh to its result. */
#include "poolrad/recovered/overlay_u7f_entry0_0000_cpu.h"
#define RAM_MASK 0xFFFFFu
static size_t phys(uint16_t seg, uint16_t off)
{ return (((size_t)seg << 4u) + off) & RAM_MASK; }
static uint8_t r8(const uint8_t *ram, uint16_t seg, uint16_t off)
{ return ram[phys(seg, off)]; }
static uint16_t r16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = phys(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & RAM_MASK] << 8u)); }
static void w16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{ size_t p = phys(seg, off); ram[p] = (uint8_t)value; ram[(p + 1u) & RAM_MASK] = (uint8_t)(value >> 8u); }
static void push(uint8_t *ram, pr_res00ba_cpu *c, uint16_t value)
{ c->sp = (uint16_t)(c->sp - 2u); w16(ram, c->ss, c->sp, value); }
static uint16_t pop(const uint8_t *ram, pr_res00ba_cpu *c)
{ uint16_t value = r16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return value; }
static uint16_t parity(uint8_t v)
{ v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 4u; }
static uint16_t arith16(pr_res00ba_cpu *c, uint16_t a, uint16_t b, int subtract)
{
    uint32_t wide = subtract ? (uint32_t)a - b : (uint32_t)a + b;
    uint16_t r = (uint16_t)(wide & 0xFFFFu), f = parity((uint8_t)r);
    if (subtract ? a < b : wide > 0xFFFFu) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((subtract ? ((a ^ b) & (a ^ r)) : (~(a ^ b) & (a ^ r))) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}

pr_u7f_0000_status pr_u7f_entry0_0000_cpu(uint8_t *ram, size_t ram_size, pr_res00ba_cpu *c, size_t *budget, pr_u7f_0000_diagnostic *diagnostic)
{
    pr_u7f_0000_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint16_t cs;
    if (ram == NULL || c == NULL || budget == NULL || ram_size != (1u << 20) || c->ip != 0x0000u || c->sp < 0x0040u || c->sp > 0xFFF0u)
        return PR_U7F_0000_ARGUMENT;
    d->callee_status = -1; d->site = 0;
    cs = c->cs;
#define STEP() do { if (*budget == 0u) return PR_U7F_0000_LIMIT; --*budget; } while (0)
    STEP(); push(ram, c, c->bp); c->ip = 0x0001u;                                            /* 0000 PUSH BP */
    STEP(); c->bp = c->sp; c->ip = 0x0003u;                                                  /* 0001 MOV BP,SP */
    STEP(); c->sp = (uint16_t)arith16(c, c->sp, 2u, 1); c->ip = 0x0006u;                     /* 0003 SUB SP,2 */
    STEP(); push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 8u))); c->ip = 0x0009u;          /* 0006 PUSH [BP+8]: the pointer's segment */
    STEP(); push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 6u))); c->ip = 0x000Cu;          /* 0009 PUSH [BP+6]: its offset */
    if (*budget == 0u) return PR_U7F_0000_LIMIT;                                             /* 000C LCALL 00BAh:13ABh (the segment word at 000F is relocated) */
    d->site = 0x000Cu;
    if (r8(ram, cs, 0x000Cu) != 0x9Au || r16(ram, cs, 0x000Du) != 0x13ABu) return PR_U7F_0000_CODE;   /* IP is still 000C, before the return frame */
    --*budget;
    {
        uint16_t resident = r16(ram, cs, 0x000Fu);
        pr_res00ba_status s;
        push(ram, c, cs); push(ram, c, 0x0011u); c->cs = resident; c->ip = 0x13ABu;
        s = pr_res00ba_helpers_cpu(ram, ram_size, c, budget);
        if (s != PR_RES00BA_OK) { d->callee_status = (int)s; return s == PR_RES00BA_LIMIT ? PR_U7F_0000_LIMIT : PR_U7F_0000_CALLEE; }
        if (c->cs != cs || c->ip != 0x0011u) return PR_U7F_0000_TRANSFER;
    }
    STEP(); c->ax = arith16(c, c->ax, 0x05DCu, 0); c->ip = 0x0014u;                          /* 0011 ADD AX,5DCh */
    STEP(); w16(ram, c->ss, (uint16_t)(c->bp - 2u), c->ax); c->ip = 0x0017u;                 /* 0014 MOV [BP-2],AX */
    STEP(); c->ax = r16(ram, c->ss, (uint16_t)(c->bp - 2u)); c->ip = 0x001Au;                /* 0017 MOV AX,[BP-2] */
    STEP(); c->sp = c->bp; c->ip = 0x001Cu;                                                  /* 001A MOV SP,BP */
    STEP(); c->bp = pop(ram, c); c->ip = 0x001Du;                                            /* 001C POP BP */
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u);        /* 001D RETF 4 */
#undef STEP
    return PR_U7F_0000_RETURNED;
}
