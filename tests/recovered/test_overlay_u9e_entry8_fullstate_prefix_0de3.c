#include "poolrad/recovered/overlay_u9e_entry8_fullstate_prefix_0de3.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MASK (PR_U9E_ENTRY8_PREFIX_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void put8(uint8_t *ram, uint16_t segment, uint16_t offset,
                 uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static pr_u9e_entry8_prefix_cpu frame(uint8_t *ram, uint16_t sp,
                                      uint16_t count)
{
    pr_u9e_entry8_prefix_cpu cpu = {
        0xAB55u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
        0x7777u, sp, 0x3000u, 0x0DE3u, 0x1000u, 0x4000u,
        0x2000u, 0x7212u
    };
    memset(ram, 0xA5, PR_U9E_ENTRY8_PREFIX_RAM_SIZE);
    put16(ram, cpu.ss, sp, 0x0FE3u);
    put16(ram, cpu.ss, (uint16_t)(sp + 2u), 0xBEEFu);
    put16(ram, cpu.ss, (uint16_t)(sp + 4u), 0x9913u);
    put16(ram, cpu.ss, (uint16_t)(sp + 6u), count);
    return cpu;
}

static void test_call(uint8_t *ram, uint8_t *expected, uint16_t sp,
                      uint16_t count)
{
    pr_u9e_entry8_prefix_cpu cpu = frame(ram, sp, count);
    pr_u9e_entry8_prefix_cpu before = cpu;
    uint16_t bp = (uint16_t)(sp - 2u);
    memcpy(expected, ram, PR_U9E_ENTRY8_PREFIX_RAM_SIZE);
    put16(expected, before.ss, bp, before.bp);
    put8(expected, before.ss, (uint16_t)(bp - 3u), 0u);
    put8(expected, before.ss, (uint16_t)(bp - 4u), (uint8_t)count);
    put8(expected, before.ss, (uint16_t)(bp - 2u), 1u);
    put16(expected, before.ss, (uint16_t)(bp - 6u), 0x0013u);
    assert(pr_u9e_entry8_fullstate_prefix_0de3(
               ram, PR_U9E_ENTRY8_PREFIX_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_PREFIX_AT_CALL);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_PREFIX_RAM_SIZE) == 0);
    assert(cpu.ax == 0x0013u && cpu.bp == bp &&
           cpu.sp == (uint16_t)(sp - 8u) && cpu.ip == 0x0E0Eu &&
           cpu.flags == 0x7256u);
    assert(cpu.bx == before.bx && cpu.cx == before.cx &&
           cpu.dx == before.dx && cpu.si == before.si &&
           cpu.di == before.di && cpu.cs == before.cs &&
           cpu.ds == before.ds && cpu.es == before.es &&
           cpu.ss == before.ss);
}

int main(void)
{
    uint8_t *ram = malloc(PR_U9E_ENTRY8_PREFIX_RAM_SIZE);
    uint8_t *expected = malloc(PR_U9E_ENTRY8_PREFIX_RAM_SIZE);
    pr_u9e_entry8_prefix_cpu cpu, before;
    assert(ram != NULL && expected != NULL);

    cpu = frame(ram, 0x8000u, 1u);
    before = cpu;
    memcpy(expected, ram, PR_U9E_ENTRY8_PREFIX_RAM_SIZE);
    assert(pr_u9e_entry8_fullstate_prefix_0de3(
               NULL, PR_U9E_ENTRY8_PREFIX_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_PREFIX_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_prefix_0de3(ram, 0u, &cpu) ==
           PR_U9E_ENTRY8_PREFIX_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_prefix_0de3(
               ram, PR_U9E_ENTRY8_PREFIX_RAM_SIZE, NULL) ==
           PR_U9E_ENTRY8_PREFIX_ARGUMENT);
    cpu.ip = 0x0DE4u;
    assert(pr_u9e_entry8_fullstate_prefix_0de3(
               ram, PR_U9E_ENTRY8_PREFIX_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_PREFIX_ARGUMENT);
    cpu.ip = 0x0DE3u;
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_PREFIX_RAM_SIZE) == 0);

    test_call(ram, expected, 0x8000u, 1u);
    test_call(ram, expected, 0x8000u, 0xEE02u);
    test_call(ram, expected, 0xFFFEu, 1u);
    test_call(ram, expected, 0xFFFFu, 1u);

    /* Count zero branches to +0E1F; no range word is pushed. */
    cpu = frame(ram, 0x8000u, 0xEE00u);
    before = cpu;
    memcpy(expected, ram, PR_U9E_ENTRY8_PREFIX_RAM_SIZE);
    put16(expected, before.ss, (uint16_t)(before.sp - 2u), before.bp);
    put8(expected, before.ss, (uint16_t)(before.sp - 5u), 0u);
    assert(pr_u9e_entry8_fullstate_prefix_0de3(
               ram, PR_U9E_ENTRY8_PREFIX_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_PREFIX_AT_TAIL);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_PREFIX_RAM_SIZE) == 0);
    assert(cpu.ax == before.ax && cpu.bp == (uint16_t)(before.sp - 2u) &&
           cpu.sp == (uint16_t)(before.sp - 6u) &&
           cpu.ip == 0x0E1Fu && cpu.flags == 0x7246u);

    /* Reject a PUSH BP that would overwrite the pinned entry code. */
    cpu = frame(ram, 0x0DE5u, 1u);
    cpu.ss = cpu.cs;
    before = cpu;
    memcpy(expected, ram, PR_U9E_ENTRY8_PREFIX_RAM_SIZE);
    assert(pr_u9e_entry8_fullstate_prefix_0de3(
               ram, PR_U9E_ENTRY8_PREFIX_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_PREFIX_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_PREFIX_RAM_SIZE) == 0);

    free(expected);
    free(ram);
    return 0;
}
