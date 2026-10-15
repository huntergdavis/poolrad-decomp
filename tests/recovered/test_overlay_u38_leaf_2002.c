#include "poolrad/recovered/overlay_u38_leaf_2002.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U38_2002_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U38_2002_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = at(seg, off);
    return (uint16_t)(ram[a] |
                      ((uint16_t)ram[(a + 1u) &
                                      (PR_U38_2002_RAM_SIZE - 1u)] << 8));
}

static void run(uint16_t sp, unsigned mode)
{
    uint8_t *ram = calloc(PR_U38_2002_RAM_SIZE, 1u);
    pr_u38_2002_cpu cpu = {0};
    const uint16_t base = 0x9000u;
    const uint16_t cells = 0x2000u;
    unsigned i;
    assert(ram != NULL);
    cpu.ax = 0xABCDu;
    cpu.bp = 0x2468u;
    cpu.sp = sp;
    cpu.cs = 0xF000u;
    cpu.ip = 0x2002u;
    cpu.ds = 0x6000u;
    cpu.es = 0x8000u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(sp + 2u), 0xD000u);
    put16(ram, cpu.ss, (uint16_t)(sp + 4u), base);
    put16(ram, cpu.ss, (uint16_t)(sp + 6u), 0xE55Eu);
    put16(ram, cpu.ss, (uint16_t)(base + 8u), cells);
    put16(ram, cpu.ss, (uint16_t)(base + 10u), 0x5000u);
    for (i = 0; i < 8u; ++i) {
        uint8_t value = mode == 0u ? (uint8_t)(i + 2u) : 2u;
        uint16_t score = mode == 0u ? (uint16_t)(i + 1u) :
                         mode == 1u ? (uint16_t)(8u - i) :
                         (i == 5u ? 0xFFFFu : (uint16_t)i);
        ram[at(0x5000u, (uint16_t)(cells + 0x96u + i))] = value;
        put16(ram, cpu.ds,
              (uint16_t)(0x409Bu + i * 0x38u + value * 4u), score);
        put16(ram, cpu.ds,
              (uint16_t)(0x409Du + i * 0x38u + value * 4u),
              i == 5u && mode == 2u ? 0xFFFFu : 0u);
    }
    assert(pr_u38_leaf_2002(ram, PR_U38_2002_RAM_SIZE, &cpu) ==
           PR_U38_2002_OK);
    assert((cpu.ax & 0xFFu) == (mode == 1u ? 0u : 7u));
    assert(get16(ram, cpu.ss, (uint16_t)(base - 0x31u)) ==
           (mode == 1u ? 8u : mode == 0u ? 8u : 7u));
    assert(cpu.bp == 0x2468u && cpu.sp == (uint16_t)(sp + 6u));
    assert(cpu.ip == 0x1234u && cpu.cs == 0xD000u);
    assert((cpu.flags & 0x08D5u) == 0x0044u);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U38_2002_RAM_SIZE, 1u);
    pr_u38_2002_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x2002u;
    assert(pr_u38_leaf_2002(NULL, PR_U38_2002_RAM_SIZE, &cpu) ==
           PR_U38_2002_ARGUMENT);
    assert(pr_u38_leaf_2002(ram, PR_U38_2002_RAM_SIZE - 1u, &cpu) ==
           PR_U38_2002_ARGUMENT);
    cpu.ip = 0x2003u;
    assert(pr_u38_leaf_2002(ram, PR_U38_2002_RAM_SIZE, &cpu) ==
           PR_U38_2002_ARGUMENT);
    free(ram);
    run(0x8000u, 0u);
    run(0x8000u, 1u);
    run(0x8000u, 2u);
    run(0xFFFEu, 0u);
    run(0xFFFFu, 1u);
    return 0;
}
