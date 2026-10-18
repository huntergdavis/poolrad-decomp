#include "poolrad/recovered/overlay_u9e_entry16_10e4.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U9E_10E4_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U9E_10E4_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(uint16_t sp, uint8_t first, uint8_t second,
                uint16_t expected_ax, uint16_t expected_flags)
{
    uint8_t *ram = calloc(PR_U9E_10E4_RAM_SIZE, 1u);
    pr_u9e_10e4_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ax = 0xBEEFu;
    cpu.bp = 0x2468u;
    cpu.sp = sp;
    cpu.cs = 0xF000u;
    cpu.ip = 0x10E4u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(sp + 2u), 0x3456u);
    put16(ram, cpu.ss, (uint16_t)(sp + 4u), (uint16_t)(0xA500u | first));
    put16(ram, cpu.ss, (uint16_t)(sp + 6u), (uint16_t)(0xB600u | second));
    assert(pr_u9e_entry16_10e4(ram, PR_U9E_10E4_RAM_SIZE, &cpu) ==
           PR_U9E_10E4_OK);
    assert(cpu.ax == expected_ax && cpu.flags == expected_flags);
    assert(cpu.bp == 0x2468u && cpu.sp == (uint16_t)(sp + 8u));
    assert(cpu.cs == 0x3456u && cpu.ip == 0x1234u);
    assert(ram[at(cpu.ss, (uint16_t)(sp - 3u))] == (uint8_t)expected_ax);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U9E_10E4_RAM_SIZE, 1u);
    pr_u9e_10e4_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x10E4u;
    assert(pr_u9e_entry16_10e4(NULL, PR_U9E_10E4_RAM_SIZE, &cpu) ==
           PR_U9E_10E4_ARGUMENT);
    assert(pr_u9e_entry16_10e4(ram, PR_U9E_10E4_RAM_SIZE - 1u, &cpu) ==
           PR_U9E_10E4_ARGUMENT);
    cpu.ip = 0;
    assert(pr_u9e_entry16_10e4(ram, PR_U9E_10E4_RAM_SIZE, &cpu) ==
           PR_U9E_10E4_ARGUMENT);
    free(ram);
    run(0x8000u, 0x33u, 0x11u, 0x0075u, 0x0297u);
    run(0xFFFCu, 0xFFu, 0x12u, 0x0100u, 0x0216u);
    return 0;
}
