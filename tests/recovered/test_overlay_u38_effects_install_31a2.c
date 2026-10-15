#include "poolrad/recovered/overlay_u38_effects_install_31a2.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U38_31A2_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U38_31A2_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & (PR_U38_31A2_RAM_SIZE - 1u)] << 8));
}

static pr_u38_31a2_cpu prepare(uint8_t *ram)
{
    pr_u38_31a2_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x31A2u, 0x1000u, 0x1111u,
        0x2000u, 0x0ED7u
    };
    unsigned i;
    memset(ram, 0xA5, PR_U38_31A2_RAM_SIZE);
    for (i = 0; i < 138u; ++i)
        put16(ram, cpu.cs, (uint16_t)(0x31A9u + 13u * i), (uint16_t)(0x4000u + i));
    put16(ram, cpu.ds, 0x6988u, 0x015Bu);
    put16(ram, cpu.ds, 0x698Au, 0x2096u);
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, cpu.cs);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U38_31A2_RAM_SIZE);
    pr_u38_31a2_cpu cpu;
    assert(ram);
    cpu = prepare(ram);
    assert(pr_u38_effects_install_31a2(NULL, PR_U38_31A2_RAM_SIZE, &cpu) == PR_U38_31A2_ARGUMENT);
    assert(pr_u38_effects_install_31a2(ram, PR_U38_31A2_RAM_SIZE, NULL) == PR_U38_31A2_ARGUMENT);
    assert(pr_u38_effects_install_31a2(ram, 1, &cpu) == PR_U38_31A2_ARGUMENT);
    assert(pr_u38_effects_install_31a2(ram, PR_U38_31A2_RAM_SIZE, &cpu) == PR_U38_31A2_OK);
    assert(word(ram, cpu.ds, 0x682Cu) == 0x002Au && word(ram, cpu.ds, 0x682Eu) == 0x4000u);
    assert(word(ram, cpu.ds, 0x6984u) == 0x018Du && word(ram, cpu.ds, 0x698Cu) == 0x0192u);
    assert(word(ram, cpu.ds, 0x6988u) == 0x015Bu && word(ram, cpu.ds, 0x698Au) == 0x2096u);
    assert(word(ram, cpu.ds, 0x6A54u) == 0x024Bu && word(ram, cpu.ds, 0x6A56u) == 0x4089u);
    assert(ram[physical(cpu.ds, 0x682Bu)] == 0xA5u && ram[physical(cpu.ds, 0x6A58u)] == 0xA5u);
    assert(cpu.ax == 0x024Bu && cpu.dx == 0x4089u && cpu.flags == 0x0ED7u);
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8004u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.bx == 0x3456u && cpu.cx == 0x5678u && cpu.si == 0x9ABCu && cpu.di == 0xBCDEu);

    cpu = prepare(ram);
    cpu.ss = cpu.ds;
    cpu.sp = 0x682Eu;
    put16(ram, cpu.ss, 0x682Eu, 0x0712u);
    put16(ram, cpu.ss, 0x6830u, 0xF000u);
    assert(pr_u38_effects_install_31a2(ram, PR_U38_31A2_RAM_SIZE, &cpu) == PR_U38_31A2_OK);
    assert(cpu.bp == 0x002Au && cpu.ip == 0x4000u && cpu.cs == 0x002Fu && cpu.sp == 0x6832u);
    assert(cpu.flags == 0x0ED7u);
    free(ram);
    return 0;
}
