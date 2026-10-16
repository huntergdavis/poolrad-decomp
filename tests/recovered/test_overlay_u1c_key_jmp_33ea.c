#include "poolrad/recovered/overlay_u1c_key_jmp_33ea.h"

#include <assert.h>
#include <string.h>

#define RAM_CAP 1048576u
#define CODE_PHYSICAL 0x38c0au

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

static void accept(uint16_t flags, uint16_t sp, size_t size)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x33eau, 0x14a0u, 0x3a16u, 0x1d16u, sp,
        0x3ef8u, 0xab45u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, flags
    };
    pr_rec_u1c_33bd_cpu expected = cpu;
    expected.ip = 0x342cu;
    memcpy(expected_ram, ram, RAM_CAP);
    assert(pr_rec_u1c_33ea_jmp_342c(&cpu, ram, size) == 1);
    assert(same_cpu(&cpu, &expected));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

static void reject(pr_rec_u1c_33bd_cpu cpu, const uint8_t *image,
                   size_t size)
{
    const pr_rec_u1c_33bd_cpu before = cpu;
    memcpy(expected_ram, ram, RAM_CAP);
    assert(pr_rec_u1c_33ea_jmp_342c(&cpu, image, size) == 0);
    assert(same_cpu(&cpu, &before));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

int main(void)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x33eau, 0x14a0u, 0x3a16u, 0x1d16u, 0x3ef0u,
        0x3ef8u, 0xab45u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    size_t i;
    for (i = 0u; i < RAM_CAP; i++) ram[i] = (uint8_t)(i * 13u + 7u);
    ram[CODE_PHYSICAL] = 0xebu;
    ram[CODE_PHYSICAL + 1u] = 0x40u;

    accept(0xf046u, 0x3ef0u, 655360u);
    accept(0xf002u, 0x3ef0u, 655360u);
    accept(0xffffu, 0xffffu, RAM_CAP);
    accept(0x0002u, 0x0000u, RAM_CAP);

    cpu.ip = 0x342cu;
    reject(cpu, ram, RAM_CAP);
    cpu.ip = 0x33eau; cpu.cs = 0x131cu;
    reject(cpu, ram, RAM_CAP);
    cpu.cs = 0x3582u;
    for (i = 0u; i < 2u; i++) {
        ram[CODE_PHYSICAL + i] ^= 1u;
        reject(cpu, ram, RAM_CAP);
        ram[CODE_PHYSICAL + i] ^= 1u;
    }
    reject(cpu, ram, CODE_PHYSICAL);
    reject(cpu, ram, CODE_PHYSICAL + 1u);
    reject(cpu, ram, 655360u - 1u);
    reject(cpu, ram, RAM_CAP - 1u);
    reject(cpu, NULL, RAM_CAP);
    assert(pr_rec_u1c_33ea_jmp_342c(NULL, ram, RAM_CAP) == 0);
    return 0;
}
