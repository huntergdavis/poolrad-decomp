#include "poolrad/recovered/overlay_u3b0_08c1_es_dx_read.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_08c1_es_dx_read(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_08C1_ES_DX_READ_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t es, di, sp, dx, flags, value;
        uint32_t low_physical, high_physical;
    } cases[] = {
        {0x0000u, 0x0000u, 0x0000u, 0xA510u, 0x0000u, 0x1234u,
         0x0002Cu, 0x0002Du},
        {0xFFFFu, 0xFFE3u, 0xFFFFu, 0x7F22u, 0xFFFEu, 0xABCDu,
         0xFFFFFu, 0x00000u},
        {0x1234u, 0xFFD3u, 0x0001u, 0x8033u, 0x0840u, 0x0E7Bu,
         0x2233Fu, 0x12340u},
        {0xFFFFu, 0xFFE4u, 0xFFFEu, 0x0044u, 0x0206u, 0x5A69u,
         0x00000u, 0x00001u},
        {0xF000u, 0xFFD2u, 0x1357u, 0xFF55u, 0x0800u, 0xF012u,
         0xFFFFEu, 0xFFFFFu},
        {0x2468u, 0x0001u, 0x8000u, 0x7F66u, 0x0040u, 0xBEEFu,
         0x246ADu, 0x246AEu}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x08C1u;
    assert(pr_u3b0_08c1_es_dx_read(NULL, PR_U3B0_RAM_SIZE,
                                   &cpu) == PR_U3B0_08C1_ES_DX_READ_ARGUMENT);
    assert(pr_u3b0_08c1_es_dx_read(ram, PR_U3B0_RAM_SIZE - 1u,
                                   &cpu) == PR_U3B0_08C1_ES_DX_READ_ARGUMENT);
    assert(pr_u3b0_08c1_es_dx_read(ram, PR_U3B0_RAM_SIZE,
                                   NULL) == PR_U3B0_08C1_ES_DX_READ_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        uint16_t offset = (uint16_t)(cases[i].di + 0x2Cu);
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        ram[0xF08C1u] = 0x26u;
        ram[0xF08C2u] = 0x8Bu;
        ram[0xF08C3u] = 0x55u;
        ram[0xF08C4u] = 0x2Cu;
        ram[cases[i].low_physical] = (uint8_t)cases[i].value;
        ram[cases[i].high_physical] = (uint8_t)(cases[i].value >> 8);
        ram[(0x13570u + offset) & 0xFFFFFu] = 0x5Au;
        ram[(0x13570u + (uint16_t)(offset + 1u)) & 0xFFFFFu] = 0xA5u;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = 0xA5C3u;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = cases[i].dx;
        cpu.si = 0x5678u;
        cpu.di = cases[i].di;
        cpu.bp = 0xBEEFu;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = cases[i].es;
        cpu.ss = 0x2468u;
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x08C1u;
        expected = cpu;
        expected.dx = cases[i].value;
        expected.ip = 0x08C5u;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_08c1_es_dx_read(ram, PR_U3B0_RAM_SIZE,
                                       &cpu) == PR_U3B0_08C1_ES_DX_READ_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x08C1u;
    for (j = 0; j < 4u; ++j) {
        ram[0xF08C1u + j] ^= 1u;
        reject_unchanged(ram, snapshot, &cpu);
        ram[0xF08C1u + j] ^= 1u;
    }
    cpu.flags |= 0x0001u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.flags &= (uint16_t)~0x0001u;
    cpu.ip = 0x08C5u;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
