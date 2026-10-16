#include "poolrad/recovered/overlay_u3b0_089c_es_al_read.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const struct {
        uint16_t cs, es, di, sp, ax, flags;
        uint32_t physical;
        uint8_t value;
    } cases[] = {
        {0xF000u, 0x0000u, 0x0000u, 0x0000u, 0xA510u, 0x0000u, 0x00000u, 0x11u},
        {0xF000u, 0xFFFFu, 0x000Fu, 0xFFFFu, 0x7F22u, 0x0041u, 0xFFFFFu, 0xA2u},
        {0xF000u, 0xFFFFu, 0x0010u, 0x0001u, 0x8033u, 0xFFFFu, 0x00000u, 0xC3u},
        {0xF000u, 0x1234u, 0xFFFFu, 0xFFFEu, 0x0044u, 0x0207u, 0x2233Fu, 0xD4u},
        {0xF000u, 0xF000u, 0xFFFFu, 0x1357u, 0xFF55u, 0x0841u, 0xFFFFFu, 0xE5u},
        {0xFFFFu, 0xFFFFu, 0x0010u, 0x2468u, 0xABCDu, 0x0000u, 0x00000u, 0xF6u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;
    uint32_t code_physical;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x089Cu;
    memset(ram, 0xA5, PR_U3B0_RAM_SIZE);
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    expected = cpu;
    assert(pr_u3b0_089c_es_al_read(NULL, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_089C_ES_AL_READ_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(pr_u3b0_089c_es_al_read(ram, PR_U3B0_RAM_SIZE - 1u,
                                   &cpu) == PR_U3B0_089C_ES_AL_READ_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(pr_u3b0_089c_es_al_read(ram, PR_U3B0_RAM_SIZE,
                                   NULL) == PR_U3B0_089C_ES_AL_READ_ARGUMENT);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        code_physical = ((uint32_t)cases[i].cs << 4) + 0x089Cu;
        ram[code_physical & 0xFFFFFu] = 0x26u;
        ram[(code_physical + 1u) & 0xFFFFFu] = 0x8Au;
        ram[(code_physical + 2u) & 0xFFFFFu] = 0x05u;
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
        cpu.cs = cases[i].cs;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x089Cu;
        expected = cpu;
        expected.ax = (uint16_t)((expected.ax & 0xFF00u) | cases[i].value);
        expected.ip = 0x089Fu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_089c_es_al_read(ram, PR_U3B0_RAM_SIZE,
                                       &cpu) == PR_U3B0_089C_ES_AL_READ_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x089Cu;
    code_physical = ((uint32_t)cpu.cs << 4) + 0x089Cu;
    for (j = 0; j < 3u; ++j) {
        ram[(code_physical + j) & 0xFFFFFu] ^= 1u;
        expected = cpu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_089c_es_al_read(ram, PR_U3B0_RAM_SIZE,
                                       &cpu) == PR_U3B0_089C_ES_AL_READ_ARGUMENT);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
        ram[(code_physical + j) & 0xFFFFFu] ^= 1u;
    }
    cpu.ip = 0x089Fu;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_089c_es_al_read(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_089C_ES_AL_READ_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    free(snapshot);
    free(ram);
    return 0;
}
