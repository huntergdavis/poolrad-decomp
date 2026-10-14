#include "poolrad/recovered/overlay_u77_stack_189d.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static size_t addr(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U77_189D_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = addr(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U77_189D_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void put_far(uint8_t *ram, uint16_t seg, uint16_t off,
                    uint16_t target_off, uint16_t target_seg)
{
    put16(ram, seg, off, target_off);
    put16(ram, seg, (uint16_t)(off + 2u), target_seg);
}

static int calls;

static int remove_node(void *context, uint8_t *ram, size_t ram_size,
                       pr_u77_189d_cpu *cpu, uint16_t node_off,
                       uint16_t node_seg, uint16_t char_off,
                       uint16_t char_seg)
{
    (void)context;
    (void)ram;
    (void)cpu;
    assert(ram_size == PR_U77_189D_RAM_SIZE);
    assert(node_off == 0x0100u && node_seg == 0x4100u);
    assert(char_off == 0x0100u && char_seg == 0x3000u);
    ++calls;
    return 1;
}

static pr_u77_189d_cpu start_cpu(void)
{
    pr_u77_189d_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x189Du, 0x1000u, 0x1111u,
        0xE000u, 0x7202u
    };
    return cpu;
}

static void prepare(uint8_t *ram, uint16_t first_seg,
                    uint8_t target_count, uint8_t first_count)
{
    put16(ram, 0xE000u, 0x8000u, 0x0712u);
    put16(ram, 0xE000u, 0x8002u, 0xF000u);
    put_far(ram, 0xE000u, 0x8004u, 0x0100u, 0x4000u);
    put_far(ram, 0x1000u, 0x5D92u, 0x0100u, 0x3000u);
    put_far(ram, 0x3000u, 0x01C8u, first_seg ? 0x0100u : 0,
            first_seg);
    ram[addr(0x4000u, 0x0139u)] = target_count;
    if (first_seg) {
        ram[addr(first_seg, 0x0139u)] = first_count;
        for (unsigned field = 0x2Eu; field <= 0x3Eu; ++field) {
            if (field == 0x39u) continue;
            uint8_t value = (field == 0x3Cu) ? 1u : (uint8_t)(field * 7u + 3u);
            ram[addr(0x4000u, (uint16_t)(0x0100u + field))] = value;
            ram[addr(first_seg, (uint16_t)(0x0100u + field))] = value;
        }
    }
}

int main(void)
{
    uint8_t *ram = calloc(PR_U77_189D_RAM_SIZE, 1u);
    pr_u77_189d_cpu cpu;
    assert(ram != NULL);
    cpu = start_cpu();
    assert(pr_u77_stack_189d(NULL, PR_U77_189D_RAM_SIZE, &cpu,
                              remove_node, NULL, 10) == PR_U77_189D_ARGUMENT);
    assert(pr_u77_stack_189d(ram, PR_U77_189D_RAM_SIZE - 1u, &cpu,
                              remove_node, NULL, 10) == PR_U77_189D_ARGUMENT);

    prepare(ram, 0, 17, 0);
    calls = 0;
    assert(pr_u77_stack_189d(ram, PR_U77_189D_RAM_SIZE, &cpu,
                              remove_node, NULL, 10) == PR_U77_189D_OK);
    assert(ram[addr(0x4000u, 0x0139u)] == 17u && calls == 0);
    assert(cpu.sp == 0x8008u && cpu.bp == 0x7777u &&
           cpu.cs == 0xF000u && cpu.ip == 0x0712u);

    free(ram);
    ram = calloc(PR_U77_189D_RAM_SIZE, 1u);
    assert(ram != NULL);
    prepare(ram, 0x4100u, 100, 50);
    cpu = start_cpu();
    calls = 0;
    assert(pr_u77_stack_189d(ram, PR_U77_189D_RAM_SIZE, &cpu,
                              remove_node, NULL, 10) == PR_U77_189D_OK);
    assert(ram[addr(0x4000u, 0x0139u)] == 150u && calls == 1);
    assert(cpu.sp == 0x8008u && cpu.bp == 0x7777u);
    free(ram);
    puts("unit 0x77 +189D stack tests passed");
    return 0;
}
