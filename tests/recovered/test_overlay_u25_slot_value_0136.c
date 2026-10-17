#include "poolrad/recovered/overlay_u25_slot_value_0136.h"
#include <stdio.h>
#include <string.h>

#define CHECK(e) do { if (!(e)) { \
    fprintf(stderr, "U25 +0136 line %d: %s\n", __LINE__, #e); return 1; \
} } while (0)
static uint8_t ram[PR_U25_0136_RAM_SIZE], before[PR_U25_0136_RAM_SIZE];
static size_t p(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & 0xFFFFFu; }
static void word(uint16_t s, uint16_t o, uint16_t v)
{
    size_t a = p(s, o); ram[a] = (uint8_t)v; ram[(a + 1) & 0xFFFFFu] = (uint8_t)(v >> 8);
}
static uint16_t readword(uint16_t s, uint16_t o)
{
    size_t a = p(s, o); return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1) & 0xFFFFFu] << 8));
}
static pr_u25_slot_value_cpu setup(uint8_t tag, uint8_t index, uint16_t value,
                                  uint16_t ss, uint16_t sp)
{
    pr_u25_slot_value_cpu c = {0x9234, 0x1357, 0x2468, 0xABCD, 0x5678, 0x6789,
                              0x4321, sp, 0x3000, 0x0136, 0x1000, 0x6000, ss, 0x0AD7};
    uint16_t i = index >= 128u ? (uint16_t)(0xFF00u | index) : index;
    memset(ram, 0xA5, sizeof ram);
    ram[p(c.ds, (uint16_t)(0x7085u + i))] = (uint8_t)value;
    ram[p(c.ds, (uint16_t)(0x7045u + i))] = (uint8_t)(value >> 8);
    ram[p(c.ds, (uint16_t)(0x7005u + i))] = tag;
    ram[p(c.ds, 0x6AB1)] = 0x7E;
    word(c.ds, 0x5D92, 0x100); word(c.ds, 0x5D94, 0x5000);
    word(c.ds, 0x5D96, 0); word(c.ds, 0x5D98, 0);
    ram[p(0x5000, 0x114)] = 7;
    word(ss, (uint16_t)(sp - 4u), 0xD35A);
    word(ss, sp, 0x1111);
    ram[(p(ss, sp) + 2u) & 0xFFFFFu] = 0;
    ram[(p(ss, sp) + 3u) & 0xFFFFFu] = 0x70;
    word(ss, (uint16_t)(sp + 4u), (uint16_t)(0xAB00u | index));
    return c;
}
static int paths(void)
{
    static const uint8_t tags[] = {0, 1, 2, 3, 0x80, 0x81, 0xFF};
    static const uint16_t wants[] = {0x4F, 0x7E, 0xC04F, 0x7E, 0x7E, 0xC04F, 0xD35A};
    unsigned index, t;
    for (index = 0; index < 256; ++index) {
        for (t = 0; t < sizeof tags; ++t) {
            pr_u25_slot_value_cpu c = setup(tags[t], (uint8_t)index, 0xC04F, 0x2000, 0x8000);
            CHECK(pr_u25_slot_value_0136(ram, sizeof ram, &c) == PR_U25_SLOT_VALUE_OK);
            CHECK(c.ax == wants[t] && c.bp == 0x4321 && c.sp == 0x8006);
            CHECK(c.cs == 0x7000 && c.ip == 0x1111);
            CHECK(readword(0x2000, 0x7FFC) == wants[t]);
        }
    }
    {
        pr_u25_slot_value_cpu c = setup(1, 1, 0x6B18, 0x2000, 0x8000);
        CHECK(pr_u25_slot_value_0136(ram, sizeof ram, &c) == PR_U25_SLOT_VALUE_OK);
        CHECK(c.ax == 7); /* Actual active-record byte, not a callback result. */
    }
    return 0;
}
static int wraps_and_rejects(void)
{
    static const uint16_t stacks[] = {0, 1, 2, 0x12, 0x13, 0x14, 0x15, 0xFFFE, 0xFFFF};
    unsigned i;
    for (i = 0; i < sizeof stacks / sizeof stacks[0]; ++i) {
        pr_u25_slot_value_cpu c = setup(2, 0xFF, 0xBEEF, 0xFFFF, stacks[i]);
        if (stacks[i] == 0x12 || stacks[i] == 0x13) {
            /* PUSH CS and CALL wrap independently; RETF's contiguous CS
             * read then observes different RAM and transfers elsewhere. */
            CHECK(pr_u25_slot_value_0136(ram, sizeof ram, &c) == PR_U25_SLOT_VALUE_TRANSFER);
            CHECK(c.ax == 0xBEEF && c.cs == 0xA5A5 && c.ip == 0x01AA);
            CHECK(c.bp == (uint16_t)(stacks[i] - 2u) && c.sp == (uint16_t)(stacks[i] - 12u));
        } else {
            CHECK(pr_u25_slot_value_0136(ram, sizeof ram, &c) == PR_U25_SLOT_VALUE_OK);
            CHECK(c.ax == 0xBEEF && c.bp == 0x4321 && c.sp == (uint16_t)(stacks[i] + 6u));
            CHECK(c.cs == 0x7000 && c.ip == 0x1111);
        }
    }
    for (i = 0x10; i <= 0x11; ++i) {
        pr_u25_slot_value_cpu c = setup(1, 1, 0xC04F, 0xFFFF, (uint16_t)i);
        CHECK(pr_u25_slot_value_0136(ram, sizeof ram, &c) == PR_U25_SLOT_VALUE_TRANSFER);
        CHECK(c.ax == 0x7E && c.cs == 0xA5A5 && c.ip == 0x0191);
        CHECK(c.bp == i - 2u && c.sp == i - 12u);
    }
    {
        pr_u25_slot_value_cpu c = setup(1, 1, 0xC04F, 0x2000, 0x8000), original = c;
        memcpy(before, ram, sizeof ram);
        CHECK(pr_u25_slot_value_0136(NULL, sizeof ram, &c) == PR_U25_SLOT_VALUE_ARGUMENT);
        CHECK(pr_u25_slot_value_0136(ram, sizeof ram - 1, &c) == PR_U25_SLOT_VALUE_ARGUMENT);
        CHECK(pr_u25_slot_value_0136(ram, sizeof ram, NULL) == PR_U25_SLOT_VALUE_ARGUMENT);
        CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
        c.ip = 0; original = c;
        CHECK(pr_u25_slot_value_0136(ram, sizeof ram, &c) == PR_U25_SLOT_VALUE_ARGUMENT);
        CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    }
    return 0;
}
int main(void)
{
    if (paths() || wraps_and_rejects()) return 1;
    puts("U25 +0136: signed indices, seven paths, real load/word results and A20 stack wrap pass");
    return 0;
}
