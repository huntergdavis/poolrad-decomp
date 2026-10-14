#include "poolrad/recovered/resident_464_reverse_1484.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t address(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & 0xfffffu;
}

static uint8_t oracle(unsigned value)
{
    unsigned i, result = 0;
    for (i = 0; i < 8; ++i) result |= ((value >> i) & 1u) << (7u - i);
    return (uint8_t)result;
}

static void frame_case(uint16_t ss, uint16_t sp, unsigned argument)
{
    uint8_t *ram = malloc(0x100000u), *expected = malloc(0x100000u);
    pr_slot_a_directory_state s;
    pr_slot_a_directory_cpu cpu;
    unsigned i, ones = 0;
    uint8_t value = oracle(argument);
    assert(ram && expected);
    memset(ram, 0xa5, 0x100000u);
    memset(&s, 0, sizeof s);
    s.ram = ram; s.ram_size = 0x100000u;
    s.cpu.ax = 0x9876; s.cpu.bx = 0x2345; s.cpu.cx = 0x3456;
    s.cpu.dx = 0x4567; s.cpu.si = 0x5678; s.cpu.di = 0x6789;
    s.cpu.bp = 0x789a; s.cpu.sp = sp; s.cpu.cs = 0x0c88;
    s.cpu.ds = 0x1d16; s.cpu.es = 0x4529; s.cpu.ss = ss;
    s.cpu.fs = 0x1357; s.cpu.gs = 0x2468;
    s.cpu.ip = 0x1484; s.cpu.flags = 0x0fd7;
    /* Independent return target and argument, including nonzero high byte. */
    ram[address(ss, sp)] = 0xae;
    ram[address(ss, (uint16_t)(sp + 1u))] = 0x17;
    ram[address(ss, (uint16_t)(sp + 2u))] = 0x88;
    ram[address(ss, (uint16_t)(sp + 3u))] = 0x0c;
    ram[address(ss, (uint16_t)(sp + 4u))] = (uint8_t)argument;
    ram[address(ss, (uint16_t)(sp + 5u))] = 0xd3;
    memcpy(expected, ram, 0x100000u);
    cpu = s.cpu;
    expected[address(ss, (uint16_t)(sp - 2u))] = 0x9a;
    expected[address(ss, (uint16_t)(sp - 1u))] = 0x78;
    expected[address(ss, (uint16_t)(sp - 4u))] = value;
    expected[address(ss, (uint16_t)(sp - 3u))] = value;
    cpu.ax = value; cpu.cx = 7; cpu.sp = (uint16_t)(sp + 6u);
    cpu.ip = 0x17ae; cpu.cs = 0x0c88;
    cpu.flags &= (uint16_t)~0x08d5u;
    for (i = 0; i < 8; ++i) ones += (value >> i) & 1u;
    if (!(ones & 1u)) cpu.flags |= 4;
    if (!value) cpu.flags |= 0x40;
    if (value >= 128) cpu.flags |= 0x80;
    assert(pr_r464_reverse_1484_frame(&s));
    assert(!memcmp(&s.cpu, &cpu, sizeof cpu));
    assert(!memcmp(ram, expected, 0x100000u));
    free(expected); free(ram);
}

int main(void)
{
    unsigned i;
    uint8_t ram[16] = {0}, original[16];
    pr_slot_a_directory_state s = {0};
    pr_slot_a_directory_cpu cpu;
    for (i = 0; i <= 0xffffu; ++i)
        assert(pr_r464_reverse_1484((uint16_t)i) == oracle(i));
    for (i = 0; i < 256; ++i) frame_case(0x1d16, 0x37a0, i);
    frame_case(0, 4, 0x96);
    frame_case(0, 0xfffa, 0x81);
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.ip = 0x1484; s.cpu.sp = 14; s.cpu.bp = 0x1234;
    cpu = s.cpu; memcpy(original, ram, sizeof ram);
    assert(!pr_r464_reverse_1484_frame(&s));
    assert(!memcmp(&s.cpu, &cpu, sizeof cpu));
    assert(!memcmp(ram, original, sizeof ram));
    s.cpu.sp = 1;
    assert(!pr_r464_reverse_1484_frame(&s));
    s.cpu.sp = 0xfffd;
    assert(!pr_r464_reverse_1484_frame(&s));
    s.cpu.ip = 0;
    assert(!pr_r464_reverse_1484_frame(&s));
    assert(!pr_r464_reverse_1484_frame(NULL));
    return 0;
}
