#include "poolrad/recovered/overlay_u25_skip_1ea6.h"
#include <stdio.h>
#include <string.h>

#define CHECK(e) do { if (!(e)) { \
    fprintf(stderr, "U25 +1EA6 line %d: %s\n", __LINE__, #e); return 1; \
} } while (0)
static uint8_t ram[PR_U25_1EA6_RAM_SIZE], before[PR_U25_1EA6_RAM_SIZE];
static size_t p(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & 0xFFFFFu; }
static void word(uint16_t s, uint16_t o, uint16_t v)
{
    size_t a = p(s, o); ram[a] = (uint8_t)v; ram[(a + 1) & 0xFFFFFu] = (uint8_t)(v >> 8);
}
static uint16_t readword(uint16_t s, uint16_t o)
{
    size_t a = p(s, o); return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1) & 0xFFFFFu] << 8));
}
static pr_u25_1ea6_cpu setup(uint8_t opcode, const uint8_t *stream, size_t length)
{
    pr_u25_1ea6_cpu c = {
        0xA123, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
        0x7777, 0x8000, 0x3000, 0x1EA6, 0x1000, 0x3333, 0x2000, 0x7602
    };
    memset(ram, 0xA5, sizeof ram);
    word(c.ss, c.sp, 0x7000); word(c.ss, c.sp + 2, c.cs);
    ram[p(0x6300, 0x1000)] = opcode;
    word(c.ds, 0x49DE, 0x1000); word(c.ds, 0x49E0, 0x6300);
    word(c.ds, 0x49D2, 0x1000); word(c.ds, 0x49D4, 0x6000);
    word(c.ds, 0x49ED, 0x9900);
    if (length) memcpy(ram + p(0x6300, 0x1001), stream, length);
    word(0x6000, 0x1000, 'Q'); word(0x6000, 0x1002, 0);
    return c;
}
static int mixed(void)
{
    static const uint8_t stream[] = {
        1, 0x34, 0x12, 0x80, 3, 4, 32, 63, 0x81, 0, 0x49, 0x80, 0, 0xFF, 0xDA
    };
    pr_u25_1ea6_cpu c = setup(0x2E, stream, sizeof stream);
    CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram, &c, 0x4000, 5, 32) == PR_U25_1EA6_OK);
    CHECK(readword(0x1000, 0x49ED) == 0x9910);
    CHECK(ram[p(0x1000, 0x6F7F)] == 0x2E);
    CHECK(ram[p(0x1000, 0x7006)] == 1 && ram[p(0x1000, 0x7086)] == 0x34 &&
          ram[p(0x1000, 0x7046)] == 0x12);
    CHECK(ram[p(0x1000, 0x7047)] == 0xA5); /* Packed string does not clear stale high byte. */
    CHECK(ram[p(0x1000, 0x70C8)] == 3 && memcmp(ram + p(0x1000, 0x70C9), "AB?", 3) == 0);
    CHECK(ram[p(0x1000, 0x71C8)] == 1 && ram[p(0x1000, 0x71C9)] == 'Q');
    CHECK(ram[p(0x1000, 0x72C8)] == 0 && ram[p(0x1000, 0x72C9)] == 0xA5);
    CHECK(ram[p(0x1000, 0x700A)] == 0xFF && ram[p(0x1000, 0x708A)] == 0xDA);
    CHECK(c.bp == 0x7777 && c.sp == 0x8004 && c.cs == 0x3000 && c.ip == 0x7000);
    return 0;
}
static int counts_and_limits(void)
{
    static const uint8_t opcodes[] = {1, 3, 4, 0x14, 0x2E, 0x1E, 0x27, 0x29};
    static const uint8_t counts[] = {1, 2, 3, 4, 5, 6, 8, 14};
    uint8_t stream[28];
    pr_u25_1ea6_cpu c;
    unsigned i;
    for (i = 0; i < 14; ++i) { stream[2*i] = 0; stream[2*i+1] = (uint8_t)(i + 1u); }
    for (i = 0; i < sizeof opcodes; ++i) {
        c = setup(opcodes[i], stream, sizeof stream);
        CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram, &c, 0x4000, 14, 1) == PR_U25_1EA6_OK);
        CHECK(readword(0x1000, 0x49ED) == 0x9901 + 2*counts[i]);
        CHECK(ram[p(0x1000, (uint16_t)(0x7085 + counts[i]))] == counts[i]);
        CHECK(ram[p(0x1000, (uint16_t)(0x7006 + counts[i]))] == 0xA5);
    }
    c = setup(0xFF, NULL, 0);
    word(c.ds, 0x49ED, 0xFFFF);
    ram[p(0x6300, 0x76FF)] = 0xFF;
    CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram, &c, 0x4000, 1, 1) == PR_U25_1EA6_OK);
    CHECK(readword(0x1000, 0x49ED) == 0 && ram[p(0x1000, 0x7006)] == 0xA5);
    CHECK(c.bp == 0x7777 && c.sp == 0x8004 && c.ip == 0x7000);
    c = setup(4, stream, 6);
    CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram, &c, 0x4000, 1, 8) == PR_U25_1EA6_OPERAND_LIMIT);
    CHECK(c.ip == 0x22 && c.bp == 0x7FF6 && c.sp == 0x7FEE);
    CHECK(readword(0x1000, 0x49ED) == 0x9902 && ram[p(c.ss, c.bp - 1)] == 2);
    {
        static const uint8_t packed[] = {0x80, 3, 4, 32, 63};
        c = setup(1, packed, sizeof packed);
        CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram, &c, 0x4000, 1, 1) == PR_U25_1EA6_STRING_LIMIT);
        CHECK(c.ip == 0x1172 && c.bp == 0x7FE4 && c.sp == 0x7DDC);
        CHECK(readword(0x1000, 0x49ED) == 0x9903 && ram[p(0x1000, 0x70C8)] == 1);
    }
    return 0;
}
static int rejects(void)
{
    pr_u25_1ea6_cpu c = setup(0, NULL, 0), original = c;
    memcpy(before, ram, sizeof ram);
    CHECK(pr_u25_skip_statement_1ea6(NULL, sizeof ram, &c, 0x4000, 1, 1) == PR_U25_1EA6_ARGUMENT);
    CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram - 1, &c, 0x4000, 1, 1) == PR_U25_1EA6_ARGUMENT);
    CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram, NULL, 0x4000, 1, 1) == PR_U25_1EA6_ARGUMENT);
    CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram, &c, 0x4000, 0, 1) == PR_U25_1EA6_ARGUMENT);
    CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram, &c, 0x4000, 1, 0) == PR_U25_1EA6_ARGUMENT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    c.sp = 0x231; original = c;
    CHECK(pr_u25_skip_statement_1ea6(ram, sizeof ram, &c, 0x4000, 1, 1) == PR_U25_1EA6_ARGUMENT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    return 0;
}
int main(void)
{
    if (mixed() || counts_and_limits() || rejects()) return 1;
    puts("U25 +1EA6: mixed strings, eight count groups, fallback wrap and nested limits pass");
    return 0;
}
