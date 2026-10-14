#include "poolrad/recovered/resident_464_bitmask_frame.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    uint8_t ram[256], expected[256];
    unsigned selector, mask, i;
    pr_slot_a_directory_state s;
    pr_slot_a_directory_cpu cpu;
    /* Every comparison-outcome mask for every nibble selector. */
    for (selector = 0; selector < 16; ++selector) {
        for (mask = 0; mask < 256; ++mask) {
            memset(ram, 0xa5, sizeof ram);
            memset(&s, 0, sizeof s);
            s.ram = ram; s.ram_size = sizeof ram;
            s.cpu.sp = 64; s.cpu.bp = 0x1234; s.cpu.cs = 0xc88;
            s.cpu.ax = 0xabcd; s.cpu.cx = 0x5678; s.cpu.di = 0x789a;
            s.cpu.flags = 0xfd7;
            ram[64] = 0x7e; ram[65] = 6; ram[66] = 0x88; ram[67] = 0xc;
            for (i = 0; i < 8; ++i) {
                unsigned nibble = (mask & (0x80u >> i)) ? selector : (selector ^ 1u);
                if (!(i & 1u)) ram[68 + i / 2] = (uint8_t)(nibble << 4);
                else ram[68 + i / 2] |= (uint8_t)nibble;
            }
            ram[72] = (uint8_t)selector; ram[73] = 0xd3;
            memcpy(expected, ram, sizeof ram); cpu = s.cpu;
            expected[57] = 3; expected[58] = 0;
            expected[59] = (uint8_t)(selector << 4);
            expected[60] = expected[61] = (uint8_t)mask;
            expected[62] = 0x34; expected[63] = 0x12;
            cpu.ax = (uint16_t)mask; cpu.cx = 4; cpu.di = 3;
            cpu.sp = 74; cpu.ip = 0x67e;
            cpu.flags = (uint16_t)((cpu.flags & (uint16_t)~0x08d5u) | 0x44u);
            assert(pr_r464_bitmask_frame(&s));
            assert(!memcmp(&s.cpu, &cpu, sizeof cpu));
            assert(!memcmp(ram, expected, sizeof ram));
        }
    }
    cpu = s.cpu;
    assert(!pr_r464_bitmask_frame(&s)); /* Return PC is outside this leaf. */
    assert(!memcmp(&cpu, &s.cpu, sizeof cpu));
    s.cpu.ip = 0; s.cpu.sp = 252;
    cpu = s.cpu; memcpy(expected, ram, sizeof ram);
    assert(!pr_r464_bitmask_frame(&s));
    assert(!memcmp(&cpu, &s.cpu, sizeof cpu));
    assert(!memcmp(ram, expected, sizeof ram));
    s.cpu.sp = 7;
    assert(!pr_r464_bitmask_frame(&s));
    assert(!pr_r464_bitmask_frame(NULL));
    return 0;
}
