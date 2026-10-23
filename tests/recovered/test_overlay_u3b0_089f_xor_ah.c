#include "poolrad/recovered/overlay_u3b0_089f_xor_ah.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const struct {
        uint16_t ax, flags, expected_flags, ss, sp;
    } cases[] = {
        {0x0000u, 0x0000u, 0x0044u, 0x0000u, 0x0000u},
        {0xFFFFu, 0xFFFFu, 0xF76Eu, 0xFFFFu, 0xFFFFu},   /* pinned XOR clears the entry AF */
        {0x8000u, 0x0811u, 0x0044u, 0x1234u, 0x0001u},   /* pinned XOR clears the entry AF */
        {0x1234u, 0x0245u, 0x0244u, 0xFFFFu, 0xFFFEu},
        {0xABCDu, 0x0081u, 0x0044u, 0xF000u, 0x1357u},
        {0x5601u, 0x0AD5u, 0x0244u, 0x2468u, 0x8000u}    /* pinned XOR clears the entry AF */
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;
    uint32_t code_physical;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x089Fu;
    memset(ram, 0xA5, PR_U3B0_RAM_SIZE);
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    expected = cpu;
    assert(pr_u3b0_089f_xor_ah(NULL, PR_U3B0_RAM_SIZE,
                               &cpu) == PR_U3B0_089F_XOR_AH_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(pr_u3b0_089f_xor_ah(ram, PR_U3B0_RAM_SIZE - 1u,
                               &cpu) == PR_U3B0_089F_XOR_AH_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(pr_u3b0_089f_xor_ah(ram, PR_U3B0_RAM_SIZE,
                               NULL) == PR_U3B0_089F_XOR_AH_ARGUMENT);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        code_physical = ((uint32_t)(i == 5u ? 0xFFFFu : 0xF000u) << 4) + 0x089Fu;
        ram[code_physical & 0xFFFFFu] = 0x30u;
        ram[(code_physical + 1u) & 0xFFFFFu] = 0xE4u;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = cases[i].ax;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = 0x89ABu;
        cpu.si = 0x5678u;
        cpu.di = 0x9ABCu;
        cpu.bp = 0xBEEFu;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = cases[i].ss;
        cpu.cs = (uint16_t)(i == 5u ? 0xFFFFu : 0xF000u);
        cpu.flags = cases[i].flags;
        cpu.ip = 0x089Fu;
        expected = cpu;
        expected.ax &= 0x00FFu;
        expected.flags = cases[i].expected_flags;
        expected.ip = 0x08A1u;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_089f_xor_ah(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_089F_XOR_AH_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x089Fu;
    code_physical = ((uint32_t)cpu.cs << 4) + 0x089Fu;
    for (j = 0; j < 2u; ++j) {
        ram[(code_physical + j) & 0xFFFFFu] ^= 1u;
        expected = cpu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_089f_xor_ah(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_089F_XOR_AH_ARGUMENT);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
        ram[(code_physical + j) & 0xFFFFFu] ^= 1u;
    }
    cpu.ip = 0x08A1u;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_089f_xor_ah(ram, PR_U3B0_RAM_SIZE,
                               &cpu) == PR_U3B0_089F_XOR_AH_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    free(snapshot);
    free(ram);
    return 0;
}
