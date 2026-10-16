#include "poolrad/recovered/overlay_u1c_script_jmp_31a8.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RAM_SIZE 1048576u
#define CODE_ADDRESS 0x389c8u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xfffffu;
}

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t word)
{
    ram[physical(segment, offset)] = (uint8_t)word;
    ram[physical(segment, (uint16_t)(offset + 1u))] = (uint8_t)(word >> 8);
}

static pr_rec_u1c_33bd_cpu starting_cpu(uint16_t flags)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x31a8u, 0x14a0u, 0x3996u, 0x1d16u, 0x3eeeu,
        0x3eeeu, 0x1234u, 0x5678u, 0x9abcu, 0xdef0u, 0x2468u,
        0xace0u, flags
    };
    return cpu;
}

static void prepare(uint8_t *ram, uint8_t *copy)
{
    size_t i;
    for (i = 0u; i < MAX_RAM_SIZE; ++i)
        ram[i] = (uint8_t)((i * 37u + 13u) & 0xffu);
    ram[CODE_ADDRESS] = 0xebu;
    ram[CODE_ADDRESS + 1u] = 0xd5u;
    put_word(ram, 0x1d16u, 0x3eeau, 0x31a8u);
    put_word(ram, 0x1d16u, 0x3eecu, 0x3582u);
    memcpy(copy, ram, MAX_RAM_SIZE);
}

static void reject_unchanged(pr_rec_u1c_33bd_cpu *cpu, uint8_t *ram,
                             const uint8_t *copy, size_t ram_size)
{
    pr_rec_u1c_33bd_cpu before = *cpu;
    assert(pr_rec_u1c_31a8_jmp_loop(cpu, ram, ram_size) == 0);
    assert(memcmp(cpu, &before, sizeof(before)) == 0);
    assert(memcmp(ram, copy, MAX_RAM_SIZE) == 0);
}

static void check_size(size_t ram_size)
{
    uint8_t *ram = malloc(MAX_RAM_SIZE);
    uint8_t *copy = malloc(MAX_RAM_SIZE);
    pr_rec_u1c_33bd_cpu cpu, before;
    unsigned i;
    assert(ram != NULL && copy != NULL);
    prepare(ram, copy);

    for (i = 0u; i < 2u; ++i) {
        cpu = starting_cpu(i != 0u ? 0x0040u : 0x0285u);
        before = cpu;
        assert(pr_rec_u1c_31a8_jmp_loop(&cpu, ram, ram_size) == 1);
        before.ip = 0x317fu;
        assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
        assert(memcmp(ram, copy, MAX_RAM_SIZE) == 0);
    }

    cpu = starting_cpu(0x0000u);
    cpu.cs ^= 1u;
    reject_unchanged(&cpu, ram, copy, ram_size);
    cpu = starting_cpu(0x0000u);
    cpu.ip ^= 1u;
    reject_unchanged(&cpu, ram, copy, ram_size);
    cpu = starting_cpu(0x0000u);
    ram[CODE_ADDRESS] ^= 1u;
    copy[CODE_ADDRESS] ^= 1u;
    reject_unchanged(&cpu, ram, copy, ram_size);
    ram[CODE_ADDRESS] ^= 1u;
    copy[CODE_ADDRESS] ^= 1u;
    ram[CODE_ADDRESS + 1u] ^= 1u;
    copy[CODE_ADDRESS + 1u] ^= 1u;
    reject_unchanged(&cpu, ram, copy, ram_size);
    ram[CODE_ADDRESS + 1u] ^= 1u;
    copy[CODE_ADDRESS + 1u] ^= 1u;
    reject_unchanged(&cpu, ram, copy, ram_size - 1u);
    assert(pr_rec_u1c_31a8_jmp_loop(NULL, ram, ram_size) == 0);
    assert(memcmp(ram, copy, MAX_RAM_SIZE) == 0);
    assert(pr_rec_u1c_31a8_jmp_loop(&cpu, NULL, ram_size) == 0);
    assert(memcmp(&cpu, &(pr_rec_u1c_33bd_cpu){
        0x3582u, 0x31a8u, 0x14a0u, 0x3996u, 0x1d16u, 0x3eeeu,
        0x3eeeu, 0x1234u, 0x5678u, 0x9abcu, 0xdef0u, 0x2468u,
        0xace0u, 0x0000u
    }, sizeof(cpu)) == 0);
    free(copy);
    free(ram);
}

int main(void)
{
    check_size(655360u);
    check_size(1048576u);
    puts("U1C +31A8 JMP source-only synthetic C: PASS");
    return 0;
}
