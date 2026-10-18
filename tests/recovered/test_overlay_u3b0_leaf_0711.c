#include "poolrad/recovered/overlay_u3b0_leaf_0711.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U3B0_0711_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U3B0_0711_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(uint16_t sp, uint16_t bound, uint8_t first_flag,
                uint8_t second_flag, uint16_t want_off, uint16_t want_seg)
{
    uint8_t *ram = calloc(PR_U3B0_0711_RAM_SIZE, 1u);
    pr_u3b0_0711_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ax = 0xABCDu;
    cpu.bp = 0x2468u;
    cpu.sp = sp;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0711u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(sp + 2u), 0xD000u);
    put16(ram, cpu.ss, (uint16_t)(sp + 4u), 0xA5A5u);
    put16(ram, cpu.ss, (uint16_t)(sp + 6u), 0x2200u);
    put16(ram, cpu.ss, (uint16_t)(sp + 8u), 0x5000u);
    put16(ram, cpu.ss, (uint16_t)(sp + 10u), bound);
    ram[at(0x5000u, 0x2229u)] = first_flag;
    ram[at(0x5000u, 0x2329u)] = second_flag;
    put16(ram, 0x5000u, 0x222Au, 0x2300u);
    put16(ram, 0x5000u, 0x222Cu, 0x5000u);
    assert(pr_u3b0_leaf_0711(ram, PR_U3B0_0711_RAM_SIZE, &cpu) ==
           PR_U3B0_0711_OK);
    assert(cpu.ax == want_off && cpu.dx == want_seg);
    assert(cpu.bp == 0x2468u && cpu.sp == (uint16_t)(sp + 12u));
    assert(cpu.ip == 0x1234u && cpu.cs == 0xD000u);
    free(ram);
}

static void null_head_fallback(void)
{
    uint8_t *ram = calloc(PR_U3B0_0711_RAM_SIZE, 1u);
    pr_u3b0_0711_cpu cpu = {0};
    assert(ram != NULL);
    cpu.sp = 0x8000u;
    cpu.ss = 0x7000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0711u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 10u), 3u);
    ram[0x29u] = 0x80u; /* Null head is still dereferenced by fallback. */
    assert(pr_u3b0_leaf_0711(ram, PR_U3B0_0711_RAM_SIZE, &cpu) ==
           PR_U3B0_0711_OK);
    assert(cpu.ax == 0u && cpu.dx == 0u && cpu.es == 0u && cpu.di == 0u);
    assert(cpu.flags == 0x0282u); /* CMP 80,0, not a fabricated null return. */
    assert(cpu.ip == 0x1234u && cpu.cs == 0xD000u && cpu.sp == 0x800Cu);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0711_RAM_SIZE, 1u);
    pr_u3b0_0711_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0711u;
    assert(pr_u3b0_leaf_0711(NULL, PR_U3B0_0711_RAM_SIZE, &cpu) ==
           PR_U3B0_0711_ARGUMENT);
    assert(pr_u3b0_leaf_0711(ram, PR_U3B0_0711_RAM_SIZE - 1u, &cpu) ==
           PR_U3B0_0711_ARGUMENT);
    free(ram);
    run(0x8000u, 2u, 0u, 1u, 0x2300u, 0x5000u);
    run(0x8000u, 0u, 0u, 1u, 0u, 0u);
    run(0x8000u, 0u, 1u, 0u, 0x2200u, 0x5000u);
    run(0xFFFEu, 1u, 1u, 0u, 0x2200u, 0x5000u);
    run(0xFFFFu, 0xFFFFu, 1u, 0u, 0x2200u, 0x5000u);
    null_head_fallback();
    return 0;
}
