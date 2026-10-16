#include "poolrad/recovered/resident_464_third_return_29cb.h"

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

pr_r464_third_return_status pr_r464_third_return_29cb(
    const uint8_t *ram, size_t ram_size, const pr_r464_2632_cpu *cpu)
{
    static const uint8_t call[] = {0x9A, 0x85, 0x16, 0x1C, 0x13};
    static const uint8_t move[] = {
        0x8B, 0xDC, 0x8C, 0xDA, 0x36, 0xC5, 0x77, 0x0A,
        0x36, 0xC4, 0x7F, 0x06, 0x36, 0x8B, 0x4F, 0x04,
        0xFC, 0x3B, 0xF7, 0x73, 0x07, 0x03, 0xF1, 0x03,
        0xF9, 0x4E, 0x4F, 0xFD, 0xF3, 0xA4, 0x8E, 0xDA,
        0xCA, 0x0A, 0x00
    };
    static const uint8_t mask[] = {0x00, 0xFF, 0xFF, 0x00};

    if (ram == NULL || cpu == NULL || ram_size < PR_R464_THIRD_RETURN_RAM_BYTES)
        return PR_R464_THIRD_RETURN_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x29CBu ||
        cpu->ax != 4u || cpu->bx != 0x3E4Eu || cpu->cx != 0u ||
        cpu->dx != 0x14A0u || cpu->si != 0x3E68u || cpu->di != 0x148Bu ||
        cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u ||
        cpu->ss != 0x1D16u || cpu->flags != 0x7216u ||
        word_at(ram, cpu->ss, 0x3E4Eu) != 0x29CBu ||
        word_at(ram, cpu->ss, 0x3E50u) != 0x0C88u ||
        word_at(ram, cpu->ss, 0x3E52u) != 4u ||
        word_at(ram, cpu->ss, 0x3E54u) != 0x1487u ||
        word_at(ram, cpu->ss, 0x3E56u) != 0x3DE4u ||
        word_at(ram, cpu->ss, 0x3E58u) != 0x3E64u ||
        word_at(ram, cpu->ss, 0x3E5Au) != 0x1D16u ||
        ram[physical(cpu->ds, 0x70C7u)] != 3u ||
        word_at(ram, cpu->ss, 0x3E6Au) != 0x1468u ||
        memcmp(ram + physical(cpu->cs, 0x29C6u), call, sizeof call) != 0 ||
        memcmp(ram + physical(0x131Cu, 0x1685u), move, sizeof move) != 0 ||
        memcmp(ram + physical(cpu->ss, 0x3E64u), mask, sizeof mask) != 0 ||
        memcmp(ram + physical(cpu->es, 0x147Fu), mask, sizeof mask) != 0 ||
        memcmp(ram + physical(cpu->es, 0x1483u), mask, sizeof mask) != 0 ||
        memcmp(ram + physical(cpu->es, 0x1487u), mask, sizeof mask) != 0)
        return PR_R464_THIRD_RETURN_UNSUPPORTED_PATH;
    return PR_R464_THIRD_RETURN_OK;
}
