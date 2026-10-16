#include "poolrad/recovered/overlay_u9e_entry8_fullstate_postcall_0e13.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MASK (PR_U9E_ENTRY8_POSTCALL_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void put8(uint8_t *ram, uint16_t segment, uint16_t offset,
                 uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static pr_u9e_entry8_postcall_cpu frame(uint8_t *ram, uint16_t ax,
                                        uint16_t bp, uint8_t sum,
                                        uint8_t index, uint8_t count)
{
    pr_u9e_entry8_postcall_cpu cpu = {
        ax, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
        bp, 0x7FF8u, 0x3000u, 0x0E13u, 0x1000u, 0x4000u,
        0x2000u, 0x7212u
    };
    memset(ram, 0xA5, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE);
    put8(ram, cpu.ss, (uint16_t)(bp - 3u), sum);
    put8(ram, cpu.ss, (uint16_t)(bp - 2u), index);
    put8(ram, cpu.ss, (uint16_t)(bp - 4u), count);
    return cpu;
}

static void check_case(uint8_t *ram, uint8_t *expected, uint16_t ax,
                       uint16_t bp, uint8_t index, uint8_t count,
                       uint8_t final_sum, uint16_t final_ax,
                       uint16_t final_flags,
                       pr_u9e_entry8_postcall_status status,
                       uint16_t final_ip)
{
    pr_u9e_entry8_postcall_cpu cpu = frame(ram, ax, bp, 0x10u, index, count);
    pr_u9e_entry8_postcall_cpu before = cpu;
    memcpy(expected, ram, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE);
    put8(expected, cpu.ss, (uint16_t)(bp - 3u), final_sum);
    assert(pr_u9e_entry8_fullstate_postcall_0e13(
               ram, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE, &cpu) == status);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE) == 0);
    assert(cpu.ax == final_ax && cpu.flags == final_flags &&
           cpu.ip == final_ip);
    assert(cpu.bx == before.bx && cpu.cx == before.cx &&
           cpu.dx == before.dx && cpu.si == before.si &&
           cpu.di == before.di && cpu.bp == before.bp &&
           cpu.sp == before.sp && cpu.cs == before.cs &&
           cpu.ds == before.ds && cpu.es == before.es &&
           cpu.ss == before.ss);
}

int main(void)
{
    uint8_t *ram = malloc(PR_U9E_ENTRY8_POSTCALL_RAM_SIZE);
    uint8_t *expected = malloc(PR_U9E_ENTRY8_POSTCALL_RAM_SIZE);
    pr_u9e_entry8_postcall_cpu cpu, before;
    assert(ram != NULL && expected != NULL);

    cpu = frame(ram, 0x12FEu, 0x8002u, 0x10u, 1u, 1u);
    before = cpu;
    memcpy(expected, ram, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE);
    assert(pr_u9e_entry8_fullstate_postcall_0e13(
               NULL, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_POSTCALL_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_postcall_0e13(ram, 0u, &cpu) ==
           PR_U9E_ENTRY8_POSTCALL_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_postcall_0e13(
               ram, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE, NULL) ==
           PR_U9E_ENTRY8_POSTCALL_ARGUMENT);
    cpu.ip = 0x0E14u;
    assert(pr_u9e_entry8_fullstate_postcall_0e13(
               ram, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_POSTCALL_ARGUMENT);
    cpu.ip = 0x0E13u;
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE) == 0);

    /* The first case falls into the tail; the second carries into AH. */
    check_case(ram, expected, 0x12FEu, 0x8002u, 1u, 1u,
               0x0Fu, 0x1201u, 0x7246u,
               PR_U9E_ENTRY8_POSTCALL_AT_TAIL, 0x0E1Fu);
    check_case(ram, expected, 0x12FFu, 0x8002u, 1u, 2u,
               0x10u, 0x1301u, 0x7297u,
               PR_U9E_ENTRY8_POSTCALL_AT_LOOP, 0x0E05u);
    /* Signed 0x80-1 sets OF; the byte locals still wrap at BP=0001. */
    check_case(ram, expected, 0x0000u, 0x0001u, 0x80u, 1u,
               0x11u, 0x0080u, 0x7A12u,
               PR_U9E_ENTRY8_POSTCALL_AT_LOOP, 0x0E05u);

    cpu = frame(ram, 0x12FEu, 0x0DE6u, 0x10u, 1u, 1u);
    cpu.ss = cpu.cs;
    before = cpu;
    memcpy(expected, ram, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE);
    assert(pr_u9e_entry8_fullstate_postcall_0e13(
               ram, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_POSTCALL_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, PR_U9E_ENTRY8_POSTCALL_RAM_SIZE) == 0);

    free(expected);
    free(ram);
    return 0;
}
