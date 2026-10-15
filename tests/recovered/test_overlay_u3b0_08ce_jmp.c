#include "poolrad/recovered/overlay_u3b0_08ce_jmp.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_08ce_jmp(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_08CE_JMP_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t cs, flags, bp, sp;
    } cases[] = {
        {0xF000u, 0x0000u, 0x0000u, 0x0000u},
        {0xFFFFu, 0x0894u, 0x0009u, 0xFFFFu},
        {0x0000u, 0x0054u, 0xFFF8u, 0x0001u},
        {0x1234u, 0xFFFEu, 0x5678u, 0x8000u},
        {0x8000u, 0x0084u, 0xABCDu, 0x1357u},
        {0xF800u, 0x0010u, 0xFFFFu, 0x2468u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    uint32_t code_base;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x08CEu;
    assert(pr_u3b0_08ce_jmp(NULL, PR_U3B0_RAM_SIZE, &cpu) ==
           PR_U3B0_08CE_JMP_ARGUMENT);
    assert(pr_u3b0_08ce_jmp(ram, PR_U3B0_RAM_SIZE - 1u, &cpu) ==
           PR_U3B0_08CE_JMP_ARGUMENT);
    assert(pr_u3b0_08ce_jmp(ram, PR_U3B0_RAM_SIZE, NULL) ==
           PR_U3B0_08CE_JMP_ARGUMENT);

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x50u + i), PR_U3B0_RAM_SIZE);
        code_base = (uint32_t)cases[i].cs << 4;
        ram[(code_base + 0x08CEu) & 0xFFFFFu] = 0xE9u;
        ram[(code_base + 0x08CFu) & 0xFFFFFu] = 0x3Bu;
        ram[(code_base + 0x08D0u) & 0xFFFFFu] = 0xFFu;
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
        cpu.ss = 0xFFFFu;
        cpu.cs = cases[i].cs;
        cpu.ip = 0x08CEu;
        cpu.flags = cases[i].flags;
        expected = cpu;
        expected.ip = 0x080Cu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_08ce_jmp(ram, PR_U3B0_RAM_SIZE, &cpu) ==
               PR_U3B0_08CE_JMP_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }

    cpu.ip = 0x08CEu;
    code_base = (uint32_t)cpu.cs << 4;
    for (j = 0; j < 3u; ++j) {
        ram[(code_base + 0x08CEu + j) & 0xFFFFFu] ^= 1u;
        reject_unchanged(ram, snapshot, &cpu);
        ram[(code_base + 0x08CEu + j) & 0xFFFFFu] ^= 1u;
    }
    cpu.flags |= 0x0001u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.flags &= (uint16_t)~0x0001u;
    cpu.ip = 0x08D1u;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
