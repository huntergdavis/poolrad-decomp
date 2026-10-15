#include "poolrad/recovered/resident_time_caller_08ae_006b.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { RAM_SIZE = 655360, STACK_BASE = 0x1d16 * 16 };

static void put_word(uint8_t *ram, unsigned offset, uint16_t value)
{
    ram[STACK_BASE + offset] = (uint8_t)value;
    ram[STACK_BASE + offset + 1u] = (uint8_t)(value >> 8);
}

static uint16_t get_word(const uint8_t *ram, unsigned offset)
{
    return (uint16_t)(ram[STACK_BASE + offset] |
                      ((uint16_t)ram[STACK_BASE + offset + 1u] << 8));
}

static void read_ram(const char *path, uint8_t *ram)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL || fread(ram, 1, RAM_SIZE, file) != RAM_SIZE ||
        fgetc(file) != EOF) {
        fprintf(stderr, "cannot read 640 KiB RAM image: %s\n", path);
        exit(2);
    }
    fclose(file);
}

int main(int argc, char **argv)
{
    pr_rec_res_08ae_001f_cpu cpu = {
        0x10d2u, 0x006bu, 0x14a0u, 0x1d16u, 0x1d16u, 0x3da8u,
        0x3db4u, 0x8d94u, 0x0005u, 0x8d7cu, 0x0005u, 0x0001u,
        0x0000u, 0x7206u
    };
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    uint8_t *expected = NULL;
    assert(ram != NULL);

    if (argc == 3) {
        read_ram(argv[1], ram);
        expected = malloc(RAM_SIZE);
        assert(expected != NULL);
        read_ram(argv[2], expected);
    } else {
        assert(argc == 1);
        put_word(ram, 0x3da8u, 0x008du);
        put_word(ram, 0x3daau, 0x10d2u);
        put_word(ram, 0x3dacu, 0u); /* Verify both stores, not only CPU. */
        put_word(ram, 0x3daeu, 0u);
        put_word(ram, 0x3db0u, 0x8d9eu);
        put_word(ram, 0x3db2u, 0x0005u);
        put_word(ram, 0x3dbau, 0x000au);
    }

    pr_rec_res_08ae_006b_return(&cpu, ram);
    assert(cpu.cs == 0x10d2u && cpu.ip == 0x008du && cpu.sp == 0x3dacu);
    assert(cpu.ax == 0x8d94u && cpu.dx == 5u && cpu.flags == 0x7206u);
    pr_rec_res_08ae_008d_to_00aa(&cpu, ram);
    assert(cpu.cs == 0x10d2u && cpu.ip == 0x00aau);
    assert(cpu.ds == 0x14a0u && cpu.es == 0x1d16u && cpu.ss == 0x1d16u);
    assert(cpu.sp == 0x3dacu && cpu.bp == 0x3db4u);
    assert(cpu.ax == 0x000au && cpu.bx == 0u && cpu.cx == 0x000au);
    assert(cpu.dx == 0u && cpu.si == 1u && cpu.di == 0u);
    assert(cpu.flags == 0x7246u);
    assert(get_word(ram, 0x3dacu) == 0x8d94u);
    assert(get_word(ram, 0x3daeu) == 0x0005u);

    if (expected != NULL) {
        size_t mismatches = 0u;
        for (size_t i = 0u; i < RAM_SIZE; ++i)
            if (ram[i] != expected[i]) ++mismatches;
        printf("+006B through +00AA RAM mismatches: %zu/%u\n",
               mismatches, RAM_SIZE);
        assert(mismatches == 0u);
    }

    /* A low-word borrow must propagate into the high-word SBB. */
    memset(ram, 0, RAM_SIZE);
    put_word(ram, 0x3da8u, 0x008du);
    put_word(ram, 0x3daau, 0x10d2u);
    put_word(ram, 0x3db0u, 0x0000u);
    put_word(ram, 0x3db2u, 0x0005u);
    put_word(ram, 0x3dbau, 0x0001u);
    cpu = (pr_rec_res_08ae_001f_cpu){
        0x10d2u, 0x006bu, 0x14a0u, 0x1d16u, 0x1d16u, 0x3da8u,
        0x3db4u, 0x0001u, 0u, 0u, 0x0005u, 1u, 0u, 0x7206u
    };
    pr_rec_res_08ae_006b_return(&cpu, ram);
    pr_rec_res_08ae_008d_to_00aa(&cpu, ram);
    assert(cpu.ip == 0x00aau && cpu.ax == 0xffffu && cpu.dx == 0xffffu);
    assert(cpu.cx == 1u && cpu.bx == 0u && cpu.flags == 0x7286u);
    assert(get_word(ram, 0x3dacu) == 1u);
    assert(get_word(ram, 0x3daeu) == 5u);
    free(expected);
    free(ram);
    return 0;
}
