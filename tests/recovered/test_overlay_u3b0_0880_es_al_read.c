#include "poolrad/recovered/overlay_u3b0_0880_es_al_read.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const struct {
        uint16_t es, di, sp, ax, flags;
        uint32_t physical;
        uint8_t value;
    } cases[] = {
        {0x0000u, 0x0000u, 0x0000u, 0xA510u, 0x0001u, 0x00000u, 0x11u},
        {0xFFFFu, 0x000Fu, 0xFFFFu, 0x7F22u, 0x0041u, 0xFFFFFu, 0xA2u},
        {0xFFFFu, 0x0010u, 0x0001u, 0x8033u, 0xFFFFu, 0x00000u, 0xC3u},
        {0x1234u, 0xFFFFu, 0xFFFEu, 0x0044u, 0x0207u, 0x2233Fu, 0xD4u},
        {0xF000u, 0xFFFFu, 0x1357u, 0xFF55u, 0x0841u, 0xFFFFFu, 0xE5u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x0880u;
    assert(pr_u3b0_0880_es_al_read(NULL, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_0880_ES_AL_READ_ARGUMENT);
    assert(pr_u3b0_0880_es_al_read(ram, PR_U3B0_RAM_SIZE - 1u,
                                   &cpu) == PR_U3B0_0880_ES_AL_READ_ARGUMENT);
    assert(pr_u3b0_0880_es_al_read(ram, PR_U3B0_RAM_SIZE,
                                   NULL) == PR_U3B0_0880_ES_AL_READ_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        /* Synthetic F000:0000 loaded copy of exact original +0880 bytes. */
        ram[0xF0880u] = 0x26u;
        ram[0xF0881u] = 0x8Au;
        ram[0xF0882u] = 0x05u;
        ram[cases[i].physical] = cases[i].value;
        ram[(0x13570u + cases[i].di) & 0xFFFFFu] =
            (uint8_t)(cases[i].value ^ 0xFFu);
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = cases[i].ax;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = 0x89ABu;
        cpu.si = 0x5678u;
        cpu.di = cases[i].di;
        cpu.bp = 0xBEEFu;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = cases[i].es;
        cpu.ss = 0x2468u;
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x0880u;
        expected = cpu;
        expected.ax = (uint16_t)((expected.ax & 0xFF00u) | cases[i].value);
        expected.ip = 0x0883u;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0880_es_al_read(ram, PR_U3B0_RAM_SIZE,
                                       &cpu) == PR_U3B0_0880_ES_AL_READ_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x0880u;
    for (j = 0; j < 3u; ++j) {
        ram[0xF0880u + j] ^= 1u;
        expected = cpu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0880_es_al_read(ram, PR_U3B0_RAM_SIZE,
                                       &cpu) == PR_U3B0_0880_ES_AL_READ_ARGUMENT);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
        ram[0xF0880u + j] ^= 1u;
    }
    cpu.flags &= (uint16_t)~0x0001u;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_0880_es_al_read(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_0880_ES_AL_READ_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    cpu.ip = 0x0883u;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_0880_es_al_read(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_0880_ES_AL_READ_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    free(snapshot);
    free(ram);
    return 0;
}
