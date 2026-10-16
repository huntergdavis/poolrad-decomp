#include "poolrad/recovered/system_seed_fullstate_120f.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MASK (PR_SYSTEM_120F_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void put(uint8_t *ram, uint16_t segment, uint16_t offset,
                uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static pr_system_120f_cpu frame(uint8_t *ram, uint32_t seed)
{
    pr_system_120f_cpu cpu = {
        0x1111u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
        0x7777u, 0x8000u, 0x3000u, 0x120Fu, 0x1000u, 0x4000u,
        0x2000u, 0x7202u
    };
    memset(ram, 0xA5, PR_SYSTEM_120F_RAM_SIZE);
    put(ram, cpu.cs, 0x1245u, 0x8405u);
    put(ram, cpu.ds, 0x4428u, (uint16_t)seed);
    put(ram, cpu.ds, 0x442Au, (uint16_t)(seed >> 16));
    put(ram, cpu.ss, cpu.sp, 0xBEEFu);
    return cpu;
}

static uint32_t seed_at(const uint8_t *ram, uint16_t ds)
{
    return (uint32_t)word(ram, ds, 0x4428u) |
           ((uint32_t)word(ram, ds, 0x442Au) << 16);
}

int main(void)
{
    uint8_t *ram = malloc(PR_SYSTEM_120F_RAM_SIZE);
    uint8_t *expected = malloc(PR_SYSTEM_120F_RAM_SIZE);
    pr_system_120f_cpu cpu, before;
    assert(ram != NULL && expected != NULL);

    cpu = frame(ram, UINT32_C(0x12345678));
    before = cpu;
    memcpy(expected, ram, PR_SYSTEM_120F_RAM_SIZE);
    assert(pr_system_seed_fullstate_120f(NULL, PR_SYSTEM_120F_RAM_SIZE,
                                         &cpu) == PR_SYSTEM_120F_ARGUMENT);
    assert(pr_system_seed_fullstate_120f(ram, 0u, &cpu) ==
           PR_SYSTEM_120F_ARGUMENT);
    assert(pr_system_seed_fullstate_120f(ram, PR_SYSTEM_120F_RAM_SIZE,
                                         NULL) == PR_SYSTEM_120F_ARGUMENT);
    cpu.ip = 0x1210u;
    assert(pr_system_seed_fullstate_120f(ram, PR_SYSTEM_120F_RAM_SIZE,
                                         &cpu) == PR_SYSTEM_120F_ARGUMENT);
    cpu.ip = 0x120Fu;
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_SYSTEM_120F_RAM_SIZE) == 0);

    /* A seed write overlapping the original CS body cannot be modeled
     * as a fixed instruction stream; reject before any CPU/RAM effect. */
    cpu = frame(ram, UINT32_C(0x12345678));
    cpu.ds = (uint16_t)(cpu.cs - 0x031Fu);
    put(ram, cpu.ds, 0x4428u, 0x5678u);
    put(ram, cpu.ds, 0x442Au, 0x1234u);
    assert(physical(cpu.ds, 0x4428u) == physical(cpu.cs, 0x1238u));
    before = cpu;
    memcpy(expected, ram, PR_SYSTEM_120F_RAM_SIZE);
    assert(pr_system_seed_fullstate_120f(ram, PR_SYSTEM_120F_RAM_SIZE,
                                         &cpu) == PR_SYSTEM_120F_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_SYSTEM_120F_RAM_SIZE) == 0);

    cpu = frame(ram, UINT32_C(0x12345678));
    before = cpu;
    memcpy(expected, ram, PR_SYSTEM_120F_RAM_SIZE);

    assert(pr_system_seed_fullstate_120f(ram, PR_SYSTEM_120F_RAM_SIZE,
                                         &cpu) == PR_SYSTEM_120F_RETURNED);
    assert(seed_at(ram, cpu.ds) == UINT32_C(0xCB5B9059));
    assert(cpu.ax == 0x9059u && cpu.bx == 0x1A00u &&
           cpu.cx == 0x7305u && cpu.dx == 0xCB5Bu);
    assert(cpu.flags == 0x7282u && cpu.ip == 0xBEEFu &&
           cpu.sp == 0x8002u && cpu.cs == before.cs);
    assert(cpu.si == before.si && cpu.di == before.di &&
           cpu.bp == before.bp && cpu.ds == before.ds &&
           cpu.es == before.es && cpu.ss == before.ss);
    put(expected, cpu.ds, 0x4428u, 0x9059u);
    put(expected, cpu.ds, 0x442Au, 0xCB5Bu);
    assert(memcmp(ram, expected, PR_SYSTEM_120F_RAM_SIZE) == 0);

    /* CS:[1245] is read from RAM; it is not an embedded host constant. */
    cpu = frame(ram, UINT32_C(0x12345678));
    put(ram, cpu.cs, 0x1245u, 0x1234u);
    assert(pr_system_seed_fullstate_120f(ram, PR_SYSTEM_120F_RAM_SIZE,
                                         &cpu) == PR_SYSTEM_120F_RETURNED);
    assert(cpu.ax == 0x0061u && word(ram, cpu.ds, 0x4428u) == 0x0061u);

    /* The ordered seed write changes the live near return word. */
    cpu = frame(ram, UINT32_C(0x12345678));
    cpu.ss = cpu.ds;
    cpu.sp = 0x4428u;
    assert(pr_system_seed_fullstate_120f(ram, PR_SYSTEM_120F_RAM_SIZE,
                                         &cpu) == PR_SYSTEM_120F_RETURNED);
    assert(seed_at(ram, cpu.ds) == UINT32_C(0xCB5B9059));
    assert(cpu.ip == 0x9059u && cpu.sp == 0x442Au);

    /* Near RET advances SP modulo 65536 and reads a physical word. */
    cpu = frame(ram, UINT32_C(0x12345678));
    cpu.sp = 0xFFFFu;
    put(ram, cpu.ss, cpu.sp, 0xCAFEu);
    assert(pr_system_seed_fullstate_120f(ram, PR_SYSTEM_120F_RAM_SIZE,
                                         &cpu) == PR_SYSTEM_120F_RETURNED);
    assert(cpu.ip == 0xCAFEu && cpu.sp == 0x0001u);

    /* ADD AX,1 carries into ADC DX,0; exercise OF and CF separately. */
    cpu = frame(ram, UINT32_C(0xA6B05733));
    assert(pr_system_seed_fullstate_120f(ram, PR_SYSTEM_120F_RAM_SIZE,
                                         &cpu) == PR_SYSTEM_120F_RETURNED);
    assert(seed_at(ram, cpu.ds) == UINT32_C(0x80000000));
    assert(cpu.flags == 0x7A96u);
    cpu = frame(ram, UINT32_C(0x26B05733));
    assert(pr_system_seed_fullstate_120f(ram, PR_SYSTEM_120F_RAM_SIZE,
                                         &cpu) == PR_SYSTEM_120F_RETURNED);
    assert(seed_at(ram, cpu.ds) == 0u);
    assert(cpu.flags == 0x7257u);

    free(expected);
    free(ram);
    return 0;
}
