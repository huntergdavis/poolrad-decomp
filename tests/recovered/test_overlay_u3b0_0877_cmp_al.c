#include "poolrad/recovered/overlay_u3b0_0877_cmp_al.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const struct {
        uint16_t ss, di, sp, initial_flags, expected_flags;
        uint32_t physical;
        uint8_t al, source;
    } cases[] = {
        /* Literal SS physical addresses and full expected FLAGS words. */
        {0x0000u, 0x0054u, 0x0000u, 0xFFFFu, 0xF76Eu,
         0x00000u, 0x00u, 0x00u},
        {0xFFFFu, 0x0063u, 0xFFFFu, 0x0002u, 0x0097u,
         0xFFFFFu, 0x00u, 0x01u},
        {0xFFFFu, 0x0064u, 0x0001u, 0xF002u, 0xF883u,
         0x00000u, 0x7Fu, 0xFFu},
        {0x1234u, 0x0053u, 0xFFFEu, 0x0246u, 0x0A12u,
         0x2233Fu, 0x80u, 0x01u},
        {0xF000u, 0x0052u, 0x1357u, 0x0800u, 0x0004u,
         0xFFFFEu, 0x05u, 0x02u},
        {0x0000u, 0x0055u, 0x8000u, 0xAAAAu, 0xA23Eu,
         0x00001u, 0x10u, 0x01u}
    };
    static const uint8_t code[] = {0x36u, 0x3Au, 0x45u, 0xACu};
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x0877u;
    assert(pr_u3b0_0877_cmp_al(NULL, PR_U3B0_RAM_SIZE,
                              &cpu) == PR_U3B0_0877_CMP_AL_ARGUMENT);
    assert(pr_u3b0_0877_cmp_al(ram, PR_U3B0_RAM_SIZE - 1u,
                              &cpu) == PR_U3B0_0877_CMP_AL_ARGUMENT);
    assert(pr_u3b0_0877_cmp_al(ram, PR_U3B0_RAM_SIZE,
                              NULL) == PR_U3B0_0877_CMP_AL_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        /* Synthetic F000:0000 loaded copy of original +0877 bytes. */
        for (j = 0; j < sizeof(code); ++j)
            ram[0xF0877u + j] = code[j];
        ram[cases[i].physical] = cases[i].source;
        /* Poison DS at the same effective offset, distinct from SS. */
        ram[(0x13570u + (uint16_t)(cases[i].di - 0x54u)) & 0xFFFFFu] =
            (uint8_t)(cases[i].source ^ 0xFFu);
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = (uint16_t)(0xA500u | cases[i].al);
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = 0x89ABu;
        cpu.si = 0x5678u;
        cpu.di = cases[i].di;
        cpu.bp = 0xBEEFu;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = cases[i].ss;
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].initial_flags;
        cpu.ip = 0x0877u;
        expected = cpu;
        expected.flags = cases[i].expected_flags;
        expected.ip = 0x087Bu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0877_cmp_al(ram, PR_U3B0_RAM_SIZE,
                                  &cpu) == PR_U3B0_0877_CMP_AL_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x0877u;
    for (j = 0; j < sizeof(code); ++j) {
        ram[0xF0877u + j] ^= 1u;
        expected = cpu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_0877_cmp_al(ram, PR_U3B0_RAM_SIZE,
                                  &cpu) == PR_U3B0_0877_CMP_AL_ARGUMENT);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
        ram[0xF0877u + j] ^= 1u;
    }
    cpu.ip = 0x087Bu;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_0877_cmp_al(ram, PR_U3B0_RAM_SIZE,
                              &cpu) == PR_U3B0_0877_CMP_AL_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    free(snapshot);
    free(ram);
    return 0;
}
