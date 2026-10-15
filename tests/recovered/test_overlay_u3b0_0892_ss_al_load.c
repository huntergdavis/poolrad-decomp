#include "poolrad/recovered/overlay_u3b0_0892_ss_al_load.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_0892_ss_al_load(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_0892_SS_AL_LOAD_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t cs, ss, bp, sp, ax, flags;
        uint32_t physical;
        uint8_t value;
    } cases[] = {
        {0xF000u, 0x0000u, 0x0009u, 0x0000u, 0xABCDu, 0x0000u,
         0x00000u, 0x80u},
        {0xF000u, 0xFFFFu, 0x0018u, 0xFFFFu, 0x12AAu, 0x0010u,
         0xFFFFFu, 0x7Fu},
        {0xF000u, 0xFFFFu, 0x0019u, 0x0001u, 0x34BBu, 0x0894u,
         0x00000u, 0x00u},
        {0xF000u, 0x1234u, 0x0008u, 0xFFFEu, 0x56CCu, 0xFFFEu,
         0x2233Fu, 0xFFu},
        {0xF000u, 0xF000u, 0x0007u, 0x1357u, 0x78DDu, 0x0044u,
         0xFFFFEu, 0xA5u},
        {0xFFFFu, 0x2468u, 0x000Au, 0x8000u, 0x9AEEu, 0x0254u,
         0x24681u, 0x5Au}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    uint32_t code_base, ds_physical;
    uint16_t offset;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x0892u;
    assert(pr_u3b0_0892_ss_al_load(NULL, PR_U3B0_RAM_SIZE, &cpu) ==
           PR_U3B0_0892_SS_AL_LOAD_ARGUMENT);
    assert(pr_u3b0_0892_ss_al_load(ram, PR_U3B0_RAM_SIZE - 1u, &cpu) ==
           PR_U3B0_0892_SS_AL_LOAD_ARGUMENT);
    assert(pr_u3b0_0892_ss_al_load(ram, PR_U3B0_RAM_SIZE, NULL) ==
           PR_U3B0_0892_SS_AL_LOAD_ARGUMENT);

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        code_base = (uint32_t)cases[i].cs << 4;
        ram[(code_base + 0x0892u) & 0xFFFFFu] = 0x8Au;
        ram[(code_base + 0x0893u) & 0xFFFFFu] = 0x46u;
        ram[(code_base + 0x0894u) & 0xFFFFFu] = 0xF7u;
        offset = (uint16_t)(cases[i].bp - 9u);
        ds_physical = (0x13570u + offset) & 0xFFFFFu;
        ram[ds_physical] = 0x3Cu;
        ram[cases[i].physical] = cases[i].value;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = cases[i].ax;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = 0x89ABu;
        cpu.si = 0x5678u;
        cpu.di = 0x9ABCu;
        cpu.bp = cases[i].bp;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = cases[i].ss;
        cpu.cs = cases[i].cs;
        cpu.ip = 0x0892u;
        cpu.flags = cases[i].flags;
        expected = cpu;
        expected.ax = (uint16_t)((cpu.ax & 0xFF00u) | cases[i].value);
        expected.ip = 0x0895u;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0892_ss_al_load(ram, PR_U3B0_RAM_SIZE, &cpu) ==
               PR_U3B0_0892_SS_AL_LOAD_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }

    cpu.ip = 0x0892u;
    code_base = (uint32_t)cpu.cs << 4;
    for (j = 0; j < 3u; ++j) {
        ram[(code_base + 0x0892u + j) & 0xFFFFFu] ^= 1u;
        reject_unchanged(ram, snapshot, &cpu);
        ram[(code_base + 0x0892u + j) & 0xFFFFFu] ^= 1u;
    }
    cpu.ip = 0x0895u;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
