#include "poolrad/recovered/resident_464_second_postreturn_29cb.h"

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

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8);
}

pr_r464_second_postreturn_status pr_r464_second_postreturn_29cb(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu)
{
    static const uint8_t continuation[] = {
        0x83, 0x46, 0xEA, 0x04, 0xA0, 0xC7, 0x70,
        0x3A, 0x46, 0xDC, 0x75, 0xD4
    };
    static const uint8_t call[] = {0x9A, 0x85, 0x16, 0x1C, 0x13};
    static const uint8_t move[] = {
        0x8B, 0xDC, 0x8C, 0xDA, 0x36, 0xC5, 0x77, 0x0A,
        0x36, 0xC4, 0x7F, 0x06, 0x36, 0x8B, 0x4F, 0x04,
        0xFC, 0x3B, 0xF7, 0x73, 0x07, 0x03, 0xF1, 0x03,
        0xF9, 0x4E, 0x4F, 0xFD, 0xF3, 0xA4, 0x8E, 0xDA,
        0xCA, 0x0A, 0x00
    };
    static const uint8_t mask[] = {0x00, 0xFF, 0xFF, 0x00};

    if (ram == NULL || cpu == NULL ||
        ram_size < PR_R464_SECOND_POSTRETURN_RAM_BYTES)
        return PR_R464_SECOND_POSTRETURN_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x29CBu ||
        cpu->ax != 4u || cpu->bx != 0x3E4Eu || cpu->cx != 0u ||
        cpu->dx != 0x14A0u || cpu->si != 0x3E68u || cpu->di != 0x1487u ||
        cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u ||
        cpu->ss != 0x1D16u || cpu->flags != 0x7206u ||
        word_at(ram, cpu->ss, 0x3E4Eu) != 0x29CBu ||
        word_at(ram, cpu->ss, 0x3E50u) != 0x0C88u ||
        word_at(ram, cpu->ss, 0x3E52u) != 4u ||
        word_at(ram, cpu->ss, 0x3E54u) != 0x1483u ||
        word_at(ram, cpu->ss, 0x3E56u) != 0x3DE4u ||
        word_at(ram, cpu->ss, 0x3E58u) != 0x3E64u ||
        word_at(ram, cpu->ss, 0x3E5Au) != 0x1D16u ||
        word_at(ram, cpu->ss, 0x3E6Au) != 0x1464u ||
        ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x24u))] != 0x0Bu ||
        ram[physical(cpu->ds, 0x70C7u)] != 2u ||
        memcmp(ram + physical(cpu->ss, 0x3E64u), mask,
               sizeof mask) != 0 ||
        memcmp(ram + physical(cpu->es, 0x147Fu), mask,
               sizeof mask) != 0 ||
        memcmp(ram + physical(cpu->es, 0x1483u), mask,
               sizeof mask) != 0 ||
        memcmp(ram + physical(cpu->cs, 0x29C6u), call,
               sizeof call) != 0 ||
        memcmp(ram + physical(0x131Cu, 0x1685u), move,
               sizeof move) != 0 ||
        memcmp(ram + physical(cpu->cs, cpu->ip), continuation,
               sizeof continuation) != 0)
        return PR_R464_SECOND_POSTRETURN_UNSUPPORTED_PATH;

    /* ADD [BP-16],4; MOV AL,DS:70C7; CMP AL,[BP-24]; JNE +29AB. */
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 0x16u),
             (uint16_t)(word_at(ram, cpu->ss,
                               (uint16_t)(cpu->bp - 0x16u)) + 4u));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         ram[physical(cpu->ds, 0x70C7u)]);
    cpu->flags = 0x7293u; /* 02-0B: CF,AF,SF; JNE is taken. */
    cpu->ip = (uint16_t)(0x29D7u + (int8_t)continuation[11]);
    return PR_R464_SECOND_POSTRETURN_OK;
}
