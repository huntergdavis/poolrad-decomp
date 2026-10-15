#include "poolrad/recovered/overlay_u3b0_leaf_0067.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U3B0_0067_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U3B0_0067_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(uint16_t sp, unsigned count, uint16_t first_off)
{
    uint8_t *ram = calloc(PR_U3B0_0067_RAM_SIZE, 1u);
    pr_u3b0_0067_cpu cpu = {0};
    unsigned n;
    assert(ram != NULL && count <= 3u);
    cpu.ax = 0xABCDu;
    cpu.dx = 0x3579u;
    cpu.di = 0x579Bu;
    cpu.es = 0x8000u;
    cpu.bp = 0x2468u;
    cpu.sp = sp;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0067u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(sp + 2u), 0xD000u);
    put16(ram, cpu.ss, (uint16_t)(sp + 4u),
          count == 0u ? 0u : first_off);
    put16(ram, cpu.ss, (uint16_t)(sp + 6u),
          count == 0u ? 0u : 0x5000u);
    for (n = 0; n < count; ++n) {
        uint16_t off = (uint16_t)(first_off + 0x100u * n);
        put16(ram, 0x5000u, (uint16_t)(off + 0x2Au),
              n + 1u < count ? (uint16_t)(off + 0x100u) : 0u);
        put16(ram, 0x5000u, (uint16_t)(off + 0x2Cu),
              n + 1u < count ? 0x5000u : 0u);
    }
    assert(pr_u3b0_leaf_0067(ram, PR_U3B0_0067_RAM_SIZE, &cpu) ==
           PR_U3B0_0067_OK);
    assert(cpu.ax == count && cpu.dx == 0u);
    assert(cpu.bp == 0x2468u && cpu.sp == (uint16_t)(sp + 8u));
    assert(cpu.ip == 0x1234u && cpu.cs == 0xD000u);
    assert(cpu.di == (count == 0u ? 0x579Bu :
                      (uint16_t)(first_off + 0x100u * (count - 1u))));
    assert(cpu.es == (count == 0u ? 0x8000u : 0x5000u));
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0067_RAM_SIZE, 1u);
    pr_u3b0_0067_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0067u;
    assert(pr_u3b0_leaf_0067(NULL, PR_U3B0_0067_RAM_SIZE, &cpu) ==
           PR_U3B0_0067_ARGUMENT);
    assert(pr_u3b0_leaf_0067(ram, PR_U3B0_0067_RAM_SIZE - 1u, &cpu) ==
           PR_U3B0_0067_ARGUMENT);
    cpu.ip = 0x0068u;
    assert(pr_u3b0_leaf_0067(ram, PR_U3B0_0067_RAM_SIZE, &cpu) ==
           PR_U3B0_0067_ARGUMENT);
    free(ram);
    run(0x8000u, 0u, 0x4000u);
    run(0x8000u, 1u, 0x4000u);
    run(0x8000u, 3u, 0x4000u);
    run(0xFFFEu, 1u, 0x4000u);
    run(0xFFFFu, 3u, 0x4000u);
    run(0xFFFBu, 1u, 0xFFD8u);
    return 0;
}
