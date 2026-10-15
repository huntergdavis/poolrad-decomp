#include "poolrad/recovered/resident_464_2632_fourth_prefix.h"

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

static void push(uint8_t *ram, pr_r464_2632_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    ram[physical(cpu->ss, cpu->sp)] = (uint8_t)value;
    ram[physical(cpu->ss, (uint16_t)(cpu->sp + 1u))] =
        (uint8_t)(value >> 8);
}

static uint8_t even_parity(uint8_t value)
{
    unsigned bits = 0;
    while (value != 0u) {
        bits ^= value & 1u;
        value >>= 1;
    }
    return (uint8_t)(bits == 0u);
}

static uint16_t sub_flags16(uint16_t flags, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    flags &= (uint16_t)~ARITH;
    if (left < right) flags |= CF;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u) flags |= OF;
    return flags;
}

pr_r464_2632_status pr_r464_2632_fourth_prefix(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu)
{
    static const pr_r464_2632_cpu measured_pre = {
        0x2D14u, 0x020Au, 6u, 0x3E4Eu, 0u, 0x14A0u, 0x3E68u,
        0x260Au, 0x3E98u, 0x3E86u, 0x14A0u, 0x3DE4u, 0x1D16u, 0x7246u
    };
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (memcmp(cpu, &measured_pre, sizeof(*cpu)) != 0)
        return PR_R464_2632_UNSUPPORTED_PATH;

    /* +020A far call, then +2632..+2645. +2646 is not executed. */
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x020Fu);
    cpu->cs = 0x0C88u;
    cpu->ip = 0x2632u;
    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->flags = sub_flags16(cpu->flags, cpu->sp, 0x24u);
    cpu->sp = (uint16_t)(cpu->sp - 0x24u);
    cpu->di = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Au));
    cpu->es = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));
    push(ram, cpu, cpu->es);
    push(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 8u);
    push(ram, cpu, cpu->ss);
    push(ram, cpu, cpu->di);
    cpu->ax = 8u;
    push(ram, cpu, cpu->ax);
    cpu->ip = 0x2646u;
    return PR_R464_2632_OK;
}
