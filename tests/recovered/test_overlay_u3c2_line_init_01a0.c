#include "poolrad/recovered/overlay_u3c2_line_init_01a0.h"
#include <stdio.h>
#include <string.h>

#define CHECK(e) do { if (!(e)) { \
    fprintf(stderr, "U3C2 +01A0 line %d: %s\n", __LINE__, #e); return 1; \
} } while (0)

static uint8_t ram[PR_U3C2_0000_RAM_SIZE], before[PR_U3C2_0000_RAM_SIZE];

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U3C2_0000_RAM_SIZE - 1u);
}

static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t p = at(segment, offset);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U3C2_0000_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(uint16_t offset)
{
    size_t p = at(0x6000u, (uint16_t)(0x100u + offset));
    return (uint16_t)(ram[p] | ((uint16_t)ram[p + 1u] << 8));
}

static pr_u3c2_01a0_cpu setup(const uint16_t *points)
{
    pr_u3c2_01a0_cpu c = {
        0xA123u, 0x2345u, 0x3456u, 0x4567u, 0x5678u, 0x6789u,
        0x7777u, 0x8000u, 0x3000u, 0x01A0u, 0x1000u, 0x3333u,
        0x2000u, 0x7202u
    };
    unsigned i;
    memset(ram, 0xA5, sizeof ram);
    for (i = 0; i < 4u; ++i) put16(0x6000u, (uint16_t)(0x100u + i * 2u), points[i]);
    put16(c.ss, c.sp, 0x7000u);
    put16(c.ss, (uint16_t)(c.sp + 2u), c.cs);
    put16(c.ss, (uint16_t)(c.sp + 4u), 0x100u);
    put16(c.ss, (uint16_t)(c.sp + 6u), 0x6000u);
    return c;
}

static int records(void)
{
    static const struct {
        uint16_t points[4], dx, dy, decision, diagonal, straight;
        uint8_t sx, sy;
    } cases[] = {
        {{2,3,9,6}, 7,3, 0xFFFF,0xFFF8,6, 1,1},
        {{9,6,2,3}, 7,3, 0xFFFF,0xFFF8,6, 0xFF,0xFF},
        {{2,3,5,10}, 3,7, 0xFFFF,0xFFF8,6, 1,1},
        {{0,0,0,0}, 0,0, 0,0,0, 0,0},
        {{0,0,0x8000,1}, 0x8000,1, 0xFFFF,0xFFFE,0, 0xFF,1},
        {{0,0,0x7FFF,0x8000}, 0x7FFF,0x8000, 0x8001,2,0, 1,0xFF},
        {{0x8000,0xFFFF,0x7FFF,0}, 1,1, 1,0,2, 0xFF,1}
    };
    unsigned i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        pr_u3c2_01a0_cpu c = setup(cases[i].points);
        CHECK(pr_u3c2_line_init_01a0(ram, sizeof ram, &c) == PR_U3C2_01A0_OK);
        CHECK(get16(0xAu) == cases[i].dx && get16(0xCu) == cases[i].dy);
        CHECK(get16(8u) == cases[i].decision && get16(0xEu) == cases[i].diagonal);
        CHECK(get16(0x10u) == cases[i].straight);
        CHECK(get16(0x14u) == cases[i].points[0] && get16(0x16u) == cases[i].points[1]);
        CHECK(ram[at(0x6000u, 0x112u)] == cases[i].sx);
        CHECK(ram[at(0x6000u, 0x113u)] == cases[i].sy);
        CHECK(ram[at(0x6000u, 0x118u)] == 0u && ram[at(0x6000u, 0x119u)] == 0xA5u);
        CHECK(c.cs == 0x3000u && c.ip == 0x7000u && c.sp == 0x8008u && c.bp == 0x7777u);
        CHECK(c.bx == 0x2345u && c.cx == 0x3456u && c.si == 0x5678u);
        CHECK(c.ax == cases[i].straight && c.es == 0x6000u && c.di == 0x100u);
    }
    return 0;
}

static int rejects(void)
{
    const uint16_t points[] = {0,0,7,3};
    pr_u3c2_01a0_cpu c = setup(points), original = c;
    memcpy(before, ram, sizeof ram);
    CHECK(pr_u3c2_line_init_01a0(NULL, sizeof ram, &c) == PR_U3C2_01A0_ARGUMENT);
    CHECK(pr_u3c2_line_init_01a0(ram, sizeof ram - 1u, &c) == PR_U3C2_01A0_ARGUMENT);
    CHECK(pr_u3c2_line_init_01a0(ram, sizeof ram, NULL) == PR_U3C2_01A0_ARGUMENT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    c.sp = 15u;
    original = c;
    CHECK(pr_u3c2_line_init_01a0(ram, sizeof ram, &c) == PR_U3C2_01A0_ARGUMENT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    c = setup(points);
    c.ss = c.cs;
    c.sp = 0x01B0u;
    original = c;
    memcpy(before, ram, sizeof ram);
    CHECK(pr_u3c2_line_init_01a0(ram, sizeof ram, &c) == PR_U3C2_01A0_CODE_ALIAS);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    if (records() || rejects()) return 1;
    puts("U3C2 +01A0: line records, signed overflow, return ABI and rejection checks passed");
    return 0;
}
