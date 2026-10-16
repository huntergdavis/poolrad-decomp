#include "poolrad/recovered/overlay_u3b0_0899_les_di.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const struct {
        uint16_t cs, ss, bp, sp, di, es, flags, new_di, new_es;
        uint32_t physical[4];
    } cases[] = {
        {0xF000u, 0x0000u, 0x0004u, 0x0000u, 0xA510u, 0xB612u, 0x0000u,
         0x1234u, 0x5678u, {0x00000u, 0x00001u, 0x00002u, 0x00003u}},
        {0xF000u, 0xFFFFu, 0x0013u, 0xFFFFu, 0x7F22u, 0xABCDu, 0x00C1u,
         0xCDEFu, 0x90ABu, {0xFFFFFu, 0x00000u, 0x00001u, 0x00002u}},
        {0xF000u, 0x1234u, 0x0003u, 0x0001u, 0x8033u, 0x2468u, 0xFFFEu,
         0x0E7Bu, 0x1359u, {0x2233Fu, 0x12340u, 0x12341u, 0x12342u}},
        {0xF000u, 0xFFFFu, 0x0014u, 0xFFFEu, 0x0044u, 0x1111u, 0x0206u,
         0x5A69u, 0x01F0u, {0x00000u, 0x00001u, 0x00002u, 0x00003u}},
        {0xF000u, 0xF000u, 0x0000u, 0x1357u, 0xFF55u, 0x2222u, 0x0800u,
         0xF012u, 0x0BADu, {0xFFFFCu, 0xFFFFDu, 0xFFFFEu, 0xFFFFFu}},
        {0xFFFFu, 0x0000u, 0x0004u, 0x1234u, 0x00FFu, 0xBEEFu, 0x0001u,
         0xA1B2u, 0xC3D4u, {0x00000u, 0x00001u, 0x00002u, 0x00003u}}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;
    uint32_t code_physical;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x0899u;
    memset(ram, 0xA5, PR_U3B0_RAM_SIZE);
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    expected = cpu;
    assert(pr_u3b0_0899_les_di(NULL, PR_U3B0_RAM_SIZE,
                               &cpu) == PR_U3B0_0899_LES_DI_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(pr_u3b0_0899_les_di(ram, PR_U3B0_RAM_SIZE - 1u,
                               &cpu) == PR_U3B0_0899_LES_DI_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(pr_u3b0_0899_les_di(ram, PR_U3B0_RAM_SIZE,
                               NULL) == PR_U3B0_0899_LES_DI_ARGUMENT);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const uint16_t words[2] = {cases[i].new_di, cases[i].new_es};
        uint16_t offset = (uint16_t)(cases[i].bp - 4u);
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        /* Synthetic loaded copy of original +0899 bytes. */
        code_physical = ((uint32_t)cases[i].cs << 4) + 0x0899u;
        ram[code_physical & 0xFFFFFu] = 0xC4u;
        ram[(code_physical + 1u) & 0xFFFFFu] = 0x7Eu;
        ram[(code_physical + 2u) & 0xFFFFFu] = 0xFCu;
        for (j = 0; j < 4u; ++j) {
            ram[cases[i].physical[j]] =
                (uint8_t)(words[j / 2u] >> (8u * (j % 2u)));
            ram[(0x13570u + (uint16_t)(offset + j)) & 0xFFFFFu] =
                (uint8_t)(0xA0u + j);
        }
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = 0xA5C3u;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = 0x89ABu;
        cpu.si = 0x5678u;
        cpu.di = cases[i].di;
        cpu.bp = cases[i].bp;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = cases[i].es;
        cpu.ss = cases[i].ss;
        cpu.cs = cases[i].cs;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x0899u;
        expected = cpu;
        expected.di = cases[i].new_di;
        expected.es = cases[i].new_es;
        expected.ip = 0x089Cu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0899_les_di(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_0899_LES_DI_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x0899u;
    code_physical = ((uint32_t)cpu.cs << 4) + 0x0899u;
    for (j = 0; j < 3u; ++j) {
        ram[(code_physical + j) & 0xFFFFFu] ^= 1u;
        expected = cpu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0899_les_di(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_0899_LES_DI_ARGUMENT);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
        ram[(code_physical + j) & 0xFFFFFu] ^= 1u;
    }
    cpu.ip = 0x089Cu;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_0899_les_di(ram, PR_U3B0_RAM_SIZE,
                               &cpu) == PR_U3B0_0899_LES_DI_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    free(snapshot);
    free(ram);
    return 0;
}
