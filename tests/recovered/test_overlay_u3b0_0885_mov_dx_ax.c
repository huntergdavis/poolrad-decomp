#include "poolrad/recovered/overlay_u3b0_0885_mov_dx_ax.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_0885_mov_dx_ax(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_0885_MOV_DX_AX_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t ax, dx, flags, ss, sp;
    } cases[] = {
        {0x0000u, 0xFFFFu, 0x0044u, 0x0000u, 0x0000u},
        {0x00FFu, 0x0000u, 0xF77Eu, 0xFFFFu, 0xFFFFu},
        {0x0034u, 0xABCDu, 0x0054u, 0x1234u, 0x0001u},
        {0x0001u, 0x1111u, 0x0244u, 0xFFFFu, 0xFFFEu},
        {0x00CDu, 0xDEADu, 0x0044u, 0xF000u, 0x1357u},
        {0x0001u, 0xEFEFu, 0x0254u, 0x2468u, 0x8000u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x0885u;
    assert(pr_u3b0_0885_mov_dx_ax(NULL, PR_U3B0_RAM_SIZE,
                                  &cpu) == PR_U3B0_0885_MOV_DX_AX_ARGUMENT);
    assert(pr_u3b0_0885_mov_dx_ax(ram, PR_U3B0_RAM_SIZE - 1u,
                                  &cpu) == PR_U3B0_0885_MOV_DX_AX_ARGUMENT);
    assert(pr_u3b0_0885_mov_dx_ax(ram, PR_U3B0_RAM_SIZE,
                                  NULL) == PR_U3B0_0885_MOV_DX_AX_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        ram[0xF0885u] = 0x8Bu;
        ram[0xF0886u] = 0xD0u;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = cases[i].ax;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = cases[i].dx;
        cpu.si = 0x5678u;
        cpu.di = 0x9ABCu;
        cpu.bp = 0xBEEFu;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = cases[i].ss;
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x0885u;
        expected = cpu;
        expected.dx = cases[i].ax;
        expected.ip = 0x0887u;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0885_mov_dx_ax(ram, PR_U3B0_RAM_SIZE,
                                      &cpu) == PR_U3B0_0885_MOV_DX_AX_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x0885u;
    for (j = 0; j < 2u; ++j) {
        ram[0xF0885u + j] ^= 1u;
        reject_unchanged(ram, snapshot, &cpu);
        ram[0xF0885u + j] ^= 1u;
    }
    cpu.flags ^= 0x0001u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.flags ^= 0x0001u;
    cpu.flags ^= 0x0040u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.flags ^= 0x0040u;
    cpu.ax |= 0x1200u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.ax &= 0x00FFu;
    cpu.ip = 0x0887u;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
