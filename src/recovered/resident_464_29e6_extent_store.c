#include "poolrad/recovered/resident_464_29e6_extent_store.h"

#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return ((size_t)segment << 4) + offset;
}

static uint16_t word_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8));
}

pr_r464_29e6_status pr_r464_29e6_extent_store(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu)
{
    static const uint8_t code[] = {
        0x8B, 0x46, 0xEA, 0x48, 0x8B, 0x7E, 0xE2,
        0xD1, 0xE7, 0x89, 0x85, 0x70, 0x4D
    };
    uint16_t end, slot;
    size_t target;

    if (ram == NULL || cpu == NULL || ram_size < PR_R464_29E6_RAM_BYTES)
        return PR_R464_29E6_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x29E6u ||
        cpu->ax != 0x138Fu || cpu->bx != 0x3E4Eu || cpu->cx != 0u ||
        cpu->dx != 0x14A0u || cpu->si != 0x3E68u || cpu->di != 0x0076u ||
        cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u ||
        cpu->ss != 0x1D16u || cpu->flags != 0x7202u ||
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
        word_at(ram, cpu->ds, 0x4DE6u) != 0u ||
        memcmp(ram + physical(cpu->cs, cpu->ip), code, sizeof code) != 0)
        return PR_R464_29E6_UNSUPPORTED_PATH;

    /* MOV AX,[BP-16]; DEC AX; MOV DI,[BP-1E]; SHL DI,1;
     * MOV DS:[DI+4D70],AX. AF after SHL is deliberately unspecified. */
    end = (uint16_t)(word_at(ram, cpu->ss, 0x3E6Au) - 1u);
    slot = (uint16_t)(word_at(ram, cpu->ss, 0x3E62u) << 1);
    target = physical(cpu->ds, (uint16_t)(slot + 0x4D70u));
    ram[target] = (uint8_t)end;
    ram[target + 1u] = (uint8_t)(end >> 8);
    cpu->ax = end;
    cpu->di = slot;
    cpu->ip = 0x29F3u;
    return PR_R464_29E6_OK;
}
