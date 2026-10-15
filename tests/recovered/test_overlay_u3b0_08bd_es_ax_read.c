#include "poolrad/recovered/overlay_u3b0_08bd_es_ax_read.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const struct {
        uint16_t es, di, sp, ax, flags, value;
        uint32_t low_physical, high_physical;
    } cases[] = {
        {0x0000u, 0x0000u, 0x0000u, 0xA510u, 0x0000u, 0x1234u,
         0x0002Au, 0x0002Bu},
        {0xFFFFu, 0xFFE5u, 0xFFFFu, 0x7F22u, 0xFFFEu, 0xABCDu,
         0xFFFFFu, 0x00000u},
        {0x1234u, 0xFFD5u, 0x0001u, 0x8033u, 0x0840u, 0x0E7Bu,
         0x2233Fu, 0x12340u},
        {0xFFFFu, 0xFFE6u, 0xFFFEu, 0x0044u, 0x0206u, 0x5A69u,
         0x00000u, 0x00001u},
        {0xF000u, 0xFFD4u, 0x1357u, 0xFF55u, 0x0800u, 0xF012u,
         0xFFFFEu, 0xFFFFFu},
        {0x2468u, 0x0001u, 0x8000u, 0x7F66u, 0x0040u, 0xBEEFu,
         0x246ABu, 0x246ACu}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x08BDu;
    assert(pr_u3b0_08bd_es_ax_read(NULL, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_08BD_ES_AX_READ_ARGUMENT);
    assert(pr_u3b0_08bd_es_ax_read(ram, PR_U3B0_RAM_SIZE - 1u,
                                   &cpu) == PR_U3B0_08BD_ES_AX_READ_ARGUMENT);
    assert(pr_u3b0_08bd_es_ax_read(ram, PR_U3B0_RAM_SIZE,
                                   NULL) == PR_U3B0_08BD_ES_AX_READ_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        uint16_t offset = (uint16_t)(cases[i].di + 0x2Au);
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        ram[0xF08BDu] = 0x26u;
        ram[0xF08BEu] = 0x8Bu;
        ram[0xF08BFu] = 0x45u;
        ram[0xF08C0u] = 0x2Au;
        ram[cases[i].low_physical] = (uint8_t)cases[i].value;
        ram[cases[i].high_physical] = (uint8_t)(cases[i].value >> 8);
        ram[(0x13570u + offset) & 0xFFFFFu] = 0x5Au;
        ram[(0x13570u + (uint16_t)(offset + 1u)) & 0xFFFFFu] = 0xA5u;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = cases[i].ax;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = 0x89ABu;
        cpu.si = 0x5678u;
        cpu.di = cases[i].di;
        cpu.bp = 0xBEEFu;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = cases[i].es;
        cpu.ss = 0x2468u;
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x08BDu;
        expected = cpu;
        expected.ax = cases[i].value;
        expected.ip = 0x08C1u;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_08bd_es_ax_read(ram, PR_U3B0_RAM_SIZE,
                                       &cpu) == PR_U3B0_08BD_ES_AX_READ_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x08BDu;
    for (j = 0; j < 4u; ++j) {
        ram[0xF08BDu + j] ^= 1u;
        expected = cpu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_08bd_es_ax_read(ram, PR_U3B0_RAM_SIZE,
                                       &cpu) == PR_U3B0_08BD_ES_AX_READ_ARGUMENT);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
        ram[0xF08BDu + j] ^= 1u;
    }
    cpu.flags |= 0x0001u;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_08bd_es_ax_read(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_08BD_ES_AX_READ_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    cpu.ip = 0x08C1u;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_08bd_es_ax_read(ram, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_08BD_ES_AX_READ_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    free(snapshot);
    free(ram);
    return 0;
}
