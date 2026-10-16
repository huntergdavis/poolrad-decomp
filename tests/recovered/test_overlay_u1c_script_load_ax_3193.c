#include "poolrad/recovered/overlay_u1c_script_load_ax_3193.h"
#include "poolrad/recovered/overlay_u1c_script_load_al_318d.h"
#include "poolrad/recovered/overlay_u1c_script_store_al_3190.h"

#include <assert.h>
#include <string.h>

#define RAM_CAP 1048576u
#define CODE_PHYSICAL 0x389b3u
#define LOAD_PHYSICAL 0x389adu
#define STORE_PHYSICAL 0x389b0u
#define PHYSICAL(segment, offset) ((((size_t)(segment) << 4) + (offset)) & 0xfffffu)

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

static void accept(uint16_t ds, uint16_t word, size_t size)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x3193u, ds, 0x3a16u, 0x1d16u, 0x3ef0u,
        0x3ef8u, 0xab45u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    pr_rec_u1c_33bd_cpu expected = cpu;
    const size_t low = PHYSICAL(ds, 0x49edu);
    const size_t high = PHYSICAL(ds, 0x49eeu);
    ram[low] = (uint8_t)word;
    ram[high] = (uint8_t)(word >> 8);
    memcpy(expected_ram, ram, RAM_CAP);
    expected.ax = word;
    expected.ip = 0x3196u;
    assert(pr_rec_u1c_3193_load_script_ax(&cpu, ram, size) == 1);
    assert(same_cpu(&cpu, &expected));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

static void accept_after_prefix(void)
{
    const uint16_t ds = 0x14a0u;
    const size_t latch_source = PHYSICAL(ds, 0x6f7fu);
    const size_t latch_destination = PHYSICAL(ds, 0x6f7eu);
    const size_t low = PHYSICAL(ds, 0x49edu);
    const size_t high = PHYSICAL(ds, 0x49eeu);
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x318du, ds, 0x3a16u, 0x1d16u, 0x3ef0u,
        0x3ef8u, 0xab00u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    pr_rec_u1c_33bd_cpu expected = cpu;
    ram[LOAD_PHYSICAL] = 0xa0u;
    ram[LOAD_PHYSICAL + 1u] = 0x7fu;
    ram[LOAD_PHYSICAL + 2u] = 0x6fu;
    ram[STORE_PHYSICAL] = 0xa2u;
    ram[STORE_PHYSICAL + 1u] = 0x7eu;
    ram[STORE_PHYSICAL + 2u] = 0x6fu;
    ram[latch_source] = 0x5au;
    ram[latch_destination] = 0x09u;
    ram[low] = 0xefu;
    ram[high] = 0xbeu;
    memcpy(expected_ram, ram, RAM_CAP);
    expected_ram[latch_destination] = 0x5au;
    expected.ax = 0xbeefu;
    expected.ip = 0x3196u;
    assert(pr_rec_u1c_318d_load_latch_al(&cpu, ram, RAM_CAP) == 1);
    assert(pr_rec_u1c_3190_store_latch_al(&cpu, ram, RAM_CAP) == 1);
    assert(pr_rec_u1c_3193_load_script_ax(&cpu, ram, RAM_CAP) == 1);
    assert(same_cpu(&cpu, &expected));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

static void reject(pr_rec_u1c_33bd_cpu cpu, const uint8_t *image,
                   size_t size)
{
    const pr_rec_u1c_33bd_cpu before = cpu;
    memcpy(expected_ram, ram, RAM_CAP);
    assert(pr_rec_u1c_3193_load_script_ax(&cpu, image, size) == 0);
    assert(same_cpu(&cpu, &before));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

int main(void)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x3193u, 0x14a0u, 0x3a16u, 0x1d16u, 0x3ef0u,
        0x3ef8u, 0xab45u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    size_t i;
    for (i = 0u; i < RAM_CAP; i++) ram[i] = (uint8_t)(i * 13u + 7u);
    ram[CODE_PHYSICAL] = 0xa1u;
    ram[CODE_PHYSICAL + 1u] = 0xedu;
    ram[CODE_PHYSICAL + 2u] = 0x49u;

    accept(0x14a0u, 0x0000u, 655360u);
    accept(0x14a0u, 0x1234u, 655360u);
    accept(0x14a0u, 0xffffu, RAM_CAP);
    accept(0xffffu, 0x8001u, 655360u); /* DS A20 wrap */
    accept(0xffffu, 0x7f80u, RAM_CAP);
    accept_after_prefix();

    cpu.ip = 0x3196u;
    reject(cpu, ram, RAM_CAP);
    cpu.ip = 0x3193u; cpu.cs = 0x131cu;
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
    assert(pr_rec_u1c_3193_load_script_ax(NULL, ram, RAM_CAP) == 0);
    return 0;
}
