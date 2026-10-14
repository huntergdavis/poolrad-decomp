#include "poolrad/recovered/overlay_u1c_clear_box_2b14.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned calls;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U1C_2B14_RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] |
                      ((uint16_t)ram[(a + 1u) & (PR_U1C_2B14_RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U1C_2B14_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static int display(void *context, uint8_t *ram, size_t size,
                   pr_u1c_2b14_cpu *cpu)
{
    (void)context;
    assert(size == PR_U1C_2B14_RAM_SIZE);
    assert(cpu->ip == 0x2B27u);
    assert(cpu->sp == 0x7FF6u);
    assert(get16(ram, cpu->ss, cpu->sp) == 0xA50Fu);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xA50Fu);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 0xA501u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 0xA501u);
    ++calls;
    return 1;
}

int main(void)
{
    uint8_t *ram = calloc(PR_U1C_2B14_RAM_SIZE, 1u);
    pr_u1c_2b14_cpu cpu = {
        0xA5CDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x2B14u, 0x3000u, 0x1111u,
        0xE000u, 0x7203u
    };
    assert(ram != NULL);
    assert(pr_u1c_clear_box_2b14(NULL, PR_U1C_2B14_RAM_SIZE,
                                   &cpu, display, NULL) == PR_U1C_2B14_ARGUMENT);
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ds, 0x49EDu, 0x7FFFu);
    ram[physical(cpu.ds, 0x84DEu)] = 0xA5u;
    assert(pr_u1c_clear_box_2b14(ram, PR_U1C_2B14_RAM_SIZE,
                                   &cpu, display, NULL) == PR_U1C_2B14_OK);
    assert(calls == 1u);
    assert(get16(ram, cpu.ds, 0x49EDu) == 0x8000u);
    assert(ram[physical(cpu.ds, 0x84DEu)] == 1u);
    assert(cpu.flags & 0x0800u); /* INC 7FFF -> 8000 sets OF. */
    assert(cpu.flags & 0x0001u); /* INC preserves CF. */
    assert(cpu.sp == 0x8004u && cpu.bp == 0x7777u &&
           cpu.cs == 0xD000u && cpu.ip == 0x0712u);
    free(ram);
    puts("unit 0x1C +2B14 clear-box tests passed");
    return 0;
}
