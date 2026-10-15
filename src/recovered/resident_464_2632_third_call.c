#include "poolrad/recovered/resident_464_2632_third_call.h"

#include <string.h>

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

pr_r464_2632_status pr_r464_2632_third_call(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    static const pr_r464_2632_cpu measured_pre = {
        0x2D14u, 0x01E4u, 0x0007u, 0x3E4Eu, 0u, 0x14A0u, 0x3E68u,
        0x260Au, 0x3E98u, 0x3E86u, 0x14A0u, 0x3DE4u, 0x1D16u, 0x7246u
    };
    pr_r464_2632_cpu saved;
    pr_r464_2632_status result;
    uint16_t old_cs_slot, old_ip_slot;

    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (memcmp(cpu, &measured_pre, sizeof(*cpu)) != 0)
        return PR_R464_2632_UNSUPPORTED_PATH;

    saved = *cpu;
    old_cs_slot = word_at(ram, cpu->ss, 0x3E84u);
    old_ip_slot = word_at(ram, cpu->ss, 0x3E82u);
    cpu->sp = 0x3E84u;
    put_word(ram, cpu->ss, cpu->sp, 0x2D14u);
    cpu->sp = 0x3E82u;
    put_word(ram, cpu->ss, cpu->sp, 0x01E9u);
    cpu->cs = 0x0C88u;
    cpu->ip = 0x2632u;
    result = pr_r464_2632_third_body(ram, ram_size, cpu,
                                       observer, context);
    if (result != PR_R464_2632_OK) {
        put_word(ram, saved.ss, 0x3E84u, old_cs_slot);
        put_word(ram, saved.ss, 0x3E82u, old_ip_slot);
        *cpu = saved;
    }
    return result;
}
