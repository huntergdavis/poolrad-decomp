#include "overlay_u86_entry1_postdc0_lea_di_019c.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE PR_1816_CALLER_RAM_SIZE

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xfffffu;
}

static pr_1816_caller_cpu starting_cpu(uint16_t cs, uint16_t bp)
{
    pr_1816_caller_cpu cpu = {
        0x1234u, 0x5678u, 0x9abcu, 0xdef0u, 0x2468u, 0xace0u,
        bp, 0x3d00u, cs, 0x019cu, 0x14a0u, 0x3996u,
        0x1d16u, 0x0a55u
    };
    return cpu;
}

static void prepare(uint8_t *ram, uint16_t cs)
{
    size_t i;
    size_t address = physical(cs, 0x019cu);
    for (i = 0u; i < RAM_SIZE; ++i)
        ram[i] = (uint8_t)((i * 29u + 7u) & 0xffu);
    ram[address] = 0x8du;
    ram[(address + 1u) & 0xfffffu] = 0x7eu;
    ram[(address + 2u) & 0xfffffu] = 0xceu;
}

static void reject_unchanged(pr_1816_caller_cpu *cpu, uint8_t *ram,
                             const uint8_t *copy, size_t ram_size,
                             uint16_t supplied_cs)
{
    pr_1816_caller_cpu before = *cpu;
    assert(pr_u86_entry1_019c_lea_di(cpu, ram, ram_size, supplied_cs) == 1);
    assert(memcmp(cpu, &before, sizeof(before)) == 0);
    assert(memcmp(ram, copy, RAM_SIZE) == 0);
}

static void check_success(uint16_t cs, uint16_t bp)
{
    uint8_t *ram = malloc(RAM_SIZE);
    uint8_t *copy = malloc(RAM_SIZE);
    pr_1816_caller_cpu cpu = starting_cpu(cs, bp);
    pr_1816_caller_cpu expected = cpu;
    assert(ram != NULL && copy != NULL);
    prepare(ram, cs);
    memcpy(copy, ram, RAM_SIZE);
    expected.di = (uint16_t)(bp - 0x32u);
    expected.ip = 0x019fu;
    assert(pr_u86_entry1_019c_lea_di(&cpu, ram, RAM_SIZE, cs) == 0);
    assert(memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(memcmp(ram, copy, RAM_SIZE) == 0);
    free(copy);
    free(ram);
}

static void check_rejections(uint16_t cs)
{
    uint8_t *ram = malloc(RAM_SIZE);
    uint8_t *copy = malloc(RAM_SIZE);
    pr_1816_caller_cpu cpu = starting_cpu(cs, 0x0032u);
    pr_1816_caller_cpu before;
    size_t address = physical(cs, 0x019cu);
    size_t byte;
    assert(ram != NULL && copy != NULL);
    prepare(ram, cs);
    memcpy(copy, ram, RAM_SIZE);

    reject_unchanged(&cpu, ram, copy, RAM_SIZE, (uint16_t)(cs ^ 1u));
    cpu.cs ^= 1u;
    reject_unchanged(&cpu, ram, copy, RAM_SIZE, cs);
    cpu.cs ^= 1u;
    cpu.ip ^= 1u;
    reject_unchanged(&cpu, ram, copy, RAM_SIZE, cs);
    cpu.ip ^= 1u;
    for (byte = 0u; byte < 3u; ++byte) {
        size_t at = (address + byte) & 0xfffffu;
        ram[at] ^= 1u;
        copy[at] ^= 1u;
        reject_unchanged(&cpu, ram, copy, RAM_SIZE, cs);
        ram[at] ^= 1u;
        copy[at] ^= 1u;
    }
    reject_unchanged(&cpu, ram, copy, RAM_SIZE - 1u, cs);
    reject_unchanged(&cpu, ram, copy, RAM_SIZE + 1u, cs);
    assert(pr_u86_entry1_019c_lea_di(NULL, ram, RAM_SIZE, cs) == 1);
    assert(memcmp(ram, copy, RAM_SIZE) == 0);
    before = cpu;
    assert(pr_u86_entry1_019c_lea_di(&cpu, NULL, RAM_SIZE, cs) == 1);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    free(copy);
    free(ram);
}

int main(void)
{
    check_success(0x3456u, 0x0000u); /* DI wraps to FFCEh. */
    check_success(0x3456u, 0x0032u);
    check_success(0x3456u, 0xffffu);
    check_success(0xffffu, 0x0001u); /* A20-wrapped code address. */
    check_rejections(0x3456u);
    check_rejections(0xffffu);
    puts("U86 +019C LEA DI source-only synthetic C: PASS");
    return 0;
}
