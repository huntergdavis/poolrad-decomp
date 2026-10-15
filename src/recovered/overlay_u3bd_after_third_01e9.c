#include "poolrad/recovered/overlay_u3bd_after_third_01e9.h"

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

static void push(uint8_t *ram, pr_r464_2632_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    ram[physical(cpu->ss, cpu->sp)] = (uint8_t)value;
    ram[physical(cpu->ss, (uint16_t)(cpu->sp + 1u))] = (uint8_t)(value >> 8);
}

pr_r464_2632_status pr_u3bd_after_third_01e9(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu)
{
    static const pr_r464_2632_cpu measured_pre = {
        0x2D14u, 0x01E9u, 1u, 0x3E4Eu, 0u, 0x14A0u, 0x3E68u,
        0x007Au, 0x3E98u, 0x3E98u, 0x14A0u, 0x3DE4u, 0x1D16u, 0x7246u
    };
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (memcmp(cpu, &measured_pre, sizeof(*cpu)) != 0)
        return PR_R464_2632_UNSUPPORTED_PATH;

    cpu->ax = 2u;
    push(ram, cpu, cpu->ax);
    cpu->ax = 0x003Eu;
    push(ram, cpu, cpu->ax);
    cpu->ax = 0x000Bu;
    push(ram, cpu, cpu->ax);
    cpu->ax = 0x002Au;
    push(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         ram[physical(cpu->ds, 0x6A63u)]);
    push(ram, cpu, cpu->ax);
    cpu->di = 0x260Au;
    push(ram, cpu, cpu->ds);
    push(ram, cpu, cpu->di);
    push(ram, cpu, word_at(ram, cpu->ds, 0x4A22u));
    push(ram, cpu, word_at(ram, cpu->ds, 0x4A20u));
    cpu->ip = 0x020Au;
    return PR_R464_2632_OK;
}
