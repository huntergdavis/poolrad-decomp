#include "loaded_stub_fixture.h"
#include "poolrad/recovered/overlay_u5e_direction_gate_17f6.h"

#include <stdio.h>
#include <string.h>

#define CHECK(e) do { if (!(e)) { \
    fprintf(stderr, "U5E +17F6 line %d: %s\n", __LINE__, #e); return 1; \
} } while (0)

static uint8_t ram[PR_U5E_263D_RAM_SIZE];
static uint8_t before[PR_U5E_263D_RAM_SIZE];
static const pr_u5e_263d_layout layout = {0x4000u, 0x5000u};

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U5E_263D_RAM_SIZE - 1u);
}

static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t p = physical(segment, offset);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U5E_263D_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u5e_263d_cpu setup(uint8_t x, uint8_t y)
{
    /* Synthetic compass steps; these are not extracted game data. */
    static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1, 0};
    static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1, 0};
    pr_u5e_263d_cpu c = {
        0xA123u, 0x2345u, 0x3456u, 0x4567u, 0x5678u, 0x6789u,
        0x7777u, 0x8000u, 0x3000u, 0x17F6u, 0x1000u, 0x3333u,
        0x2000u, 0x7202u
    };
    unsigned i;
    memset(ram, 0xA5, sizeof ram);
    test_loaded_stub(ram, c.cs, 0x2649u, (uint16_t)(layout.slots_cs + 0x100u), 0x005Cu, layout.slots_cs, 0x0BE7u);
    test_loaded_stub(ram, c.cs, 0x2657u, (uint16_t)(layout.slots_cs + 0x100u), 0x0061u, layout.slots_cs, 0x0C0Fu);
    test_loaded_stub(ram, c.cs, 0x2665u, (uint16_t)(layout.slots_cs + 0x100u), 0x005Cu, layout.slots_cs, 0x0BE7u);
    test_loaded_stub(ram, c.cs, 0x2673u, (uint16_t)(layout.slots_cs + 0x100u), 0x0061u, layout.slots_cs, 0x0C0Fu);
    test_loaded_stub(ram, c.cs, 0x2693u, (uint16_t)(layout.predicate_cs + 0x100u), 0x0034u, layout.predicate_cs, 0x057Bu);
    put16(c.ss, c.sp, 0x7000u);
    put16(c.ss, c.sp + 2u, c.cs);
    put16(c.ss, c.sp + 4u, 0x0200u);
    put16(c.ss, c.sp + 6u, 0x6000u);
    put16(c.ss, c.sp + 8u, 0x0100u);
    put16(c.ss, c.sp + 10u, 0x6000u);
    put16(c.ds, 0x65BDu, 0x0100u);
    put16(c.ds, 0x65BFu, 0x6000u);
    put16(c.ds, 0x65C1u, 0x0200u);
    put16(c.ds, 0x65C3u, 0x6000u);
    ram[physical(c.ds, 0x5F2Au)] = 2u;
    ram[physical(c.ds, 0x5F2Bu)] = 10u;
    ram[physical(c.ds, 0x5F2Cu)] = 10u;
    ram[physical(c.ds, 0x5F2Fu)] = x;
    ram[physical(c.ds, 0x5F30u)] = y;
    for (i = 0; i < 9u; ++i) {
        ram[physical(c.ds, (uint16_t)(0x2736u + i))] = (uint8_t)dx[i];
        ram[physical(c.ds, (uint16_t)(0x273Fu + i))] = (uint8_t)dy[i];
    }
    put16(0x6000u, 0x0308u, 0x0100u);
    put16(0x6000u, 0x030Au, 0x7000u);
    ram[physical(0x7000u, 0x0109u)] = 0u;
    ram[physical(0x7000u, 0x010Fu)] = 255u;
    ram[physical(0x7000u, 0x0112u)] = 7u;
    return c;
}

static int results(void)
{
    pr_u5e_263d_cpu c = setup(10u, 10u);
    CHECK(pr_u5e_direction_gate_17f6(ram, sizeof ram, &c, &layout, 2048u) == PR_U5E_263D_OK);
    CHECK(ram[physical(0x7000u, 0x010Fu)] == 0u);
    CHECK(ram[physical(0x7000u, 0x0112u)] == 7u);
    CHECK(c.ax == 7u && c.dx == 0u && c.cx == 8u);
    CHECK(c.cs == 0x3000u && c.ip == 0x7000u && c.sp == 0x800Cu && c.bp == 0x7777u);
    CHECK(c.bx == 0x2345u && c.si == 0x5678u && c.es == 0x7000u && c.di == 0x0100u);
    c = setup(10u, 10u);
    /* (0-255+8) % 8 = -7; storing its byte gives 249, then 8-249
     * stores 15. (7+15)%8 = 6. Unsigned modulo would give 0 instead. */
    ram[physical(0x7000u, 0x0109u)] = 255u;
    CHECK(pr_u5e_direction_gate_17f6(ram, sizeof ram, &c, &layout, 2048u) == PR_U5E_263D_OK);
    CHECK(ram[physical(0x7000u, 0x0112u)] == 6u);
    CHECK(c.ax == 6u && c.dx == 2u && c.cx == 8u);
    return 0;
}

static int limit(void)
{
    pr_u5e_263d_cpu c = setup(10u, 10u);
    CHECK(pr_u5e_direction_gate_17f6(ram, sizeof ram, &c, &layout, 1u) == PR_U5E_263D_LIMIT);
    CHECK(c.cs == layout.slots_cs && c.ip == 0x0C71u);
    CHECK(ram[physical(0x7000u, 0x010Fu)] == 0u);
    CHECK(ram[physical(0x7000u, 0x0112u)] == 7u);
    return 0;
}

static int rejects(void)
{
    pr_u5e_263d_cpu c = setup(10u, 10u), original = c;
    memcpy(before, ram, sizeof ram);
    CHECK(pr_u5e_direction_gate_17f6(NULL, sizeof ram, &c, &layout, 10u) == PR_U5E_263D_ARGUMENT);
    CHECK(pr_u5e_direction_gate_17f6(ram, sizeof ram - 1u, &c, &layout, 10u) == PR_U5E_263D_ARGUMENT);
    CHECK(pr_u5e_direction_gate_17f6(ram, sizeof ram, NULL, &layout, 10u) == PR_U5E_263D_ARGUMENT);
    CHECK(pr_u5e_direction_gate_17f6(ram, sizeof ram, &c, NULL, 10u) == PR_U5E_263D_ARGUMENT);
    CHECK(pr_u5e_direction_gate_17f6(ram, sizeof ram, &c, &layout, 0u) == PR_U5E_263D_ARGUMENT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    c.sp = 49u;
    original = c;
    CHECK(pr_u5e_direction_gate_17f6(ram, sizeof ram, &c, &layout, 10u) == PR_U5E_263D_ARGUMENT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    c = setup(10u, 10u);
    c.ss = c.cs;
    c.sp = 0x1810u;
    original = c;
    memcpy(before, ram, sizeof ram);
    CHECK(pr_u5e_direction_gate_17f6(ram, sizeof ram, &c, &layout, 10u) == PR_U5E_263D_CODE_ALIAS);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    if (results() || limit() || rejects()) return 1;
    puts("U5E +17F6: counter, signed remainder, nested limit and rejection checks passed");
    return 0;
}
