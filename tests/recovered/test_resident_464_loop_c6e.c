#include "poolrad/recovered/resident_464_loop_c6e.h"

#include <assert.h>
#include <string.h>

static uint8_t ram[0x10000], saved[0x10000];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & 0xfffffu;
}

static void word(uint16_t seg, uint16_t off, uint16_t value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(value >> 8);
}

static pr_slot_a_directory_state fixture(void)
{
    pr_slot_a_directory_state s;
    memset(ram, 0, sizeof ram);
    memset(&s, 0, sizeof s);
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x0c88; s.cpu.ip = 0x0c6e;
    s.cpu.ss = 0x0100; s.cpu.bp = 0x0200;
    s.cpu.ds = 0x0110; s.cpu.es = 0x0140;
    s.cpu.ax = 0x127e; s.cpu.bx = 0x1111;
    s.cpu.cx = 0x2222; s.cpu.dx = 0x3333;
    s.cpu.si = 0x4444; s.cpu.di = 0x5555;
    s.cpu.sp = 0x01e0; s.cpu.flags = 0x7212;
    word(s.cpu.ss, 0x01f8, 0x0182); /* BP-8 source index */
    ram[at(s.cpu.ss, 0x01ef)] = 2; /* BP-11 plane count */
    word(s.cpu.ss, 0x01f6, 0x127e); /* BP-0A data index */
    ram[at(s.cpu.ss, 0x01e7)] = 4; /* BP-19 row selector bound */
    ram[at(s.cpu.ds, 0x70c7)] = 3;
    word(s.cpu.ss, 0x020a, 0x0300);
    word(s.cpu.ss, 0x020c, 0x0200);
    return s;
}

int main(void)
{
    pr_slot_a_directory_state s = fixture();
    pr_slot_a_directory_cpu before;
    size_t changes = 0;
    size_t i;
    memcpy(saved, ram, sizeof ram);
    assert(pr_r464_loop_c6e(&s) == PR_R464_LOOP_NEXT_MASKED_BYTE);
    assert(s.cpu.cs == 0x0c88 && s.cpu.ip == 0x0c3d);
    assert(s.cpu.ax == 0x0183 && s.cpu.di == 0x0483 &&
           s.cpu.es == 0x0200 && s.cpu.flags == 0x7202);
    assert(s.cpu.bx == 0x1111 && s.cpu.cx == 0x2222 &&
           s.cpu.dx == 0x3333 && s.cpu.si == 0x4444 &&
           s.cpu.sp == 0x01e0);
    assert(ram[at(0x0100, 0x01f8)] == 0x83);
    assert(ram[at(0x0100, 0x01ef)] == 1);
    assert(ram[at(0x0100, 0x01f6)] == 0x7f);
    assert(ram[at(0x0110, 0x70c7)] == 4);
    for (i = 0; i < sizeof ram; ++i)
        if (ram[i] != saved[i]) ++changes;
    assert(changes == 4);

    /* The zero-plane and equal-selector arms have distinct successors.
     * They are typed boundaries and must leave this bounded model atomic. */
    s = fixture(); ram[at(0x0100, 0x01ef)] = 1;
    before = s.cpu; memcpy(saved, ram, sizeof ram);
    assert(pr_r464_loop_c6e(&s) == PR_R464_LOOP_OTHER_BRANCH);
    assert(!memcmp(saved, ram, sizeof ram) &&
           !memcmp(&before, &s.cpu, sizeof before));
    s = fixture(); ram[at(0x0110, 0x70c7)] = 4;
    before = s.cpu; memcpy(saved, ram, sizeof ram);
    assert(pr_r464_loop_c6e(&s) == PR_R464_LOOP_OTHER_BRANCH);
    assert(!memcmp(saved, ram, sizeof ram) &&
           !memcmp(&before, &s.cpu, sizeof before));

    s = fixture(); before = s.cpu; memcpy(saved, ram, sizeof ram);
    s.ram_size = at(0x0110, 0x70c7);
    assert(pr_r464_loop_c6e(&s) == PR_R464_LOOP_MEMORY_BOUNDARY);
    assert(!memcmp(saved, ram, sizeof ram) &&
           !memcmp(&before, &s.cpu, sizeof before));
    s = fixture(); s.cpu.ip = 0x0c3d;
    assert(pr_r464_loop_c6e(&s) == PR_R464_LOOP_WRONG_ENTRY);
    assert(pr_r464_loop_c6e(NULL) == PR_R464_LOOP_MEMORY_BOUNDARY);
    return 0;
}
