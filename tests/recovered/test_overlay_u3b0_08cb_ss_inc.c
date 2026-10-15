#include "poolrad/recovered/overlay_u3b0_08cb_ss_inc.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_08cb_ss_inc(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_08CB_SS_INC_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t ss, bp, sp, flags, expected_defined;
        uint32_t physical;
        uint8_t old_value;
    } cases[] = {
        {0x0000u, 0x0009u, 0x0000u, 0x0000u, 0x0000u, 0x00000u, 0x00u},
        {0xFFFFu, 0x0018u, 0xFFFFu, 0xFFFEu, 0x0010u, 0xFFFFFu, 0x0Fu},
        {0x1234u, 0x0008u, 0x0001u, 0x0206u, 0x0890u, 0x2233Fu, 0x7Fu},
        {0xFFFFu, 0x0019u, 0xFFFEu, 0x0000u, 0x0054u, 0x00000u, 0xFFu},
        {0xF000u, 0x0007u, 0x1357u, 0x0800u, 0x0084u, 0xFFFFEu, 0xFEu},
        {0x2468u, 0x000Au, 0x8000u, 0x0244u, 0x0084u, 0x24681u, 0x80u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x08CBu;
    assert(pr_u3b0_08cb_ss_inc(NULL, PR_U3B0_RAM_SIZE,
                               &cpu) == PR_U3B0_08CB_SS_INC_ARGUMENT);
    assert(pr_u3b0_08cb_ss_inc(ram, PR_U3B0_RAM_SIZE - 1u,
                               &cpu) == PR_U3B0_08CB_SS_INC_ARGUMENT);
    assert(pr_u3b0_08cb_ss_inc(ram, PR_U3B0_RAM_SIZE,
                               NULL) == PR_U3B0_08CB_SS_INC_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        ram[0xF08CBu] = 0xFEu;
        ram[0xF08CCu] = 0x46u;
        ram[0xF08CDu] = 0xF7u;
        ram[cases[i].physical] = cases[i].old_value;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = 0x7F13u;
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
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x08CBu;
        expected = cpu;
        expected.flags = (uint16_t)((cases[i].flags & (uint16_t)~0x08D4u) |
                                    cases[i].expected_defined);
        expected.ip = 0x08CEu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        snapshot[cases[i].physical] = (uint8_t)(cases[i].old_value + 1u);
        assert(pr_u3b0_08cb_ss_inc(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_08CB_SS_INC_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x08CBu;
    for (j = 0; j < 3u; ++j) {
        ram[0xF08CBu + j] ^= 1u;
        reject_unchanged(ram, snapshot, &cpu);
        ram[0xF08CBu + j] ^= 1u;
    }
    cpu.flags |= 0x0001u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.flags &= (uint16_t)~0x0001u;
    cpu.ip = 0x08CEu;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
