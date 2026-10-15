#include "poolrad/recovered/overlay_u3b0_088f_add_ax_dx.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_088f_add_ax_dx(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_088F_ADD_AX_DX_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t ax, dx, flags, result, defined_flags;
    } cases[] = {
        {0x0000u, 0x0000u, 0x0044u, 0x0000u, 0x0044u},
        {0xFFFFu, 0x0001u, 0x0254u, 0x0000u, 0x0055u},
        {0xFF80u, 0x8000u, 0xF244u, 0x7F80u, 0x0801u},
        {0x000Fu, 0x0001u, 0x0044u, 0x0010u, 0x0010u},
        {0x007Fu, 0x7FFFu, 0x0254u, 0x807Eu, 0x0894u},
        {0xFFFEu, 0x0002u, 0xF244u, 0x0000u, 0x0055u},
        {0xFF80u, 0xFF80u, 0x0044u, 0xFF00u, 0x0085u},
        {0x007Fu, 0x0001u, 0x0254u, 0x0080u, 0x0010u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x088Fu;
    assert(pr_u3b0_088f_add_ax_dx(NULL, PR_U3B0_RAM_SIZE, &cpu) ==
           PR_U3B0_088F_ADD_AX_DX_ARGUMENT);
    assert(pr_u3b0_088f_add_ax_dx(ram, PR_U3B0_RAM_SIZE - 1u, &cpu) ==
           PR_U3B0_088F_ADD_AX_DX_ARGUMENT);
    assert(pr_u3b0_088f_add_ax_dx(ram, PR_U3B0_RAM_SIZE, NULL) ==
           PR_U3B0_088F_ADD_AX_DX_ARGUMENT);

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x30u + i), PR_U3B0_RAM_SIZE);
        ram[0xF088Fu] = 0x03u;
        ram[0xF0890u] = 0xC2u;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = cases[i].ax;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = cases[i].dx;
        cpu.si = 0x5678u;
        cpu.di = 0x9ABCu;
        cpu.bp = 0xFFF9u;
        cpu.sp = 0x0001u;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = 0xFFFFu;
        cpu.cs = 0xF000u;
        cpu.ip = 0x088Fu;
        cpu.flags = cases[i].flags;
        expected = cpu;
        expected.ax = cases[i].result;
        expected.flags = (uint16_t)((cases[i].flags & (uint16_t)~0x08D5u) |
                                    cases[i].defined_flags);
        expected.ip = 0x0891u;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_088f_add_ax_dx(ram, PR_U3B0_RAM_SIZE, &cpu) ==
               PR_U3B0_088F_ADD_AX_DX_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }

    cpu.ip = 0x088Fu;
    for (j = 0; j < 2u; ++j) {
        ram[0xF088Fu + j] ^= 1u;
        reject_unchanged(ram, snapshot, &cpu);
        ram[0xF088Fu + j] ^= 1u;
    }
    cpu.ax = 0x0180u; /* CBW cannot produce this word. */
    reject_unchanged(ram, snapshot, &cpu);
    cpu.ax = 0x007Fu;
    cpu.flags ^= 0x0040u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.flags ^= 0x0040u;
    cpu.ip = 0x0891u;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
