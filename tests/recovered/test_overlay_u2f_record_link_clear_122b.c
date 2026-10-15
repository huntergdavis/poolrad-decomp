#include "poolrad/recovered/overlay_u2f_record_link_clear_122b.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U2F_122B_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U2F_122B_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}

static uint16_t word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & (PR_U2F_122B_RAM_SIZE - 1u)] << 8));
}

static pr_u2f_122b_cpu prepare(uint8_t *ram)
{
    pr_u2f_122b_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x122Bu, 0x1000u, 0x1111u,
        0x2000u, 0x0ED7u
    };
    memset(ram, 0xA5, PR_U2F_122B_RAM_SIZE);
    put16(ram, 0x4000u, 0x1108u, 0x2000u);
    put16(ram, 0x4000u, 0x110Au, 0x5000u);
    put16(ram, 0x5000u, 0x200Au, 0x3000u);
    put16(ram, 0x5000u, 0x200Cu, 0x6000u);
    ram[physical(0x4000u, 0x110Eu)] = 7;
    ram[physical(0x6000u, 0x310Eu)] = 7;
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ss, 0x8004u, 0x1000u);
    put16(ram, cpu.ss, 0x8006u, 0x4000u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U2F_122B_RAM_SIZE);
    pr_u2f_122b_cpu cpu;
    assert(ram);
    cpu = prepare(ram);
    assert(pr_u2f_record_link_clear_122b(NULL, PR_U2F_122B_RAM_SIZE, &cpu) == PR_U2F_122B_ARGUMENT);
    assert(pr_u2f_record_link_clear_122b(ram, PR_U2F_122B_RAM_SIZE, NULL) == PR_U2F_122B_ARGUMENT);
    assert(pr_u2f_record_link_clear_122b(ram, 1, &cpu) == PR_U2F_122B_ARGUMENT);
    assert(cpu.sp == 0x8000u && ram[physical(cpu.ss, 0x7FFEu)] == 0xA5u);
    assert(pr_u2f_record_link_clear_122b(ram, PR_U2F_122B_RAM_SIZE, &cpu) == PR_U2F_122B_OK);
    assert(ram[physical(0x4000u, 0x110Fu)] == 1 && ram[physical(0x4000u, 0x1110u)] == 0xA5u);
    assert(word(ram, 0x5000u, 0x200Au) == 0 && word(ram, 0x5000u, 0x200Cu) == 0);
    assert(ram[physical(0x5000u, 0x2009u)] == 0xA5u && ram[physical(0x5000u, 0x200Eu)] == 0xA5u);
    assert(cpu.ax == 0 && cpu.di == 0x2000u && cpu.es == 0x5000u && (cpu.flags & 0xFFEFu) == 0x0646u);
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8008u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.bx == 0x3456u && cpu.cx == 0x5678u && cpu.dx == 0x789Au && cpu.si == 0x9ABCu);
    assert(cpu.ds == 0x1000u && cpu.ss == 0x2000u);

    cpu = prepare(ram);
    put16(ram, 0x5000u, 0x200Au, 0);
    put16(ram, 0x5000u, 0x200Cu, 0);
    assert(pr_u2f_record_link_clear_122b(ram, PR_U2F_122B_RAM_SIZE, &cpu) == PR_U2F_122B_OK);
    assert(cpu.ax == 0 && cpu.di == 0x2000u && cpu.es == 0x5000u && (cpu.flags & 0xFFEFu) == 0x0646u);

    cpu = prepare(ram);
    ram[physical(0x6000u, 0x310Eu)] = 0x80u;
    ram[physical(0x4000u, 0x110Eu)] = 0x7Fu;
    assert(pr_u2f_record_link_clear_122b(ram, PR_U2F_122B_RAM_SIZE, &cpu) == PR_U2F_122B_OK);
    assert(cpu.ax == 0x7080u && cpu.di == 0x1000u && cpu.es == 0x4000u && cpu.flags == 0x0E12u);
    assert(word(ram, 0x5000u, 0x200Au) == 0x3000u && word(ram, 0x5000u, 0x200Cu) == 0x6000u);

    /* The first flag store changes the argument offset to 7E01. */
    cpu = prepare(ram);
    put16(ram, cpu.ss, 0x8004u, 0x7EF5u);
    put16(ram, cpu.ss, 0x8006u, cpu.ss);
    put16(ram, cpu.ss, 0x7F09u, 0x2000u);
    put16(ram, cpu.ss, 0x7F0Bu, 0x5000u);
    ram[physical(cpu.ss, 0x7F0Fu)] = 7;
    assert(pr_u2f_record_link_clear_122b(ram, PR_U2F_122B_RAM_SIZE, &cpu) == PR_U2F_122B_OK);
    assert(word(ram, cpu.ss, 0x8004u) == 0x7E01u && word(ram, 0x5000u, 0x200Au) == 0);

    /* Matching clears both live return words, rather than a host snapshot. */
    cpu = prepare(ram);
    put16(ram, 0x4000u, 0x1108u, 0x7FF6u);
    put16(ram, 0x4000u, 0x110Au, cpu.ss);
    ram[physical(0xF000u, 0x0820u)] = 7;
    assert(pr_u2f_record_link_clear_122b(ram, PR_U2F_122B_RAM_SIZE, &cpu) == PR_U2F_122B_OK);
    assert(cpu.cs == 0 && cpu.ip == 0 && cpu.sp == 0x8008u && cpu.bp == 0x7777u);
    free(ram);
    return 0;
}
