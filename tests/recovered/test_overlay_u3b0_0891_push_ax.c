#include "poolrad/recovered/overlay_u3b0_0891_push_ax.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_0891_push_ax(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_0891_PUSH_AX_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t cs, ss, sp, ax, flags, expected_sp;
        uint32_t low_physical, high_physical;
    } cases[] = {
        {0xF000u, 0x0000u, 0x0002u, 0x1234u, 0x0000u, 0x0000u,
         0x00000u, 0x00001u},
        {0xFFFFu, 0xFFFFu, 0x0012u, 0xABCDu, 0x0894u, 0x0010u,
         0x00000u, 0x00001u},
        {0xF000u, 0x1234u, 0x0001u, 0xFEDCu, 0x0010u, 0xFFFFu,
         0x2233Fu, 0x12340u},
        {0xF000u, 0x0000u, 0x0001u, 0x00FFu, 0xFFFEu, 0xFFFFu,
         0x0FFFFu, 0x00000u},
        {0xF000u, 0xFFFFu, 0x0001u, 0x8001u, 0x0044u, 0xFFFFu,
         0x0FFEFu, 0xFFFF0u},
        {0xF000u, 0xF000u, 0x0893u, 0x3412u, 0x0801u, 0x0891u,
         0xF0891u, 0xF0892u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    uint32_t code_physical;
    size_t i;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x0891u;
    assert(pr_u3b0_0891_push_ax(NULL, PR_U3B0_RAM_SIZE, &cpu) ==
           PR_U3B0_0891_PUSH_AX_ARGUMENT);
    assert(pr_u3b0_0891_push_ax(ram, PR_U3B0_RAM_SIZE - 1u, &cpu) ==
           PR_U3B0_0891_PUSH_AX_ARGUMENT);
    assert(pr_u3b0_0891_push_ax(ram, PR_U3B0_RAM_SIZE, NULL) ==
           PR_U3B0_0891_PUSH_AX_ARGUMENT);

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x30u + i), PR_U3B0_RAM_SIZE);
        code_physical = (((uint32_t)cases[i].cs << 4) + 0x0891u) & 0xFFFFFu;
        ram[code_physical] = 0x50u;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = cases[i].ax;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = 0x89ABu;
        cpu.si = 0x5678u;
        cpu.di = 0x9ABCu;
        cpu.bp = 0xFFF9u;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = cases[i].ss;
        cpu.cs = cases[i].cs;
        cpu.ip = 0x0891u;
        cpu.flags = cases[i].flags;
        expected = cpu;
        expected.sp = cases[i].expected_sp;
        expected.ip = 0x0892u;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        snapshot[cases[i].low_physical] = (uint8_t)cpu.ax;
        snapshot[cases[i].high_physical] = (uint8_t)(cpu.ax >> 8);
        assert(pr_u3b0_0891_push_ax(ram, PR_U3B0_RAM_SIZE, &cpu) ==
               PR_U3B0_0891_PUSH_AX_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }

    cpu.ip = 0x0891u;
    code_physical = (((uint32_t)cpu.cs << 4) + 0x0891u) & 0xFFFFFu;
    ram[code_physical] = 0x50u;
    ram[code_physical] ^= 1u;
    reject_unchanged(ram, snapshot, &cpu);
    ram[code_physical] ^= 1u;
    cpu.ip = 0x0892u;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
