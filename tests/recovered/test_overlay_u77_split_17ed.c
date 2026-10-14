#include "poolrad/recovered/overlay_u77_split_17ed.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static size_t addr(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U77_17ED_RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = addr(seg, off);
    return (uint16_t)(ram[a] |
                      ((uint16_t)ram[(a + 1u) & (PR_U77_17ED_RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = addr(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U77_17ED_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static unsigned calls[4];

static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u77_17ed_cpu *cpu, pr_u77_17ed_call call)
{
    (void)context;
    assert(ram_size == PR_U77_17ED_RAM_SIZE);
    ++calls[call];
    if (call == PR_U77_17ED_ALLOC) {
        assert(get16(ram, cpu->ss, cpu->sp) == 0x003Fu);
        cpu->ax = 0x0200u;
        cpu->dx = 0x5000u;
    } else if (call == PR_U77_17ED_COPY) {
        uint16_t length = get16(ram, cpu->ss, cpu->sp);
        uint16_t dst_off = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
        uint16_t dst_seg = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
        uint16_t src_off = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
        uint16_t src_seg = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u));
        assert(length == 0x3Fu && dst_off == 0x0200u && dst_seg == 0x5000u);
        assert(src_off == 0x0100u && src_seg == 0x4000u);
        for (unsigned i = 0; i < length; ++i)
            ram[addr(dst_seg, (uint16_t)(dst_off + i))] =
                ram[addr(src_seg, (uint16_t)(src_off + i))];
    }
    return 1;
}

static pr_u77_17ed_cpu initial_cpu(void)
{
    pr_u77_17ed_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x17EDu, 0x1000u, 0x1111u,
        0xE000u, 0x7202u
    };
    return cpu;
}

static void prepare(uint8_t *ram, uint8_t count)
{
    for (unsigned i = 0; i < 0x3Fu; ++i)
        ram[addr(0x4000u, (uint16_t)(0x0100u + i))] =
            (uint8_t)(i * 19u + 7u);
    ram[addr(0x4000u, 0x0139u)] = count;
    put16(ram, 0x4000u, 0x012Au, 0x0300u);
    put16(ram, 0x4000u, 0x012Cu, 0x6000u);
    put16(ram, 0xE000u, 0x8000u, 0x0712u);
    put16(ram, 0xE000u, 0x8002u, 0xF000u);
    put16(ram, 0xE000u, 0x8004u, 0x0100u);
    put16(ram, 0xE000u, 0x8006u, 0x4000u);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U77_17ED_RAM_SIZE, 1u);
    pr_u77_17ed_cpu cpu = initial_cpu();
    assert(ram != NULL);
    assert(pr_u77_split_17ed(NULL, PR_U77_17ED_RAM_SIZE, &cpu,
                              external, NULL) == PR_U77_17ED_ARGUMENT);
    assert(pr_u77_split_17ed(ram, PR_U77_17ED_RAM_SIZE - 1u, &cpu,
                              external, NULL) == PR_U77_17ED_ARGUMENT);

    prepare(ram, 5u);
    assert(pr_u77_split_17ed(ram, PR_U77_17ED_RAM_SIZE, &cpu,
                              external, NULL) == PR_U77_17ED_OK);
    assert(calls[PR_U77_17ED_ALLOC] == 1u &&
           calls[PR_U77_17ED_COPY] == 1u &&
           calls[PR_U77_17ED_STRING] == 0u &&
           calls[PR_U77_17ED_REPORT] == 0u);
    assert(ram[addr(0x4000u, 0x0139u)] == 3u);
    assert(ram[addr(0x5000u, 0x0239u)] == 2u);
    assert(ram[addr(0x5000u, 0x0234u)] == 0u);
    assert(get16(ram, 0x4000u, 0x012Au) == 0x0200u &&
           get16(ram, 0x4000u, 0x012Cu) == 0x5000u);
    assert(get16(ram, 0x5000u, 0x022Au) == 0x0300u &&
           get16(ram, 0x5000u, 0x022Cu) == 0x6000u);
    assert(cpu.sp == 0x8008u && cpu.bp == 0x7777u &&
           cpu.cs == 0xF000u && cpu.ip == 0x0712u);

    free(ram);
    ram = calloc(PR_U77_17ED_RAM_SIZE, 1u);
    assert(ram != NULL);
    prepare(ram, 1u);
    cpu = initial_cpu();
    for (unsigned i = 0; i < 4u; ++i) calls[i] = 0u;
    assert(pr_u77_split_17ed(ram, PR_U77_17ED_RAM_SIZE, &cpu,
                              external, NULL) == PR_U77_17ED_OK);
    assert(calls[PR_U77_17ED_ALLOC] == 0u &&
           calls[PR_U77_17ED_COPY] == 0u &&
           calls[PR_U77_17ED_STRING] == 1u &&
           calls[PR_U77_17ED_REPORT] == 1u);
    assert(ram[addr(0x4000u, 0x0139u)] == 1u);
    free(ram);
    puts("unit 0x77 +17ED split tests passed");
    return 0;
}
