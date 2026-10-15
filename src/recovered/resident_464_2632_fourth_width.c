#include "poolrad/recovered/resident_464_2632_fourth_width.h"

#include <string.h>

enum {
    CF = 0x0001u, PF = 0x0004u, AF = 0x0010u,
    ZF = 0x0040u, SF = 0x0080u, OF = 0x0800u,
    ARITH = CF | PF | AF | ZF | SF | OF
};

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint16_t word_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return (uint16_t)(ram[physical(segment, offset)] |
        ((uint16_t)ram[physical(segment, (uint16_t)(offset + 1u))] << 8));
}

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t value)
{
    ram[physical(segment, offset)] = (uint8_t)value;
    ram[physical(segment, (uint16_t)(offset + 1u))] = (uint8_t)(value >> 8);
}

static uint16_t sub_flags16(uint16_t flags, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    uint8_t low = (uint8_t)result;
    unsigned parity = 0;
    while (low != 0u) { parity ^= low & 1u; low >>= 1; }
    flags &= (uint16_t)~ARITH;
    if (left < right) flags |= CF;
    if (parity == 0u) flags |= PF;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u) flags |= OF;
    return flags;
}

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_width(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    uint16_t x, count, row, width, sum, remainder;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    x = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x16u));
    count = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x12u));
    row = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x14u));
    width = word_at(ram, cpu->es, (uint16_t)(cpu->di + 2u));
    if (cpu->cs != 0x0C88u || cpu->ip != 0x2699u ||
        cpu->bx != 0u || cpu->dx != 0u || cpu->si != 0x2612u ||
        cpu->di != 8u || cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u || cpu->ss != 0x1D16u ||
        cpu->flags != 0x7246u || count > 64u || x > 0x7FFFu ||
        (uint32_t)x + count > 0x7FFFu || row > 0x7FFFu ||
        word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)) != 8u ||
        word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)) != 0x3DE4u ||
        cpu->cx != (uint16_t)(x + count) || cpu->ax != width ||
        width > 0x7FFFu || width < cpu->cx)
        return PR_R464_2632_UNSUPPORTED_PATH;
    sum = cpu->cx;
    remainder = (uint16_t)(width - sum);

    /* +2699 JL and +269B JG both fall through when DX == BX == 0. */
    observe(0x269Bu, ram, cpu, observer, context);
    observe(0x269Du, ram, cpu, observer, context);
    cpu->flags = sub_flags16(cpu->flags, cpu->ax, cpu->cx);
    observe(0x269Fu, ram, cpu, observer, context);
    /* Width >= sum: +269F JAE branches to +26CD. */
    observe(0x26CDu, ram, cpu, observer, context);

    /* +26CD..+26F7: zero first local; recompute sum and width remainder. */
    cpu->ax = 0u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH) | PF | ZF);
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Cu), 0u);
    cpu->ax = count;
    cpu->dx = 0u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH) | PF | ZF);
    cpu->cx = count;
    cpu->bx = 0u;
    cpu->ax = x;
    cpu->dx = 0u;
    cpu->ax = sum;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH) | PF | ZF);
    cpu->cx = sum;
    cpu->bx = 0u;
    cpu->di = 8u;
    cpu->es = 0x3DE4u;
    cpu->ax = width;
    cpu->dx = 0u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH) | PF | ZF);
    cpu->ax = remainder;
    cpu->flags = sub_flags16(cpu->flags, 0u, 0u); /* SBB DX,BX, carry=0. */
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 0x10u), remainder);
    observe(0x26F7u, ram, cpu, observer, context);
    cpu->flags = sub_flags16(cpu->flags, row, 0u);
    observe(0x26FBu, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
