#include "poolrad/recovered/overlay_u9e_entry8_fullstate_tail_0e1f.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MASK (PR_U9E_ENTRY8_TAIL_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void put8(uint8_t *ram, uint16_t segment, uint16_t offset,
                 uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static pr_u9e_entry8_tail_cpu frame(uint8_t *ram, uint16_t bp)
{
    pr_u9e_entry8_tail_cpu cpu = {
        0xABCDu, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
        bp, 0x7FF8u, 0x3000u, 0x0E1Fu, 0x1000u, 0x4000u,
        0x2000u, 0x7212u
    };
    memset(ram, 0xA5, PR_U9E_ENTRY8_TAIL_RAM_SIZE);
    put8(ram, cpu.ss, (uint16_t)(bp - 3u), 0x12u);
    put16(ram, cpu.ss, bp, 0x7777u);
    put16(ram, cpu.ss, (uint16_t)(bp + 2u), 0x0FE3u);
    put16(ram, cpu.ss, (uint16_t)(bp + 4u), 0xBEEFu);
    put16(ram, cpu.ss, (uint16_t)(bp + 6u), 0x9913u);
    put16(ram, cpu.ss, (uint16_t)(bp + 8u), 1u);
    return cpu;
}

static void check_return(uint8_t *ram, uint8_t *expected, uint16_t bp,
                         int decoy)
{
    pr_u9e_entry8_tail_cpu cpu = frame(ram, bp);
    pr_u9e_entry8_tail_cpu before = cpu;
    if (decoy) {
        size_t linear_cs = (physical(cpu.ss, (uint16_t)(bp + 2u)) + 2u) & MASK;
        ram[linear_cs] = 0xADu;
        ram[(linear_cs + 1u) & MASK] = 0xDEu;
    }
    memcpy(expected, ram, PR_U9E_ENTRY8_TAIL_RAM_SIZE);
    put8(expected, before.ss, (uint16_t)(bp - 1u), 0x12u);
    assert(pr_u9e_entry8_fullstate_tail_0e1f(
               ram, PR_U9E_ENTRY8_TAIL_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_TAIL_RETURNED);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_TAIL_RAM_SIZE) == 0);
    assert(cpu.ax == 0xAB12u && cpu.bp == 0x7777u &&
           cpu.sp == (uint16_t)(bp + 10u) && cpu.ip == 0x0FE3u &&
           cpu.cs == 0xBEEFu && cpu.flags == before.flags);
    assert(cpu.bx == before.bx && cpu.cx == before.cx &&
           cpu.dx == before.dx && cpu.si == before.si &&
           cpu.di == before.di && cpu.ds == before.ds &&
           cpu.es == before.es && cpu.ss == before.ss);
}

int main(void)
{
    uint8_t *ram = malloc(PR_U9E_ENTRY8_TAIL_RAM_SIZE);
    uint8_t *expected = malloc(PR_U9E_ENTRY8_TAIL_RAM_SIZE);
    pr_u9e_entry8_tail_cpu cpu, before;
    assert(ram != NULL && expected != NULL);

    cpu = frame(ram, 0x8000u);
    before = cpu;
    memcpy(expected, ram, PR_U9E_ENTRY8_TAIL_RAM_SIZE);
    assert(pr_u9e_entry8_fullstate_tail_0e1f(
               NULL, PR_U9E_ENTRY8_TAIL_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_TAIL_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_tail_0e1f(ram, 0u, &cpu) ==
           PR_U9E_ENTRY8_TAIL_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_tail_0e1f(
               ram, PR_U9E_ENTRY8_TAIL_RAM_SIZE, NULL) ==
           PR_U9E_ENTRY8_TAIL_ARGUMENT);
    cpu.ip = 0x0E20u;
    assert(pr_u9e_entry8_fullstate_tail_0e1f(
               ram, PR_U9E_ENTRY8_TAIL_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_TAIL_ARGUMENT);
    cpu.ip = 0x0E1Fu;
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_TAIL_RAM_SIZE) == 0);

    check_return(ram, expected, 0x8000u, 0);
    /* IP word at FFFE/FFFF; CS begins at offset 0000/0001. */
    check_return(ram, expected, 0xFFFCu, 1);
    check_return(ram, expected, 0xFFFDu, 1);

    cpu = frame(ram, 0x0DE4u);
    cpu.ss = cpu.cs;
    before = cpu;
    memcpy(expected, ram, PR_U9E_ENTRY8_TAIL_RAM_SIZE);
    assert(pr_u9e_entry8_fullstate_tail_0e1f(
               ram, PR_U9E_ENTRY8_TAIL_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_TAIL_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_TAIL_RAM_SIZE) == 0);

    free(expected);
    free(ram);
    return 0;
}
