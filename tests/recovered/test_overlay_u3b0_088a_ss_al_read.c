#include "poolrad/recovered/overlay_u3b0_088a_ss_al_read.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_088a_ss_al_read(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_088A_SS_AL_READ_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t ss, di, sp, ax, flags;
        uint32_t physical;
        uint8_t value;
    } cases[] = {
        {0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0044u, 0x0001Eu, 0x80u},
        {0xFFFFu, 0xFFF1u, 0xFFFFu, 0x00FFu, 0xF77Eu, 0xFFFFFu, 0x7Fu},
        {0x1234u, 0xFFE1u, 0x0001u, 0x0033u, 0x0054u, 0x2233Fu, 0x00u},
        {0xFFFFu, 0xFFF2u, 0xFFFEu, 0x0044u, 0x0244u, 0x00000u, 0xFFu},
        {0xF000u, 0xFFE0u, 0x1357u, 0x0055u, 0x0044u, 0xFFFFEu, 0xA5u},
        {0x2468u, 0x0001u, 0x8000u, 0x0066u, 0x0254u, 0x2469Fu, 0x5Au}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x088Au;
    assert(pr_u3b0_088a_ss_al_read(NULL, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_088A_SS_AL_READ_ARGUMENT);
    assert(pr_u3b0_088a_ss_al_read(ram, PR_U3B0_RAM_SIZE - 1u,
                                   &cpu) == PR_U3B0_088A_SS_AL_READ_ARGUMENT);
    assert(pr_u3b0_088a_ss_al_read(ram, PR_U3B0_RAM_SIZE,
                                   NULL) == PR_U3B0_088A_SS_AL_READ_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        uint16_t offset = (uint16_t)(cases[i].di + 0x1Eu);
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        ram[0xF088Au] = 0x36u;
        ram[0xF088Bu] = 0x8Au;
        ram[0xF088Cu] = 0x45u;
        ram[0xF088Du] = 0x1Eu;
        ram[cases[i].physical] = cases[i].value;
        ram[(0x13570u + offset) & 0xFFFFFu] = 0x3Cu;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = cases[i].ax;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = cases[i].ax;
        cpu.si = 0x5678u;
        cpu.di = cases[i].di;
        cpu.bp = 0xBEEFu;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = cases[i].ss;
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x088Au;
        expected = cpu;
        expected.ax = cases[i].value;
        expected.ip = 0x088Eu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_088a_ss_al_read(ram, PR_U3B0_RAM_SIZE,
                                       &cpu) == PR_U3B0_088A_SS_AL_READ_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x088Au;
    for (j = 0; j < 4u; ++j) {
        ram[0xF088Au + j] ^= 1u;
        reject_unchanged(ram, snapshot, &cpu);
        ram[0xF088Au + j] ^= 1u;
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
    cpu.ip = 0x088Eu;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
