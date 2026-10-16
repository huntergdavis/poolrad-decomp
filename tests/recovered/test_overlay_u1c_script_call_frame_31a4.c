#include "poolrad/recovered/overlay_u1c_script_call_frame_31a4.h"

#include <assert.h>
#include <string.h>

#define RAM_CAP 1048576u
#define CODE_PHYSICAL 0x389c4u
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

static void set_code(void)
{
    ram[CODE_PHYSICAL] = 0x0eu;
    ram[CODE_PHYSICAL + 1u] = 0xe8u;
    ram[CODE_PHYSICAL + 2u] = 0x85u;
    ram[CODE_PHYSICAL + 3u] = 0xfdu;
}

static void write_word(uint8_t *image, size_t address, uint16_t value)
{
    image[address] = (uint8_t)value;
    image[(address + 1u) & 0xfffffu] = (uint8_t)(value >> 8);
}

static void accept(uint16_t ss, uint16_t sp, size_t size)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x31a4u, 0x14a0u, 0x3a16u, ss, sp,
        sp, 0xab45u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    pr_rec_u1c_33bd_cpu expected = cpu;
    const uint16_t cs_sp = (uint16_t)(sp - 2u);
    const uint16_t ip_sp = (uint16_t)(cs_sp - 2u);
    const size_t cs_address = PHYSICAL(ss, cs_sp);
    const size_t ip_address = PHYSICAL(ss, ip_sp);
    set_code();
    memcpy(expected_ram, ram, RAM_CAP);
    write_word(expected_ram, cs_address, 0x3582u);
    write_word(expected_ram, ip_address, 0x31a8u);
    expected.sp = ip_sp;
    expected.ip = 0x2f2du;
    assert(pr_rec_u1c_31a4_call_dispatch_frame(&cpu, ram, size) == 1);
    assert(same_cpu(&cpu, &expected));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

static void reject(pr_rec_u1c_33bd_cpu cpu, uint8_t *image, size_t size)
{
    const pr_rec_u1c_33bd_cpu before = cpu;
    memcpy(expected_ram, ram, RAM_CAP);
    assert(pr_rec_u1c_31a4_call_dispatch_frame(&cpu, image, size) == 0);
    assert(same_cpu(&cpu, &before));
    assert(memcmp(ram, expected_ram, RAM_CAP) == 0);
}

int main(void)
{
    pr_rec_u1c_33bd_cpu cpu = {
        0x3582u, 0x31a4u, 0x14a0u, 0x3a16u, 0x1d16u, 0x3eeeu,
        0x3eeeu, 0xab45u, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0xf046u
    };
    size_t i;
    for (i = 0u; i < RAM_CAP; i++) ram[i] = (uint8_t)(i * 13u + 7u);
    set_code();

    accept(0x1d16u, 0x3eeeu, 655360u); /* supplied first-visit frame */
    assert(ram[0x2104au] == 0xa8u && ram[0x2104bu] == 0x31u);
    assert(ram[0x2104cu] == 0x82u && ram[0x2104du] == 0x35u);
    accept(0x1d16u, 0x0001u, 655360u); /* word crosses offset FFFF */
    accept(0xffffu, 0x0014u, 655360u); /* both pushes A20 wrap */
    accept(0x9fffu, 0x0011u, RAM_CAP); /* first push crosses 640 KiB */
    accept(0x3582u, 0x31a6u, RAM_CAP); /* first push aliases code */

    set_code();
    cpu.ip = 0x31a5u;
    reject(cpu, ram, RAM_CAP);
    cpu.ip = 0x31a4u; cpu.cs = 0x131cu;
    reject(cpu, ram, RAM_CAP);
    cpu.cs = 0x3582u; cpu.flags &= (uint16_t)~0x0040u;
    reject(cpu, ram, RAM_CAP);
    cpu.flags |= 0x0040u; cpu.ss = 0x9fffu; cpu.sp = 0x0011u;
    reject(cpu, ram, 655360u); /* high byte of PUSH CS is outside RAM */
    cpu.ss = 0x1d16u; cpu.sp = 0x3eeeu;
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
    assert(pr_rec_u1c_31a4_call_dispatch_frame(NULL, ram, RAM_CAP) == 0);
    return 0;
}
