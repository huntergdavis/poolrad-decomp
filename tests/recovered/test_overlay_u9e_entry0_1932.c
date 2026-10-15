#include "poolrad/recovered/overlay_u9e_entry0_1932.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U9E_1932_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U9E_1932_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(uint16_t sp, uint16_t bp)
{
    uint8_t *ram = calloc(PR_U9E_1932_RAM_SIZE, 1u);
    pr_u9e_1932_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = bp;
    cpu.sp = sp;
    cpu.cs = 0xF000u;
    cpu.ip = 0x1932u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0AD7u;
    put16(ram, cpu.ss, sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(sp + 2u), 0xD000u);
    assert(pr_u9e_entry0_1932(ram, PR_U9E_1932_RAM_SIZE, &cpu) ==
           PR_U9E_1932_OK);
    assert(cpu.ax == 0xA5CDu && cpu.bp == bp && cpu.flags == 0x0AD7u);
    assert(cpu.sp == (uint16_t)(sp + 4u));
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u);
    assert(ram[at(cpu.ss, (uint16_t)(sp - 2u))] == (uint8_t)bp);
    assert(ram[at(cpu.ss, (uint16_t)(sp - 1u))] == (uint8_t)(bp >> 8));
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U9E_1932_RAM_SIZE, 1u);
    pr_u9e_1932_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x1932u;
    assert(pr_u9e_entry0_1932(NULL, PR_U9E_1932_RAM_SIZE, &cpu) ==
           PR_U9E_1932_ARGUMENT);
    assert(pr_u9e_entry0_1932(ram, PR_U9E_1932_RAM_SIZE - 1u, &cpu) ==
           PR_U9E_1932_ARGUMENT);
    free(ram);
    run(0x8000u, 0x2468u);
    run(0x0000u, 0xFFFFu);
    run(0xFFFEu, 0x68ACu);
    run(0xFFFFu, 0x1357u);
    return 0;
}
