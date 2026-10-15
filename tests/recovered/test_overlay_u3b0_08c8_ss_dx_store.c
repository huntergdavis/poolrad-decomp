#include "poolrad/recovered/overlay_u3b0_08c8_ss_dx_store.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reject_unchanged(uint8_t *ram, uint8_t *snapshot, pr_u3b0_cpu *cpu)
{
    pr_u3b0_cpu before = *cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_08c8_ss_dx_store(ram, PR_U3B0_RAM_SIZE, cpu) ==
           PR_U3B0_08C8_SS_DX_STORE_ARGUMENT);
    assert(memcmp(cpu, &before, sizeof(*cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
}

int main(void)
{
    static const struct {
        uint16_t ss, bp, sp, dx, flags;
        uint32_t low_physical, high_physical;
    } cases[] = {
        {0x0000u, 0x0002u, 0x0000u, 0x1234u, 0x0000u,
         0x00000u, 0x00001u},
        {0xFFFFu, 0x0011u, 0xFFFFu, 0xABCDu, 0xFFFEu,
         0xFFFFFu, 0x00000u},
        {0x1234u, 0x0001u, 0x0001u, 0x0E7Bu, 0x0840u,
         0x2233Fu, 0x12340u},
        {0xFFFFu, 0x0012u, 0xFFFEu, 0x5A69u, 0x0206u,
         0x00000u, 0x00001u},
        {0xF000u, 0x0000u, 0x1357u, 0xF012u, 0x0800u,
         0xFFFFEu, 0xFFFFFu},
        {0x2468u, 0x0003u, 0x8000u, 0xBEEFu, 0x0040u,
         0x24681u, 0x24682u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x08C8u;
    assert(pr_u3b0_08c8_ss_dx_store(NULL, PR_U3B0_RAM_SIZE,
                                    &cpu) == PR_U3B0_08C8_SS_DX_STORE_ARGUMENT);
    assert(pr_u3b0_08c8_ss_dx_store(ram, PR_U3B0_RAM_SIZE - 1u,
                                    &cpu) == PR_U3B0_08C8_SS_DX_STORE_ARGUMENT);
    assert(pr_u3b0_08c8_ss_dx_store(ram, PR_U3B0_RAM_SIZE,
                                    NULL) == PR_U3B0_08C8_SS_DX_STORE_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        uint16_t previous_offset = (uint16_t)(cases[i].bp - 4u);
        uint32_t previous_base = (uint32_t)cases[i].ss << 4;
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        ram[0xF08C8u] = 0x89u;
        ram[0xF08C9u] = 0x56u;
        ram[0xF08CAu] = 0xFEu;
        ram[(previous_base + previous_offset) & 0xFFFFFu] = 0x5Au;
        ram[(previous_base + (uint16_t)(previous_offset + 1u)) & 0xFFFFFu] = 0xA5u;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = 0x7F13u;
        cpu.bx = 0xDE17u;
        cpu.cx = 0x2345u;
        cpu.dx = cases[i].dx;
        cpu.si = 0x5678u;
        cpu.di = 0x9ABCu;
        cpu.bp = cases[i].bp;
        cpu.sp = cases[i].sp;
        cpu.ds = 0x1357u;
        cpu.es = 0x2468u;
        cpu.ss = cases[i].ss;
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x08C8u;
        expected = cpu;
        expected.ip = 0x08CBu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        snapshot[cases[i].low_physical] = (uint8_t)cases[i].dx;
        snapshot[cases[i].high_physical] = (uint8_t)(cases[i].dx >> 8);
        assert(pr_u3b0_08c8_ss_dx_store(ram, PR_U3B0_RAM_SIZE,
                                        &cpu) == PR_U3B0_08C8_SS_DX_STORE_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x08C8u;
    for (j = 0; j < 3u; ++j) {
        ram[0xF08C8u + j] ^= 1u;
        reject_unchanged(ram, snapshot, &cpu);
        ram[0xF08C8u + j] ^= 1u;
    }
    cpu.flags |= 0x0001u;
    reject_unchanged(ram, snapshot, &cpu);
    cpu.flags &= (uint16_t)~0x0001u;
    cpu.ip = 0x08CBu;
    reject_unchanged(ram, snapshot, &cpu);
    free(snapshot);
    free(ram);
    return 0;
}
