#include "poolrad/recovered/system_random_bit_fullstate_11d6.h"

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

static pr_system_11d6_cpu frame(uint8_t *ram, uint16_t low, uint16_t high)
{
    pr_system_11d6_cpu cpu = {
        0x1111u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
        0x7777u, 0x8000u, 0x3000u, 0x11D6u, 0x1000u, 0x4000u,
        0x2000u, 0x7202u
    };
    memset(ram, 0xA5, PR_SYSTEM_120F_RAM_SIZE);
    put(ram, cpu.cs, 0x1245u, 0x8405u);
    put(ram, cpu.ds, 0x4428u, low);
    put(ram, cpu.ds, 0x442Au, high);
    put(ram, cpu.ss, cpu.sp, 0xBEEFu);
    put(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xCAFEu);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_SYSTEM_120F_RAM_SIZE);
    uint8_t *expected = malloc(PR_SYSTEM_120F_RAM_SIZE);
    pr_system_11d6_cpu cpu, before;
    assert(ram != NULL && expected != NULL);

    cpu = frame(ram, 0x5678u, 0x1234u);
    before = cpu;
    memcpy(expected, ram, PR_SYSTEM_120F_RAM_SIZE);
    assert(pr_system_random_bit_fullstate_11d6(NULL, PR_SYSTEM_120F_RAM_SIZE,
                                                &cpu) == PR_SYSTEM_11D6_ARGUMENT);
    assert(pr_system_random_bit_fullstate_11d6(ram, 0u, &cpu) ==
           PR_SYSTEM_11D6_ARGUMENT);
    assert(pr_system_random_bit_fullstate_11d6(ram, PR_SYSTEM_120F_RAM_SIZE,
                                                NULL) == PR_SYSTEM_11D6_ARGUMENT);
    cpu.ip = 0x11D7u;
    assert(pr_system_random_bit_fullstate_11d6(ram, PR_SYSTEM_120F_RAM_SIZE,
                                                &cpu) == PR_SYSTEM_11D6_ARGUMENT);
    cpu.ip = 0x11D6u;
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_SYSTEM_120F_RAM_SIZE) == 0);

    /* The seed high bit is set, so the first TEST exits before any shift. */
    assert(pr_system_random_bit_fullstate_11d6(ram, PR_SYSTEM_120F_RAM_SIZE,
                                                &cpu) == PR_SYSTEM_11D6_RETURNED);
    assert(cpu.ax == 0x0080u && cpu.bx == 0x9059u &&
           cpu.cx == 0x0020u && cpu.dx == 0x4B5Bu);
    assert(cpu.sp == 0x8004u && cpu.ip == 0xBEEFu && cpu.cs == 0xCAFEu);
    assert(cpu.flags == 0x7206u);
    assert(cpu.si == before.si && cpu.di == before.di &&
           cpu.bp == before.bp && cpu.ds == before.ds &&
           cpu.es == before.es && cpu.ss == before.ss);
    put(expected, before.ss, (uint16_t)(before.sp - 2u), 0x11D9u);
    put(expected, before.ds, 0x4428u, 0x9059u);
    put(expected, before.ds, 0x442Au, 0xCB5Bu);
    assert(memcmp(ram, expected, PR_SYSTEM_120F_RAM_SIZE) == 0);

    /* A low seed of one and zero high seed takes 31 shifts to test bit 31. */
    cpu = frame(ram, 0u, 0u);
    assert(pr_system_random_bit_fullstate_11d6(ram, PR_SYSTEM_120F_RAM_SIZE,
                                                &cpu) == PR_SYSTEM_11D6_RETURNED);
    assert(word(ram, cpu.ds, 0x4428u) == 1u &&
           word(ram, cpu.ds, 0x442Au) == 0u);
    assert(cpu.ax == 0x0061u && cpu.bx == 0u &&
           cpu.cx == 1u && cpu.dx == 0u);

    /* A seed write can redirect the live near return before this wrapper. */
    cpu = frame(ram, 0x5678u, 0x1234u);
    cpu.ss = cpu.ds;
    cpu.sp = 0x442Au;
    put(ram, cpu.ss, cpu.sp, 0xBEEFu);
    put(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xCAFEu);
    assert(pr_system_random_bit_fullstate_11d6(ram, PR_SYSTEM_120F_RAM_SIZE,
                                                &cpu) == PR_SYSTEM_11D6_CALLEE_TRANSFER);
    assert(cpu.ip == word(ram, cpu.ds, 0x4428u) &&
           cpu.ip != 0x11D9u && cpu.sp == 0x442Au && cpu.cs == 0x3000u);

    /* RETF reads CS after offset wrap, rather than IP's linear +2. */
    for (unsigned i = 0u; i < 2u; ++i) {
        uint16_t sp = i == 0u ? 0xFFFEu : 0xFFFFu;
        size_t linear_cs;
        cpu = frame(ram, 0x5678u, 0x1234u);
        cpu.sp = sp;
        put(ram, cpu.ss, sp, 0xBEEFu);
        put(ram, cpu.ss, (uint16_t)(sp + 2u), 0xCAFEu);
        linear_cs = (physical(cpu.ss, sp) + 2u) & MASK;
        ram[linear_cs] = 0xADu;
        ram[(linear_cs + 1u) & MASK] = 0xDEu;
        assert(pr_system_random_bit_fullstate_11d6(ram, PR_SYSTEM_120F_RAM_SIZE,
                                                    &cpu) == PR_SYSTEM_11D6_RETURNED);
        assert(cpu.ip == 0xBEEFu && cpu.cs == 0xCAFEu &&
               cpu.sp == (uint16_t)(sp + 4u));
    }

    /* The near call must not overwrite its own pinned instruction body. */
    cpu = frame(ram, 0x5678u, 0x1234u);
    cpu.ss = cpu.cs;
    cpu.sp = 0x11DBu;
    before = cpu;
    memcpy(expected, ram, PR_SYSTEM_120F_RAM_SIZE);
    assert(pr_system_random_bit_fullstate_11d6(ram, PR_SYSTEM_120F_RAM_SIZE,
                                                &cpu) == PR_SYSTEM_11D6_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_SYSTEM_120F_RAM_SIZE) == 0);

    free(expected);
    free(ram);
    return 0;
}
