#include "poolrad/recovered/resident_464_masked_byte_0c3d.h"

#include <assert.h>
#include <string.h>

static uint8_t ram[0x3000];

static size_t at(uint16_t seg, uint16_t off)
{
    return ((((size_t)seg << 4) + off) & 0xfffffu);
}

static void word(uint16_t seg, uint16_t off, uint16_t value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(value >> 8);
}

static pr_slot_a_directory_state fixture(uint16_t source_offset,
                                         uint16_t dest_offset)
{
    pr_slot_a_directory_state s;
    memset(ram, 0, sizeof ram);
    memset(&s, 0, sizeof s);
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x0c88; s.cpu.ip = 0x0c3d;
    s.cpu.ss = 0x0100; s.cpu.bp = 0x0200;
    s.cpu.es = 0x0200; s.cpu.di = source_offset;
    s.cpu.ax = 0x4444; s.cpu.bx = 0x5555;
    s.cpu.cx = 0x6601; s.cpu.dx = 0x7702;
    s.cpu.si = 0x1234; s.cpu.sp = 0x01e0;
    s.cpu.ds = 0x0110; s.cpu.flags = 0x7202;
    word(s.cpu.ss, 0x01f0, 0x0020); /* BP-10: mask index */
    word(s.cpu.ss, 0x01f6, 0x0030); /* BP-0A: destination index */
    word(s.cpu.ss, 0x0206, dest_offset);
    word(s.cpu.ss, 0x0208, 0x0140);
    word(s.cpu.ss, 0x020a, source_offset);
    word(s.cpu.ss, 0x020c, 0x0200);
    word(0x0200, (uint16_t)(source_offset + 0x13u), 0x0400);
    word(0x0200, (uint16_t)(source_offset + 0x15u), 0x0130);
    ram[at(0x0130, 0x0420)] = 0x0f;
    ram[at(0x0200, (uint16_t)(source_offset + 0x17u))] = 0x80;
    ram[at(0x0140, (uint16_t)(dest_offset + 0x47u))] = 0xa5;
    return s;
}

int main(void)
{
    pr_slot_a_directory_state s = fixture(0x0300, 0x0500);
    pr_slot_a_directory_cpu before;
    uint8_t saved[sizeof ram];
    assert(pr_r464_masked_byte_0c3d(&s) == PR_R464_MASKED_BYTE_AT_0C6E);
    assert(ram[at(0x0140, 0x0547)] == 0x85);
    assert(s.cpu.ax == 0x0030 && s.cpu.di == 0x0530);
    assert(s.cpu.cx == 0x6680 && s.cpu.dx == 0x7785);
    assert(s.cpu.es == 0x0140 && s.cpu.ip == 0x0c6e);
    assert(s.cpu.bx == 0x5555 && s.cpu.si == 0x1234 &&
           s.cpu.sp == 0x01e0 && s.cpu.flags == 0x7206);

    /* A reached store can leave its destination unchanged. */
    s = fixture(0x0300, 0x0500);
    ram[at(0x0140, 0x0547)] = 0x85;
    assert(pr_r464_masked_byte_0c3d(&s) == PR_R464_MASKED_BYTE_AT_0C6E);
    assert(ram[at(0x0140, 0x0547)] == 0x85);

    /* 16-bit offset wrap applies both to LES's source-header field and to
     * the destination DI addition; carry from that addition is observable. */
    s = fixture(0xfff0, 0xfff0);
    assert(pr_r464_masked_byte_0c3d(&s) == PR_R464_MASKED_BYTE_AT_0C6E);
    assert(ram[at(0x0140, 0x0037)] == 0x85);
    assert(s.cpu.di == 0x0020 && (s.cpu.flags & 1u) == 1u);

    s = fixture(0x0300, 0x0500);
    before = s.cpu; memcpy(saved, ram, sizeof ram);
    s.cpu.ip = 0x0c6e;
    assert(pr_r464_masked_byte_0c3d(&s) == PR_R464_MASKED_BYTE_WRONG_ENTRY);
    assert(s.cpu.ip == 0x0c6e && !memcmp(saved, ram, sizeof ram));
    s.cpu = before;
    s.ram_size = at(0x0140, 0x0547);
    assert(pr_r464_masked_byte_0c3d(&s) ==
           PR_R464_MASKED_BYTE_MEMORY_BOUNDARY);
    assert(!memcmp(&s.cpu, &before, sizeof before));
    assert(!memcmp(saved, ram, sizeof ram));
    assert(pr_r464_masked_byte_0c3d(NULL) ==
           PR_R464_MASKED_BYTE_MEMORY_BOUNDARY);
    return 0;
}
