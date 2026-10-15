#include "poolrad/recovered/overlay_u3c2_entry_prefix_0909.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t address(uint16_t ss, uint16_t offset)
{
    return (((size_t)ss << 4) + offset) & ((1u << 20) - 1u);
}

static void check(uint8_t *ram, uint8_t *expected, uint16_t ss,
                  uint16_t sp, uint16_t flags)
{
    pr_u6c_3f31_cpu cpu = {
        0x1234, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
        0x7788, sp, 0xF000, 0x0909, 0x1000, 0x3000, ss, flags
    }, want = cpu;
    uint32_t a = (uint16_t)(sp - 2u);
    int32_t difference = (int32_t)a - 32;
    int32_t signed_result = (a < 32768 ? (int32_t)a : (int32_t)a - 65536) - 32;
    unsigned ones = 0, bit;
    size_t at = address(ss, (uint16_t)a);
    memset(ram, 0xA5, 1u << 20);
    memcpy(expected, ram, 1u << 20);
    expected[at] = 0x88;
    expected[(at + 1) & ((1u << 20) - 1u)] = 0x77;
    expected[address(ss, (uint16_t)(a - 3u))] = 0;
    want.bp = (uint16_t)a;
    want.sp = (uint16_t)difference;
    want.ip = 0x0913;
    want.flags &= 0xF72A;
    if (difference < 0) want.flags |= 1;
    if (want.sp == 0) want.flags |= 0x40;
    if (want.sp >= 32768) want.flags |= 0x80;
    if (signed_result < -32768 || signed_result > 32767) want.flags |= 0x800;
    for (bit = 0; bit < 8; ++bit) ones += (want.sp >> bit) & 1u;
    if (ones % 2 == 0) want.flags |= 4;
    /* Subtracting 32 cannot borrow across bit 3: AF is defined zero. */
    assert(!(want.flags & 0x10));
    assert(pr_u3c2_entry_prefix_0909(ram, 1u << 20, &cpu) == PR_U3C2_0909_PREFIX_OK);
    assert(memcmp(ram, expected, 1u << 20) == 0);
    assert(memcmp(&cpu, &want, sizeof(cpu)) == 0);
}

int main(void)
{
    static const uint16_t segments[] = {0, 0x2000, 0xFFFF};
    static const uint16_t flags[] = {0x0202, 0x0ED7, 0x0047, 0x0A93, 0x0602, 0x0002};
    static const uint16_t stacks[] = {0, 1, 2, 0x21, 0x22, 0x8002, 0x8021, 0xFFFF};
    uint8_t *ram = malloc(1u << 20), *expected = malloc(1u << 20);
    unsigned s, n;
    assert(ram && expected);
    for (s = 0; s < 3; ++s)
    for (n = 0; n < 352; ++n) {
        uint16_t sp = (uint16_t)(n < 256 ? n : n < 336 ? 0x7FF0 + n - 256 : 0xFFF0 + n - 336);
        check(ram, expected, segments[s], sp, 0x0ED7);
    }
    for (s = 0; s < 6; ++s)
    for (n = 0; n < 8; ++n) check(ram, expected, 0x2000, stacks[n], flags[s]);
    {
        pr_u6c_3f31_cpu cpu = {0}, before;
        cpu.ip = 0x0913;
        before = cpu;
        memcpy(expected, ram, 1u << 20);
        assert(pr_u3c2_entry_prefix_0909(ram, 1u << 20, &cpu) == PR_U3C2_0909_PREFIX_ARGUMENT);
        assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
        assert(memcmp(ram, expected, 1u << 20) == 0);
        cpu.ip = 0x0909;
        before = cpu;
        assert(pr_u3c2_entry_prefix_0909(NULL, 1u << 20, &cpu) == PR_U3C2_0909_PREFIX_ARGUMENT);
        assert(pr_u3c2_entry_prefix_0909(ram, 1u << 20, NULL) == PR_U3C2_0909_PREFIX_ARGUMENT);
        assert(pr_u3c2_entry_prefix_0909(ram, (1u << 20) - 1u, &cpu) == PR_U3C2_0909_PREFIX_ARGUMENT);
        assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
        assert(memcmp(ram, expected, 1u << 20) == 0);
    }
    free(expected);
    free(ram);
    return 0;
}
