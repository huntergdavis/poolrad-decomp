#include "poolrad/recovered/overlay_u25_string_cores.h"
#include <stdio.h>
#include <string.h>

#define CHECK(e) do { if (!(e)) { \
    fprintf(stderr, "U25 strings line %d: %s\n", __LINE__, #e); return 1; \
} } while (0)
static uint8_t ram[PR_U25_STRINGS_RAM_SIZE], before[PR_U25_STRINGS_RAM_SIZE];
static size_t p(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & 0xFFFFFu; }
static void word(uint16_t s, uint16_t o, uint16_t v)
{
    size_t a = p(s, o); ram[a] = (uint8_t)v; ram[(a + 1) & 0xFFFFFu] = (uint8_t)(v >> 8);
}
static uint16_t readword(uint16_t s, uint16_t o)
{
    size_t a = p(s, o); return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1) & 0xFFFFFu] << 8));
}
static pr_u25_strings_cpu setup(uint16_t ip, uint16_t first, uint16_t second)
{
    pr_u25_strings_cpu c = {
        0xA123, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
        0x7777, 0x8000, 0x3000, ip, 0x1000, 0x3333, 0x2000, 0x7602
    };
    memset(ram, 0xA5, sizeof ram);
    word(c.ss, c.sp, 0x7000); word(c.ss, c.sp + 2, c.cs);
    word(c.ss, c.sp + 4, first); word(c.ss, c.sp + 6, second);
    word(c.ds, 0x49DE, 0x1000); word(c.ds, 0x49E0, 0x6000);
    word(c.ds, 0x49ED, 0);
    return c;
}
static int unpack(void)
{
    /* Synthetic six-bit codes 1,2,0,63 packed MSB first: A,B,(skip),?. */
    static const uint8_t packed[] = {4, 32, 63};
    pr_u25_strings_cpu c = setup(0x1153, 0xAB01, 0xCD03);
    unsigned i;
    for (i = 0; i < 3; ++i) ram[p(0x6000, (uint16_t)(0x7701 + i))] = packed[i];
    CHECK(pr_u25_unpack_text_1153(ram, sizeof ram, &c, 0x4000, 16) == PR_U25_STRINGS_OK);
    CHECK(readword(0x1000, 0x49ED) == 3);
    CHECK(ram[p(0x1000, 0x70C8)] == 3);
    CHECK(memcmp(ram + p(0x1000, 0x70C9), "AB?", 3) == 0);
    CHECK(c.bp == 0x7777 && c.sp == 0x8008 && c.cs == 0x3000 && c.ip == 0x7000);
    CHECK((c.flags & 0x400) == 0); /* SYSTEM CLD is externally visible. */
    c = setup(0x1153, 1, 255);
    memset(ram + p(0x6000, 0x7701), 0xFF, 255);
    CHECK(pr_u25_unpack_text_1153(ram, sizeof ram, &c, 0x4000, 400) == PR_U25_STRINGS_OK);
    CHECK(readword(0x1000, 0x49ED) == 255 && ram[p(0x1000, 0x70C8)] == 255);
    for (i = 0; i < 255; ++i) CHECK(ram[p(0x1000, (uint16_t)(0x70C9 + i))] == '?');
    c = setup(0x1153, 1, 0);
    CHECK(pr_u25_unpack_text_1153(ram, sizeof ram, &c, 0x4000, 1) == PR_U25_STRINGS_OK);
    CHECK(readword(0x1000, 0x49ED) == 0 && ram[p(0x1000, 0x70C8)] == 0);
    CHECK((c.flags & 0x400) != 0); /* No runtime call on the empty path. */
    c = setup(0x1153, 1, 1);
    word(0x1000, 0x49ED, 0xFFFF);
    ram[p(0x6000, 0x7700)] = 4;
    CHECK(pr_u25_unpack_text_1153(ram, sizeof ram, &c, 0x4000, 4) == PR_U25_STRINGS_OK);
    CHECK(readword(0x1000, 0x49ED) == 0 && ram[p(0x1000, 0x70C9)] == 'A');
    return 0;
}
static int address(void)
{
    pr_u25_strings_cpu c = setup(0x12FB, 0x4900, 1);
    word(0x1000, 0x49D2, 0x1000); word(0x1000, 0x49D4, 0x6000);
    word(0x6000, 0x1000, 0x1200); /* Low zero is data: only a zero WORD terminates. */
    word(0x6000, 0x1002, 'Q'); word(0x6000, 0x1004, 0);
    CHECK(pr_u25_string_by_address_12fb(ram, sizeof ram, &c, 0x4000, 4) == PR_U25_STRINGS_OK);
    CHECK(ram[p(0x1000, 0x70C8)] == 2 && ram[p(0x1000, 0x70C9)] == 0 &&
          ram[p(0x1000, 0x70CA)] == 'Q');
    c = setup(0x12FB, 0x6B00, 1);
    word(0x1000, 0x5D92, 0x1000); word(0x1000, 0x5D94, 0x6000);
    ram[p(0x6000, 0x1000)] = 3; memcpy(ram + p(0x6000, 0x1001), "Ada", 3);
    CHECK(pr_u25_string_by_address_12fb(ram, sizeof ram, &c, 0x4000, 1) == PR_U25_STRINGS_OK);
    CHECK(ram[p(0x1000, 0x70C8)] == 3 && memcmp(ram + p(0x1000, 0x70C9), "Ada", 3) == 0);
    c = setup(0x12FB, 0xC000, 1);
    CHECK(pr_u25_string_by_address_12fb(ram, sizeof ram, &c, 0x4000, 1) == PR_U25_STRINGS_OK);
    CHECK(ram[p(0x1000, 0x70C8)] == 0);
    c = setup(0x12FB, 0x9900, 1);
    memset(ram + p(0x6000, 0x1000), 'x', 256);
    CHECK(pr_u25_string_by_address_12fb(ram, sizeof ram, &c, 0x4000, 270) == PR_U25_STRINGS_LIMIT);
    CHECK(c.cs == 0x3000 && c.ip == 0x14D2 && c.bp == 0x7FFE);
    CHECK(ram[p(c.ss, c.bp - 2)] == 14 && ram[p(0x1000, 0x70C8)] == 255);
    return 0;
}
static int rejects(void)
{
    pr_u25_strings_cpu c = setup(0x1153, 1, 1), original = c;
    memcpy(before, ram, sizeof ram);
    CHECK(pr_u25_unpack_text_1153(NULL, sizeof ram, &c, 0x4000, 1) == PR_U25_STRINGS_ARGUMENT);
    CHECK(pr_u25_unpack_text_1153(ram, sizeof ram - 1, &c, 0x4000, 1) == PR_U25_STRINGS_ARGUMENT);
    CHECK(pr_u25_unpack_text_1153(ram, sizeof ram, NULL, 0x4000, 1) == PR_U25_STRINGS_ARGUMENT);
    CHECK(pr_u25_unpack_text_1153(ram, sizeof ram, &c, 0x4000, 0) == PR_U25_STRINGS_ARGUMENT);
    CHECK(pr_u25_string_by_address_12fb(ram, sizeof ram, &c, 0x4000, 1) == PR_U25_STRINGS_ARGUMENT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    c.sp = 0x217; original = c;
    CHECK(pr_u25_unpack_text_1153(ram, sizeof ram, &c, 0x4000, 1) == PR_U25_STRINGS_ARGUMENT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    return 0;
}
int main(void)
{
    if (unpack() || address() || rejects()) return 1;
    puts("U25 string cores: decode, runtime cap, PC wrap, address classes, exact limit and rejection pass");
    return 0;
}
