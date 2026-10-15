#include "poolrad/recovered/overlay_u6c_record_roll_sum_4235.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6C_4235_RAM_SIZE - 1u);
}
static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U6C_4235_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}
static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & (PR_U6C_4235_RAM_SIZE - 1u)] << 8u));
}
static pr_u6c_4235_cpu prepare(uint8_t *ram, unsigned mask, unsigned gate,
                              unsigned count, unsigned range, uint32_t seed)
{
    unsigned i;
    pr_u6c_4235_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x4235u, 0x1000u, 0x1111u, 0x2000u, 0x0ED7u
    };
    memset(ram, 0xA5, PR_U6C_4235_RAM_SIZE);
    for (i = 0; i < 8; ++i) {
        ram[physical(0x4000u, (uint16_t)(0x1096u + i))] = (uint8_t)gate;
        ram[physical(cpu.ds, (uint16_t)(0x05DEu + i))] = (uint8_t)(1u << i);
        ram[physical(cpu.ds, (uint16_t)(0x08B6u + i))] = (uint8_t)count;
        ram[physical(cpu.ds, (uint16_t)(0x08BEu + i))] = (uint8_t)range;
    }
    put16(ram, cpu.ds, 0x4428u, (uint16_t)seed);
    put16(ram, cpu.ds, 0x442Au, (uint16_t)(seed >> 16u));
    put16(ram, 0xD000u, 0x1245u, 0x8405u);
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ss, 0x8004u, (uint16_t)mask);
    put16(ram, cpu.ss, 0x8006u, 0x1000u);
    put16(ram, cpu.ss, 0x8008u, 0x4000u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U6C_4235_RAM_SIZE);
    const pr_u6c_4235_layout layout = {0xC000u, 0xD000u};
    pr_u6c_4235_cpu cpu;
    unsigned mask, count, range, gate, i, j;
    assert(ram);
    cpu = prepare(ram, 0, 1, 1, 6, 1);
    assert(pr_u6c_record_roll_sum_4235(NULL, PR_U6C_4235_RAM_SIZE, &cpu, &layout) == PR_U6C_4235_ARGUMENT);
    assert(pr_u6c_record_roll_sum_4235(ram, 1, &cpu, &layout) == PR_U6C_4235_ARGUMENT);
    assert(pr_u6c_record_roll_sum_4235(ram, PR_U6C_4235_RAM_SIZE, NULL, &layout) == PR_U6C_4235_ARGUMENT);
    assert(pr_u6c_record_roll_sum_4235(ram, PR_U6C_4235_RAM_SIZE, &cpu, NULL) == PR_U6C_4235_ARGUMENT);
    assert(cpu.sp == 0x8000u && ram[physical(cpu.ss, 0x7FFEu)] == 0xA5u);
    for (mask = 0; mask < 256; mask += 17) {
        for (gate = 1; gate <= 2; ++gate) {
            for (count = 0; count <= 3; ++count) {
                for (range = 0; range <= 255; range += 51) {
                    uint32_t seed = 0xBB36E7A1u;
                    uint8_t total = 0;
                    cpu = prepare(ram, mask | 0xAB00u, gate, count, range, seed);
                    for (i = 0; i < 8; ++i) if ((mask & (1u << i)) != 0u) {
                        uint8_t roll = 0;
                        for (j = 0; j < count; ++j) {
                            seed = seed * 0x08088405u + 1u;
                            roll = (uint8_t)(roll + (range ? (seed >> 16u) % range : 0u) + 1u);
                        }
                        if (gate == 1u && roll < 2u * range / 3u) roll = (uint8_t)(2u * range / 3u);
                        total = (uint8_t)(total + roll);
                    }
                    assert(pr_u6c_record_roll_sum_4235(ram, PR_U6C_4235_RAM_SIZE, &cpu, &layout) == PR_U6C_4235_OK);
                    assert(cpu.ax == total && cpu.bp == 0x7777u && cpu.sp == 0x800Au);
                    assert(cpu.cs == 0xF000u && cpu.ip == 0x0712u && cpu.flags == 0x0646u);
                    assert(cpu.si == 0x9ABCu && cpu.ds == 0x1000u && cpu.ss == 0x2000u);
                    assert(get16(ram, cpu.ds, 0x4428u) == (uint16_t)seed);
                    assert(get16(ram, cpu.ds, 0x442Au) == (uint16_t)(seed >> 16u));
                    assert(ram[physical(cpu.ss, 0x7FFDu)] == total);
                }
            }
        }
    }
    cpu = prepare(ram, 255, 128, 255, 255, 0xFFFFFFFFu);
    assert(pr_u6c_record_roll_sum_4235(ram, PR_U6C_4235_RAM_SIZE, &cpu, &layout) == PR_U6C_4235_OK);
    assert(cpu.ax == 0 && cpu.bx == 0x3456u && cpu.cx == 0x5678u && cpu.dx == 0x789Au);
    assert(get16(ram, cpu.ds, 0x4428u) == 0xFFFFu);
    /* Live seed overwrites the nested near-return IP. Stop after that RET,
     * retaining the actual seeded word, stack pop and reached SYSTEM CS. */
    cpu = prepare(ram, 255, 1, 1, 6, 1);
    cpu.ss = cpu.ds;
    cpu.sp = 0x4444u;
    put16(ram, cpu.ss, 0x4444u, 0x0712u);
    put16(ram, cpu.ss, 0x4446u, 0xF000u);
    put16(ram, cpu.ss, 0x4448u, 255u);
    put16(ram, cpu.ss, 0x444Au, 0x1000u);
    put16(ram, cpu.ss, 0x444Cu, 0x4000u);
    assert(pr_u6c_record_roll_sum_4235(ram, PR_U6C_4235_RAM_SIZE, &cpu, &layout) == PR_U6C_4235_TRANSFER);
    assert(cpu.cs == layout.system_cs && cpu.sp == 0x442Au && cpu.bp == 0x4434u);
    assert(cpu.ip == get16(ram, cpu.ds, 0x4428u) && cpu.ip != 0x11C3u);
    free(ram);
    return 0;
}
