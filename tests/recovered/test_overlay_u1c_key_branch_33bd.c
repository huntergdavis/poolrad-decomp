#include "poolrad/recovered/overlay_u1c_key_branch_33bd.h"

#include <assert.h>
#include <string.h>

#define RAM_SIZE 655360u
#define ADDRESS(segment, offset) (((size_t)(segment) << 4) + (offset))

static uint8_t ram[RAM_SIZE];
static uint8_t before[RAM_SIZE];

static int same_cpu(const pr_rec_u1c_33bd_cpu *a,
                    const pr_rec_u1c_33bd_cpu *b)
{
    return a->cs == b->cs && a->ip == b->ip && a->ds == b->ds &&
           a->es == b->es && a->ss == b->ss && a->sp == b->sp &&
           a->bp == b->bp && a->ax == b->ax && a->bx == b->bx &&
           a->cx == b->cx && a->dx == b->dx && a->si == b->si &&
           a->di == b->di && a->flags == b->flags;
}

static void run_case(pr_rec_u1c_33bd_cpu input, uint16_t value,
                     uint16_t expected_ip, uint16_t expected_flags)
{
    pr_rec_u1c_33bd_cpu expected = input;
    const size_t address = ADDRESS(input.es, input.di + 0x0594u);
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
    memcpy(before, ram, RAM_SIZE);
    expected.ip = expected_ip;
    expected.flags = expected_flags;
    assert(pr_rec_u1c_33bd_branch(&input, ram, RAM_SIZE) == 1);
    assert(same_cpu(&input, &expected));
    assert(memcmp(ram, before, RAM_SIZE) == 0);
}

static void reject_unchanged(pr_rec_u1c_33bd_cpu input,
                             const uint8_t *supplied_ram, size_t size)
{
    const pr_rec_u1c_33bd_cpu original = input;
    memcpy(before, ram, RAM_SIZE);
    assert(pr_rec_u1c_33bd_branch(&input, supplied_ram, size) == 0);
    assert(same_cpu(&input, &original));
    assert(memcmp(ram, before, RAM_SIZE) == 0);
}

int main(void)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x33bdu, 0x14a0u, 0x3a16u, 0x1d16u, 0x1111u,
        0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u, 0x7777u,
        0x0000u, 0x7002u
    };
    size_t i;

    for (i = 0u; i < RAM_SIZE; i++) ram[i] = (uint8_t)(i * 31u + 9u);
    run_case(cpu, 0u, 0x33c5u, 0x7097u); /* Saved word, not full native CPU. */
    run_case(cpu, 1u, 0x33c5u, 0x7046u);
    run_case(cpu, 2u, 0x33d2u, 0x7002u); /* Alternate source-only arm. */
    run_case(cpu, 0x8000u, 0x33d2u, 0x7816u);

    cpu.ip = 0x33c3u;
    reject_unchanged(cpu, ram, RAM_SIZE);
    cpu.ip = 0x33bdu;
    cpu.cs = 0x3583u;
    reject_unchanged(cpu, ram, RAM_SIZE);
    cpu.cs = 0x3582u;
    cpu.di = 0xfa6bu; /* Operand WORD would cross 16-bit offset wrap. */
    reject_unchanged(cpu, ram, RAM_SIZE);
    cpu.di = 0u;
    cpu.es = 0xffffu;
    reject_unchanged(cpu, ram, RAM_SIZE);
    cpu.es = 0x3a16u;
    reject_unchanged(cpu, ram, RAM_SIZE - 1u);
    reject_unchanged(cpu, NULL, RAM_SIZE);
    assert(pr_rec_u1c_33bd_branch(NULL, ram, RAM_SIZE) == 0);
    return 0;
}
