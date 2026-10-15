#include "poolrad/recovered/overlay_u9e_sidecar_call_018b.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U9E_018B_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t p = at(segment, offset);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U9E_018B_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t p = at(segment, offset);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U9E_018B_RAM_SIZE - 1u)] << 8));
}

static void run(int mode)
{
    uint8_t *ram = calloc(PR_U9E_018B_RAM_SIZE, 1u);
    pr_u9e_018b_cpu cpu = {0};
    uint16_t cell;
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x7FFEu;
    cpu.sp = 0x7F10u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x018Bu;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x72D3u;
    cell = (uint16_t)(cpu.bp - 0xE0u);
    put16(ram, cpu.ss, cpu.sp, cell);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), cpu.ss);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), 0xA57Du);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 6u), 0x2300u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 8u), 0x4000u);
    put16(ram, 0x4000u, 0x237Fu, mode ? 0x2200u : 0u);
    put16(ram, 0x4000u, 0x2381u, mode ? 0x5000u : 0u);
    ram[at(0x5000u, 0x2200u)] = 0x7Du;
    if (mode) {
        assert(pr_u9e_sidecar_call_018b(ram, PR_U9E_018B_RAM_SIZE,
                                         &cpu, 4u) == PR_U9E_018B_FOUND);
        assert(cpu.ip == 0x02D4u && (cpu.ax & 255u) == 1u);
        assert(ram[at(cpu.ss, (uint16_t)(cpu.bp - 0xE1u))] == 1u);
    } else {
        assert(pr_u9e_sidecar_call_018b(ram, PR_U9E_018B_RAM_SIZE,
                                         &cpu, 4u) == PR_U9E_018B_ABSENT);
        assert(cpu.ip == 0x019Cu && (cpu.ax & 255u) == 0u);
    }
    assert(cpu.cs == 0xF000u && cpu.bp == 0x7FFEu && cpu.sp == 0x7F1Au);
    assert(cpu.es == cpu.ss && cpu.di == cell);
    assert(get16(ram, cpu.ss, cell) == (mode ? 0x2200u : 0u));
    assert(get16(ram, cpu.ss, (uint16_t)(cell + 2u)) ==
           (mode ? 0x5000u : 0u));
    assert(get16(ram, cpu.ss, 0x7F0Cu) == 0x0190u);
    assert(get16(ram, cpu.ss, 0x7F0Eu) == 0xF000u);
    assert(pr_u9e_sidecar_call_018b(NULL, PR_U9E_018B_RAM_SIZE,
                                     &cpu, 4u) == PR_U9E_018B_ARGUMENT);
    free(ram);
}

int main(void)
{
    run(0);
    run(1);
    return 0;
}
