#include "poolrad/recovered/overlay_u1c_sprite_off_2aa2.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned calls;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U1C_2AA2_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U1C_2AA2_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static int view(void *context, uint8_t *ram, size_t size,
                pr_u1c_2aa2_cpu *cpu)
{
    (void)context;
    (void)ram;
    assert(size == PR_U1C_2AA2_RAM_SIZE);
    assert(cpu->ip == 0x2AB0u);
    ++calls;
    return 1;
}

int main(void)
{
    uint8_t *ram = calloc(PR_U1C_2AA2_RAM_SIZE, 1u);
    pr_u1c_2aa2_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x2AA2u, 0x3000u, 0x1111u,
        0xE000u, 0x7202u
    };
    assert(ram != NULL);
    assert(pr_u1c_sprite_off_2aa2(NULL, PR_U1C_2AA2_RAM_SIZE,
                                    &cpu, view, NULL) == PR_U1C_2AA2_ARGUMENT);
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ds, 0x49EDu, 0xFFFFu);
    ram[physical(cpu.ds, 0x84E3u)] = 1u;
    ram[physical(cpu.ds, 0x84E0u)] = 0xA5u;
    assert(pr_u1c_sprite_off_2aa2(ram, PR_U1C_2AA2_RAM_SIZE,
                                    &cpu, view, NULL) == PR_U1C_2AA2_OK);
    assert(calls == 1u);
    assert(ram[physical(cpu.ds, 0x49EDu)] == 0u &&
           ram[physical(cpu.ds, 0x49EEu)] == 0u);
    assert(ram[physical(cpu.ds, 0x84E3u)] == 0u &&
           ram[physical(cpu.ds, 0x84E0u)] == 0u);
    assert(cpu.sp == 0x8004u && cpu.bp == 0x7777u &&
           cpu.cs == 0xD000u && cpu.ip == 0x0712u);
    free(ram);
    puts("unit 0x1C +2AA2 sprite-off tests passed");
    return 0;
}
