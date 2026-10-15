#include "poolrad/recovered/overlay_u3b0_0874_di_load.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const struct {
        uint16_t ss, bp, sp, di, flags, value;
        uint32_t low_physical, high_physical;
    } cases[] = {
        {0x0000u, 0x0000u, 0x0000u, 0xA510u, 0x0000u, 0x1234u,
         0x00006u, 0x00007u},
        {0xFFFFu, 0x0009u, 0xFFFFu, 0x7F22u, 0x00C1u, 0xABCDu,
         0xFFFFFu, 0x00000u},
        {0x1234u, 0xFFF9u, 0x0001u, 0x8033u, 0xFFFFu, 0x0E7Bu,
         0x2233Fu, 0x12340u},
        {0xFFFFu, 0x000Au, 0xFFFEu, 0x0044u, 0x0246u, 0x5A69u,
         0x00000u, 0x00001u},
        {0xF000u, 0xFFF9u, 0x1357u, 0xFF55u, 0x0840u, 0xF012u,
         0xFFFFFu, 0xF0000u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x0874u;
    assert(pr_u3b0_0874_di_load(NULL, PR_U3B0_RAM_SIZE,
                               &cpu) == PR_U3B0_0874_DI_LOAD_ARGUMENT);
    assert(pr_u3b0_0874_di_load(ram, PR_U3B0_RAM_SIZE - 1u,
                               &cpu) == PR_U3B0_0874_DI_LOAD_ARGUMENT);
    assert(pr_u3b0_0874_di_load(ram, PR_U3B0_RAM_SIZE,
                               NULL) == PR_U3B0_0874_DI_LOAD_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        /* Synthetic F000:0000 loaded copy of original +0874 bytes. */
        ram[0xF0874u] = 0x8Bu;
        ram[0xF0875u] = 0x7Eu;
        ram[0xF0876u] = 0x06u;
        ram[0xFFFFFu] = 0x5Au;
        ram[0x00000u] = 0xC3u;
        ram[cases[i].low_physical] = (uint8_t)cases[i].value;
        ram[cases[i].high_physical] = (uint8_t)(cases[i].value >> 8);
        ram[(0x13570u + (uint16_t)(cases[i].bp + 6u)) & 0xFFFFFu] = 0x5Au;
        ram[(0x13570u + (uint16_t)(cases[i].bp + 7u)) & 0xFFFFFu] = 0xA5u;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = 0xA5C3u;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = 0x89ABu;
        cpu.si = 0x5678u;
        cpu.di = cases[i].di;
        cpu.bp = cases[i].bp;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = cases[i].ss;
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x0874u;
        expected = cpu;
        expected.di = cases[i].value;
        expected.ip = 0x0877u;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0874_di_load(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_0874_DI_LOAD_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x0874u;
    for (j = 0; j < 3u; ++j) {
        ram[0xF0874u + j] ^= 1u;
        expected = cpu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0874_di_load(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_0874_DI_LOAD_ARGUMENT);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
        ram[0xF0874u + j] ^= 1u;
    }
    cpu.ip = 0x0877u;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_0874_di_load(ram, PR_U3B0_RAM_SIZE,
                               &cpu) == PR_U3B0_0874_DI_LOAD_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    free(snapshot);
    free(ram);
    return 0;
}
