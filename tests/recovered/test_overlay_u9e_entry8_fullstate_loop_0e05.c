#include "poolrad/recovered/overlay_u9e_entry8_fullstate_loop_0e05.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIZE PR_U9E_ENTRY8_LOOP_RAM_SIZE
#define MASK (SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static pr_u9e_entry8_loop_cpu frame(uint8_t *ram, uint16_t bp,
                                    uint16_t sp, uint16_t ss,
                                    uint16_t ds, uint8_t index,
                                    uint8_t range, uint16_t flags)
{
    pr_u9e_entry8_loop_cpu cpu = {
        0xABCDu, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
        bp, sp, 0x3000u, 0x0E05u, ds, 0x4000u, ss, flags
    };
    memset(ram, 0xA5, SIZE);
    ram[physical(ss, (uint16_t)(bp - 2u))] = index;
    ram[physical(ss, (uint16_t)(bp + 6u))] = range;
    return cpu;
}

static void check_case(uint8_t *ram, uint8_t *expected,
                       uint16_t bp, uint16_t sp, uint16_t ss,
                       uint16_t ds, uint8_t index, uint8_t range,
                       uint16_t incoming_flags, uint16_t final_flags)
{
    pr_u9e_entry8_loop_cpu cpu = frame(
        ram, bp, sp, ss, ds, index, range, incoming_flags);
    pr_u9e_entry8_loop_cpu want = cpu;
    memcpy(expected, ram, SIZE);
    expected[physical(ss, (uint16_t)(bp - 2u))] = (uint8_t)(index + 1u);
    put16(expected, ss, (uint16_t)(sp - 2u), range);
    want.ax = range;
    want.sp = (uint16_t)(sp - 2u);
    want.ip = 0x0E0Eu;
    want.flags = final_flags;
    assert(pr_u9e_entry8_fullstate_loop_0e05(ram, SIZE, &cpu) ==
           PR_U9E_ENTRY8_LOOP_AT_CALL);
    assert(memcmp(ram, expected, SIZE) == 0);
    assert(memcmp(&cpu, &want, sizeof cpu) == 0);
}

int main(void)
{
    uint8_t *ram = malloc(SIZE);
    uint8_t *expected = malloc(SIZE);
    pr_u9e_entry8_loop_cpu cpu, before;
    assert(ram != NULL && expected != NULL);

    /* 0F->10 sets AF before XOR AH,AH; high AX is discarded. */
    check_case(ram, expected, 0x8002u, 0x7FFCu, 0x2000u,
               0x1000u, 0x0Fu, 0x14u, 0xF291u, 0xF254u);
    /* SP=0001 pushes at SS:FFFF: low byte is physical FFFFF,
     * high byte wraps to physical 00000, with AF clear. */
    check_case(ram, expected, 0x0007u, 0x0001u, 0xF000u,
               0x1000u, 0x10u, 0x00u, 0xF291u, 0xF244u);
    /* With DS=SS, PUSH AX at 4428 changes the live seed low WORD
     * before the SYSTEM callback. The range is reread from RAM. */
    check_case(ram, expected, 0x4430u, 0x442Au, 0x1000u,
               0x1000u, 0xFFu, 0x80u, 0x7A91u, 0x7254u);

    cpu = frame(ram, 0x0E07u, 0x0E01u, 0x3000u,
                0x1000u, 1u, 0x14u, 0xF291u);
    before = cpu;
    memcpy(expected, ram, SIZE);
    assert(pr_u9e_entry8_fullstate_loop_0e05(ram, SIZE, &cpu) ==
           PR_U9E_ENTRY8_LOOP_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, SIZE) == 0);

    cpu = frame(ram, 0x0E31u, 0x0E2Bu, 0x3000u,
                0x1000u, 1u, 0x14u, 0xF291u);
    before = cpu;
    memcpy(expected, ram, SIZE);
    assert(pr_u9e_entry8_fullstate_loop_0e05(ram, SIZE, &cpu) ==
           PR_U9E_ENTRY8_LOOP_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, SIZE) == 0);

    cpu = frame(ram, 0x8002u, 0x7FFCu, 0x2000u,
                0x1000u, 1u, 0x14u, 0xF291u);
    before = cpu;
    memcpy(expected, ram, SIZE);
    assert(pr_u9e_entry8_fullstate_loop_0e05(NULL, SIZE, &cpu) ==
           PR_U9E_ENTRY8_LOOP_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_loop_0e05(ram, SIZE - 1u, &cpu) ==
           PR_U9E_ENTRY8_LOOP_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_loop_0e05(ram, SIZE, NULL) ==
           PR_U9E_ENTRY8_LOOP_ARGUMENT);
    cpu.ip = 0x0E06u;
    assert(pr_u9e_entry8_fullstate_loop_0e05(ram, SIZE, &cpu) ==
           PR_U9E_ENTRY8_LOOP_ARGUMENT);
    cpu.ip = 0x0E05u;
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, SIZE) == 0);

    free(expected);
    free(ram);
    return 0;
}
