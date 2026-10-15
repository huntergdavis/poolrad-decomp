#include "poolrad/recovered/overlay_u3b0_0887_ss_di_read.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_0887_ss_di_read(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_0887_SS_DI_READ_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t ss, bp, sp, ax, flags, value;
        uint32_t low_physical, high_physical;
    } cases[] = {
        {0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0044u, 0x1234u,
         0x00006u, 0x00007u},
        {0xFFFFu, 0x0009u, 0xFFFFu, 0x00FFu, 0xF77Eu, 0xABCDu,
         0xFFFFFu, 0x00000u},
        {0x1234u, 0xFFF9u, 0x0001u, 0x0033u, 0x0054u, 0x0E7Bu,
         0x2233Fu, 0x12340u},
        {0xFFFFu, 0x000Au, 0xFFFEu, 0x0044u, 0x0244u, 0x5A69u,
         0x00000u, 0x00001u},
        {0xF000u, 0xFFF8u, 0x1357u, 0x0055u, 0x0044u, 0xF012u,
         0xFFFFEu, 0xFFFFFu},
        {0x2468u, 0x0001u, 0x8000u, 0x0066u, 0x0254u, 0xBEEFu,
         0x24687u, 0x24688u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x0887u;
    assert(pr_u3b0_0887_ss_di_read(NULL, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_0887_SS_DI_READ_ARGUMENT);
    assert(pr_u3b0_0887_ss_di_read(ram, PR_U3B0_RAM_SIZE - 1u,
                                   &cpu) == PR_U3B0_0887_SS_DI_READ_ARGUMENT);
    assert(pr_u3b0_0887_ss_di_read(ram, PR_U3B0_RAM_SIZE,
                                   NULL) == PR_U3B0_0887_SS_DI_READ_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        uint16_t offset = (uint16_t)(cases[i].bp + 6u);
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        ram[0xF0887u] = 0x8Bu;
        ram[0xF0888u] = 0x7Eu;
        ram[0xF0889u] = 0x06u;
        ram[cases[i].low_physical] = (uint8_t)cases[i].value;
        ram[cases[i].high_physical] = (uint8_t)(cases[i].value >> 8);
        ram[(0x13570u + offset) & 0xFFFFFu] = 0x5Au;
        ram[(0x13570u + (uint16_t)(offset + 1u)) & 0xFFFFFu] = 0xA5u;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = cases[i].ax;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = cases[i].ax;
        cpu.si = 0x5678u;
        cpu.di = 0x9ABCu;
        cpu.bp = cases[i].bp;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = cases[i].ss;
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x0887u;
        expected = cpu;
        expected.di = cases[i].value;
        expected.ip = 0x088Au;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0887_ss_di_read(ram, PR_U3B0_RAM_SIZE,
                                       &cpu) == PR_U3B0_0887_SS_DI_READ_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x0887u;
    for (j = 0; j < 3u; ++j) {
        ram[0xF0887u + j] ^= 1u;
        reject_unchanged(ram, snapshot, &cpu);
        ram[0xF0887u + j] ^= 1u;
    }
    cpu.dx ^= 1u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.dx ^= 1u;
    cpu.ax |= 0x1200u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.ax &= 0x00FFu;
    cpu.flags ^= 0x0040u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.flags ^= 0x0040u;
    cpu.ip = 0x088Au;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
