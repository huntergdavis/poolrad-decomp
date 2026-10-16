#include "poolrad/recovered/overlay_u1c_script_les_di_3196.h"
#include "poolrad/recovered/overlay_u1c_script_load_al_318d.h"
#include "poolrad/recovered/overlay_u1c_script_store_al_3190.h"
#include "poolrad/recovered/overlay_u1c_script_load_ax_3193.h"

#include <assert.h>
#include <string.h>

#define RAM_CAP 1048576u
#define CODE_PHYSICAL 0x389b6u
#define LOAD_PHYSICAL 0x389adu
#define STORE_PHYSICAL 0x389b0u
#define WORD_PHYSICAL 0x389b3u
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

static void set_pointer(uint16_t ds, uint16_t offset, uint16_t segment)
{
    const size_t base = PHYSICAL(ds, 0x49deu);
    ram[base] = (uint8_t)offset;
    ram[(base + 1u) & 0xfffffu] = (uint8_t)(offset >> 8);
    ram[(base + 2u) & 0xfffffu] = (uint8_t)segment;
    ram[(base + 3u) & 0xfffffu] = (uint8_t)(segment >> 8);
}

static void accept(uint16_t ds, uint16_t offset, uint16_t segment,
                   size_t size)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x3196u, ds, 0x3a16u, 0x1d16u, 0x3ef0u,
        0x3ef8u, 0xab45u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    pr_rec_u1c_33bd_cpu expected = cpu;
    set_pointer(ds, offset, segment);
    memcpy(expected_ram, ram, RAM_CAP);
    expected.di = offset;
    expected.es = segment;
    expected.ip = 0x319au;
    assert(pr_rec_u1c_3196_les_script_pointer(&cpu, ram, size) == 1);
    assert(same_cpu(&cpu, &expected));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

static void accept_after_prefix(void)
{
    const uint16_t ds = 0x14a0u;
    const size_t latch_source = PHYSICAL(ds, 0x6f7fu);
    const size_t latch_destination = PHYSICAL(ds, 0x6f7eu);
    const size_t pc = PHYSICAL(ds, 0x49edu);
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
    ram[WORD_PHYSICAL] = 0xa1u;
    ram[WORD_PHYSICAL + 1u] = 0xedu;
    ram[WORD_PHYSICAL + 2u] = 0x49u;
    ram[latch_source] = 0x5au;
    ram[latch_destination] = 0x09u;
    ram[pc] = 0x89u;
    ram[pc + 1u] = 0x67u;
    set_pointer(ds, 0x3456u, 0x789au);
    memcpy(expected_ram, ram, RAM_CAP);
    expected_ram[latch_destination] = 0x5au;
    expected.ax = 0x6789u;
    expected.di = 0x3456u;
    expected.es = 0x789au;
    expected.ip = 0x319au;
    assert(pr_rec_u1c_318d_load_latch_al(&cpu, ram, RAM_CAP) == 1);
    assert(pr_rec_u1c_3190_store_latch_al(&cpu, ram, RAM_CAP) == 1);
    assert(pr_rec_u1c_3193_load_script_ax(&cpu, ram, RAM_CAP) == 1);
    assert(pr_rec_u1c_3196_les_script_pointer(&cpu, ram, RAM_CAP) == 1);
    assert(same_cpu(&cpu, &expected));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

static void reject(pr_rec_u1c_33bd_cpu cpu, const uint8_t *image,
                   size_t size)
{
    const pr_rec_u1c_33bd_cpu before = cpu;
    memcpy(expected_ram, ram, RAM_CAP);
    assert(pr_rec_u1c_3196_les_script_pointer(&cpu, image, size) == 0);
    assert(same_cpu(&cpu, &before));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

int main(void)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x3196u, 0x14a0u, 0x3a16u, 0x1d16u, 0x3ef0u,
        0x3ef8u, 0xab45u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    size_t i;
    for (i = 0u; i < RAM_CAP; i++) ram[i] = (uint8_t)(i * 13u + 7u);
    ram[CODE_PHYSICAL] = 0xc4u;
    ram[CODE_PHYSICAL + 1u] = 0x3eu;
    ram[CODE_PHYSICAL + 2u] = 0xdeu;
    ram[CODE_PHYSICAL + 3u] = 0x49u;

    accept(0x14a0u, 0x0000u, 0x0000u, 655360u);
    accept(0x14a0u, 0x1234u, 0x5678u, 655360u);
    accept(0x14a0u, 0xffffu, 0xabcdu, RAM_CAP);
    accept(0xffffu, 0x8001u, 0x7f80u, 655360u); /* DS A20 wrap */
    accept(0x9b62u, 0x1234u, 0x5678u, RAM_CAP); /* 640 KiB edge */
    accept_after_prefix();

    cpu.ip = 0x319au;
    reject(cpu, ram, RAM_CAP);
    cpu.ip = 0x3196u; cpu.cs = 0x131cu;
    reject(cpu, ram, RAM_CAP);
    cpu.cs = 0x3582u; cpu.flags &= (uint16_t)~0x0040u;
    reject(cpu, ram, RAM_CAP);
    cpu.flags |= 0x0040u; cpu.ds = 0x9b62u;
    reject(cpu, ram, 655360u); /* last two operand bytes are outside RAM */
    cpu.ds = 0x14a0u;
    for (i = 0u; i < 4u; i++) {
        ram[CODE_PHYSICAL + i] ^= 1u;
        reject(cpu, ram, RAM_CAP);
        ram[CODE_PHYSICAL + i] ^= 1u;
    }
    reject(cpu, ram, CODE_PHYSICAL);
    reject(cpu, ram, CODE_PHYSICAL + 1u);
    reject(cpu, ram, CODE_PHYSICAL + 2u);
    reject(cpu, ram, CODE_PHYSICAL + 3u);
    reject(cpu, ram, RAM_CAP - 1u);
    reject(cpu, NULL, RAM_CAP);
    assert(pr_rec_u1c_3196_les_script_pointer(NULL, ram, RAM_CAP) == 0);
    return 0;
}
