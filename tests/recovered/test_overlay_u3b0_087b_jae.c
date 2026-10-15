#include "poolrad/recovered/overlay_u3b0_087b_jae.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const struct {
        uint16_t flags, expected_ip, ss, sp;
    } cases[] = {
        {0x0000u, 0x08BAu, 0x0000u, 0x0000u},
        {0x0001u, 0x087Du, 0xFFFFu, 0xFFFFu},
        {0xFFFFu, 0x087Du, 0x1234u, 0x0001u},
        {0xFFFEu, 0x08BAu, 0xF000u, 0xFFFEu},
        {0x0080u, 0x08BAu, 0x1357u, 0x1357u},
        {0x0885u, 0x087Du, 0x2468u, 0x8000u}
    };
    uint8_t *ram = malloc(PR_U3B0_RAM_SIZE);
    uint8_t *snapshot = malloc(PR_U3B0_RAM_SIZE);
    pr_u3b0_cpu cpu = {0}, expected;
    size_t i, j;

    assert(ram != NULL && snapshot != NULL);
    assert(sizeof(cpu) == 14u * sizeof(uint16_t));
    cpu.ip = 0x087Bu;
    assert(pr_u3b0_087b_jae(NULL, PR_U3B0_RAM_SIZE,
                            &cpu) == PR_U3B0_087B_JAE_ARGUMENT);
    assert(pr_u3b0_087b_jae(ram, PR_U3B0_RAM_SIZE - 1u,
                            &cpu) == PR_U3B0_087B_JAE_ARGUMENT);
    assert(pr_u3b0_087b_jae(ram, PR_U3B0_RAM_SIZE,
                            NULL) == PR_U3B0_087B_JAE_ARGUMENT);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(ram, (int)(0x40u + i), PR_U3B0_RAM_SIZE);
        ram[0xF087Bu] = 0x73u;
        ram[0xF087Cu] = 0x3Du;
        cpu = (pr_u3b0_cpu){0};
        cpu.ax = 0xA5C3u;
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
        cpu.cs = 0xF000u;
        cpu.flags = cases[i].flags;
        cpu.ip = 0x087Bu;
        expected = cpu;
        expected.ip = cases[i].expected_ip;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_087b_jae(ram, PR_U3B0_RAM_SIZE,
                                &cpu) == PR_U3B0_087B_JAE_OK);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    }
    cpu.ip = 0x087Bu;
    for (j = 0; j < 2u; ++j) {
        ram[0xF087Bu + j] ^= 1u;
        expected = cpu;
        memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
        assert(pr_u3b0_087b_jae(ram, PR_U3B0_RAM_SIZE,
                                &cpu) == PR_U3B0_087B_JAE_ARGUMENT);
        assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
        assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
        ram[0xF087Bu + j] ^= 1u;
    }
    cpu.ip = 0x087Du;
    expected = cpu;
    memcpy(snapshot, ram, PR_U3B0_RAM_SIZE);
    assert(pr_u3b0_087b_jae(ram, PR_U3B0_RAM_SIZE,
                            &cpu) == PR_U3B0_087B_JAE_ARGUMENT);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, snapshot, PR_U3B0_RAM_SIZE) == 0);
    free(snapshot);
    free(ram);
    return 0;
}
