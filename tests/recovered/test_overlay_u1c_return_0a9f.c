#include "poolrad/recovered/overlay_u1c_return_0a9f.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned calls;

static size_t addr(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U1C_0A9F_RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = addr(seg, off);
    return (uint16_t)(ram[a] |
                      ((uint16_t)ram[(a + 1u) & (PR_U1C_0A9F_RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = addr(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U1C_0A9F_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static int free_node(void *context, uint8_t *ram, size_t size,
                     pr_u1c_0a9f_cpu *cpu)
{
    (void)context;
    assert(size == PR_U1C_0A9F_RAM_SIZE && cpu->ip == 0x0AD8u);
    assert(get16(ram, cpu->ss, cpu->sp) == 6u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0x0100u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 0x4000u);
    assert(get16(ram, cpu->ds, 0x6F74u) == 0x0300u);
    assert(get16(ram, cpu->ds, 0x6F76u) == 0x6000u);
    put16(ram, cpu->ds, 0x6F74u, 0x3344u);
    ++calls;
    return 1;
}

static pr_u1c_0a9f_cpu make_cpu(void)
{
    pr_u1c_0a9f_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x0A9Fu, 0x3000u, 0x1111u,
        0xE000u, 0x72D3u
    };
    return cpu;
}

static void stack(uint8_t *ram, const pr_u1c_0a9f_cpu *cpu)
{
    put16(ram, cpu->ss, cpu->sp, 0x0712u);
    put16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u), 0xD000u);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U1C_0A9F_RAM_SIZE, 1u);
    pr_u1c_0a9f_cpu cpu = make_cpu();
    assert(ram != NULL);
    assert(pr_u1c_return_0a9f(NULL, PR_U1C_0A9F_RAM_SIZE,
                                &cpu, free_node, NULL) == PR_U1C_0A9F_ARGUMENT);
    stack(ram, &cpu);
    put16(ram, cpu.ds, 0x49EDu, 0xFFFFu);
    assert(pr_u1c_return_0a9f(ram, PR_U1C_0A9F_RAM_SIZE,
                                &cpu, NULL, NULL) == PR_U1C_0A9F_OK);
    assert(calls == 0u && get16(ram, cpu.ds, 0x49EDu) == 0u);
    assert(cpu.sp == 0x8004u && cpu.bp == 0x7777u &&
           cpu.cs == 0xD000u && cpu.ip == 0x0712u);
    free(ram);

    ram = calloc(PR_U1C_0A9F_RAM_SIZE, 1u);
    assert(ram != NULL);
    cpu = make_cpu();
    stack(ram, &cpu);
    put16(ram, cpu.ds, 0x6F70u, 0x0100u);
    put16(ram, cpu.ds, 0x6F72u, 0x4000u);
    put16(ram, 0x4000u, 0x0100u, 0x9137u);
    put16(ram, 0x4000u, 0x0102u, 0x0300u);
    put16(ram, 0x4000u, 0x0104u, 0x6000u);
    assert(pr_u1c_return_0a9f(ram, PR_U1C_0A9F_RAM_SIZE,
                                &cpu, free_node, NULL) == PR_U1C_0A9F_OK);
    assert(calls == 1u && get16(ram, cpu.ds, 0x49EDu) == 0x9137u);
    assert(get16(ram, cpu.ds, 0x6F70u) == 0x3344u);
    assert(get16(ram, cpu.ds, 0x6F72u) == 0x6000u);
    assert(cpu.sp == 0x8004u && cpu.bp == 0x7777u &&
           cpu.cs == 0xD000u && cpu.ip == 0x0712u);
    free(ram);
    puts("unit 0x1C +0A9F RETURN tests passed");
    return 0;
}
