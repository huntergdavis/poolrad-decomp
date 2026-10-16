#include "poolrad/recovered/resident_464_29d7_extent_branch.h"

#include <string.h>

enum {
    CF = 0x0001u, PF = 0x0004u, AF = 0x0010u,
    ZF = 0x0040u, SF = 0x0080u, OF = 0x0800u,
    ARITH = CF | PF | AF | ZF | SF | OF
};

static size_t physical(uint16_t segment, uint16_t offset)
{
    return ((size_t)segment << 4) + offset;
}

static uint16_t word_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8));
}

static int even_parity(uint8_t value)
{
    unsigned bits = 0;
    while (value != 0u) {
        bits ^= value & 1u;
        value >>= 1;
    }
    return bits == 0u;
}

static uint16_t cmp_flags16(uint16_t flags, uint16_t left, uint16_t right)
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

pr_r464_29d7_status pr_r464_29d7_extent_branch(
    const uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu)
{
    static const uint8_t code[] = {
        0x8B, 0x46, 0xEA, 0x48, 0x8B, 0x7E, 0xE2, 0xD1,
        0xE7, 0x3B, 0x85, 0x70, 0x4D, 0x76, 0x0D
    };
    uint16_t end, row_slot, extent;

    if (ram == NULL || cpu == NULL || ram_size < PR_R464_29D7_RAM_BYTES)
        return PR_R464_29D7_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x29D7u ||
        cpu->ax != 0x000Bu || cpu->bx != 0x3E4Eu || cpu->cx != 0u ||
        cpu->dx != 0x14A0u || cpu->si != 0x3E68u || cpu->di != 0x13AFu ||
        cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u ||
        cpu->ss != 0x1D16u || cpu->flags != 0x7246u ||
        word_at(ram, cpu->ss, 0x3E4Eu) != 0x29CBu ||
        word_at(ram, cpu->ss, 0x3E50u) != 0x0C88u ||
        word_at(ram, cpu->ss, 0x3E52u) != 4u ||
        word_at(ram, cpu->ss, 0x3E54u) != 0x13ABu ||
        word_at(ram, cpu->ss, 0x3E56u) != 0x3DE4u ||
        word_at(ram, cpu->ss, 0x3E58u) != 0x3E64u ||
        word_at(ram, cpu->ss, 0x3E5Au) != 0x1D16u ||
        ram[physical(cpu->ss, 0x3E5Cu)] != 0x0Bu ||
        word_at(ram, cpu->ss, 0x3E62u) != 0x003Bu ||
        word_at(ram, cpu->ss, 0x3E6Au) != 0x1390u ||
        ram[physical(cpu->ds, 0x70C7u)] != 0x0Bu ||
        memcmp(ram + physical(cpu->cs, cpu->ip), code, sizeof code) != 0)
        return PR_R464_29D7_UNSUPPORTED_PATH;

    /* MOV AX,[BP-16]; DEC AX; MOV DI,[BP-1E]; SHL DI,1;
     * CMP AX,DS:[DI+4D70]; JBE +29F3. CMP owns the final flags. */
    end = (uint16_t)(word_at(ram, cpu->ss, 0x3E6Au) - 1u);
    row_slot = (uint16_t)(word_at(ram, cpu->ss, 0x3E62u) << 1);
    extent = word_at(ram, cpu->ds, (uint16_t)(row_slot + 0x4D70u));
    cpu->ax = end;
    cpu->di = row_slot;
    cpu->flags = cmp_flags16(cpu->flags, end, extent);
    cpu->ip = end <= extent ? 0x29F3u : 0x29E6u;
    return PR_R464_29D7_OK;
}
