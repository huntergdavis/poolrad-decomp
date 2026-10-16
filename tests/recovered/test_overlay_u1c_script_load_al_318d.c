#include "poolrad/recovered/overlay_u1c_script_load_al_318d.h"

#include <assert.h>
#include <string.h>

#define RAM_CAP 1048576u
#define CODE_PHYSICAL 0x389adu
#define PHYSICAL(segment, offset) (((((size_t)(segment) << 4) + (offset))) & 0xfffffu)

static uint8_t ram[RAM_CAP];
static uint8_t expected_ram[RAM_CAP];

static int same_cpu(const pr_rec_u1c_33bd_cpu *a,
                    const pr_rec_u1c_33bd_cpu *b)
{
    return a->cs == b->cs && a->ip == b->ip && a->ds == b->ds &&
           a->es == b->es && a->ss == b->ss && a->sp == b->sp &&
           a->bp == b->bp && a->ax == b->ax && a->bx == b->bx &&
           a->cx == b->cx && a->dx == b->dx && a->si == b->si &&
           a->di == b->di && a->flags == b->flags;
}

static void accept(uint16_t ds, uint8_t latch, size_t size)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x318du, ds, 0x3a16u, 0x1d16u, 0x3ef0u,
        0x3ef8u, 0xab48u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    pr_rec_u1c_33bd_cpu expected;
    ram[PHYSICAL(ds, 0x6f7fu)] = latch;
    /* Explicit synthetic ZF-set caller state; no predecessor executes here. */
    expected = cpu;
    expected.ip = 0x3190u;
    expected.ax = (uint16_t)((cpu.ax & 0xff00u) | latch);
    memcpy(expected_ram, ram, RAM_CAP);
    assert(pr_rec_u1c_318d_load_latch_al(&cpu, ram, size) == 1);
    assert(same_cpu(&cpu, &expected));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

static void reject(pr_rec_u1c_33bd_cpu cpu, const uint8_t *image,
                   size_t size)
{
    const pr_rec_u1c_33bd_cpu before = cpu;
    memcpy(expected_ram, ram, RAM_CAP);
    assert(pr_rec_u1c_318d_load_latch_al(&cpu, image, size) == 0);
    assert(same_cpu(&cpu, &before));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

int main(void)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x318du, 0x14a0u, 0x3a16u, 0x1d16u, 0x3ef0u,
        0x3ef8u, 0xab48u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    size_t i;
    for (i = 0u; i < RAM_CAP; i++) ram[i] = (uint8_t)(i * 13u + 7u);
    ram[CODE_PHYSICAL] = 0xa0u;
    ram[CODE_PHYSICAL + 1u] = 0x7fu;
    ram[CODE_PHYSICAL + 2u] = 0x6fu;

    accept(0x14a0u, 0x00u, 655360u);
    accept(0x14a0u, 0x45u, 655360u);
    accept(0x14a0u, 0xffu, RAM_CAP);
    accept(0xffffu, 0x80u, RAM_CAP); /* DS A20 wrap */

    cpu.ip = 0x3190u;
    reject(cpu, ram, RAM_CAP);
    cpu.ip = 0x318du; cpu.cs = 0x131cu;
    reject(cpu, ram, RAM_CAP);
    cpu.cs = 0x3582u; cpu.flags &= (uint16_t)~0x0040u;
    reject(cpu, ram, RAM_CAP);
    cpu.flags |= 0x0040u; cpu.ds = 0xa000u;
    reject(cpu, ram, 655360u);
    cpu.ds = 0x14a0u;
    for (i = 0u; i < 3u; i++) {
        ram[CODE_PHYSICAL + i] ^= 1u;
        reject(cpu, ram, RAM_CAP);
        ram[CODE_PHYSICAL + i] ^= 1u;
    }
    reject(cpu, ram, CODE_PHYSICAL);
    reject(cpu, ram, CODE_PHYSICAL + 1u);
    reject(cpu, ram, CODE_PHYSICAL + 2u);
    reject(cpu, ram, RAM_CAP - 1u);
    reject(cpu, NULL, RAM_CAP);
    assert(pr_rec_u1c_318d_load_latch_al(NULL, ram, RAM_CAP) == 0);
    return 0;
}
