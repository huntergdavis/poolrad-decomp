#include "poolrad/recovered/overlay_u3c2_caller_windows_0909.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t address(uint16_t ss, uint16_t offset)
{
    return (((size_t)ss << 4) + offset) & ((1u << 20) - 1u);
}

int main(void)
{
    static const uint16_t segments[] = {0, 0x2000, 0xFFFF};
    static const uint16_t stacks[] = {0, 1, 2, 3, 0x11, 0x17, 0x8000, 0xFFFE, 0xFFFF};
    static const uint8_t indices[] = {0, 1, 127, 255};
    uint8_t *ram = malloc(1u << 20), *expected = malloc(1u << 20);
    unsigned window, segment, stack, index;
    assert(ram && expected);
    for (window = 0; window < 2; ++window)
    for (segment = 0; segment < 3; ++segment)
    for (stack = 0; stack < 9; ++stack)
    for (index = 0; index < 4; ++index) {
        pr_u6c_3f31_cpu cpu = {
            0xABCD, 0x1234, 0x2345, 0x3456, 0x4567, 0x5678,
            0x7000, stacks[stack], 0xF000, window ? 0x0B73 : 0x0A71,
            0x1000, 0x3000, segments[segment], 0x0ED7
        }, want;
        uint16_t values[7] = {0x11, 0x22, 0x33, 0x44, 0x55, cpu.cs,
                              window ? 0x0BA7 : 0x0AA5};
        int first = window ? -5 : -3, second = first - 1;
        unsigned i;
        memset(ram, 0xA5, 1u << 20);
        ram[address(cpu.ss, (uint16_t)(cpu.bp + first))] = indices[index];
        ram[address(cpu.ss, (uint16_t)(cpu.bp + second))] = indices[index];
        for (i = 0; i < 4; ++i)
            ram[address(cpu.ss, (uint16_t)(cpu.bp + indices[index] - 15 - 4 * i))] = (uint8_t)values[i];
        ram[address(cpu.ss, (uint16_t)(cpu.bp + (window ? -2 : 12)))] = 0x55;
        memcpy(expected, ram, 1u << 20);
        want = cpu;
        for (i = 0; i < 7; ++i) {
            size_t at;
            want.sp = (uint16_t)(want.sp - 2);
            at = address(want.ss, want.sp);
            expected[at] = (uint8_t)values[i];
            expected[(at + 1) & ((1u << 20) - 1u)] = (uint8_t)(values[i] >> 8);
        }
        want.ax = 0x55;
        want.di = indices[index];
        want.ip = 0x057B;
        want.flags = 0x0646;
        assert(pr_u3c2_caller_windows_0909(ram, 1u << 20, &cpu) == PR_U3C2_CALLER_WINDOWS_OK);
        assert(memcmp(ram, expected, 1u << 20) == 0);
        assert(memcmp(&cpu, &want, sizeof(cpu)) == 0);
    }
    {
        pr_u6c_3f31_cpu cpu = {0}, before;
        cpu.ip = 0x0909;
        before = cpu;
        memcpy(expected, ram, 1u << 20);
        assert(pr_u3c2_caller_windows_0909(ram, 1u << 20, &cpu) == PR_U3C2_CALLER_WINDOWS_ARGUMENT);
        assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
        assert(memcmp(ram, expected, 1u << 20) == 0);
        cpu.ip = 0x0A71;
        before = cpu;
        assert(pr_u3c2_caller_windows_0909(NULL, 1u << 20, &cpu) == PR_U3C2_CALLER_WINDOWS_ARGUMENT);
        assert(pr_u3c2_caller_windows_0909(ram, 1u << 20, NULL) == PR_U3C2_CALLER_WINDOWS_ARGUMENT);
        assert(pr_u3c2_caller_windows_0909(ram, (1u << 20) - 1u, &cpu) == PR_U3C2_CALLER_WINDOWS_ARGUMENT);
        assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
        assert(memcmp(ram, expected, 1u << 20) == 0);
    }
    free(expected);
    free(ram);
    return 0;
}
