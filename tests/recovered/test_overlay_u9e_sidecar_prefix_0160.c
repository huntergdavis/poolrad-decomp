#include "poolrad/recovered/overlay_u9e_sidecar_prefix_0160.h"

#include <assert.h>
#include <stdlib.h>

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U9E_0160_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U9E_0160_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) &
        (PR_U9E_0160_RAM_SIZE - 1u)] << 8));
}

int main(void)
{
    uint8_t *ram = calloc(PR_U9E_0160_RAM_SIZE, 1u);
    pr_u9e_0160_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x1234u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0160u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    put16(ram, cpu.ds, 0x5D96u, 0x4567u);
    put16(ram, cpu.ds, 0x5D98u, 0x789Au);
    put16(ram, cpu.ss, 0x8004u, 0x0031u);
    put16(ram, cpu.ss, 0x8006u, 0x1357u);
    put16(ram, cpu.ss, 0x8008u, 0x2468u);
    assert(pr_u9e_sidecar_prefix_0160(ram, PR_U9E_0160_RAM_SIZE, &cpu) ==
           PR_U9E_0160_PREFIX_DONE);
    assert(cpu.cs == 0xF000u && cpu.ip == 0x018Bu);
    assert(cpu.bp == 0x7FFEu && cpu.sp == 0x7F10u);
    assert(cpu.ax == 0x4531u && cpu.dx == 0x789Au && cpu.di == 0x7F1Eu);
    assert(get16(ram, cpu.ss, cpu.sp) == cpu.di);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u)) == cpu.ss);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u)) == 0x4531u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.sp + 6u)) == 0x1357u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.sp + 8u)) == 0x2468u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.bp - 0x00DCu)) == 0x4567u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.bp - 0x00DAu)) == 0x789Au);
    assert(ram[physical(cpu.ss, (uint16_t)(cpu.bp - 0x00E1u))] == 0u);
    assert(pr_u9e_sidecar_prefix_0160(NULL, PR_U9E_0160_RAM_SIZE, &cpu) ==
           PR_U9E_0160_ARGUMENT);
    assert(pr_u9e_sidecar_prefix_0160(ram, PR_U9E_0160_RAM_SIZE - 1u, &cpu) ==
           PR_U9E_0160_ARGUMENT);
    free(ram);
    return 0;
}
