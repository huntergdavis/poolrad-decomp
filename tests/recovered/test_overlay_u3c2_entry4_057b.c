#include "poolrad/recovered/overlay_u3c2_entry4_057b.h"

#include <stdio.h>
#include <string.h>

#define CHECK(e) do { if (!(e)) { \
    fprintf(stderr, "U3C2 +057B line %d: %s\n", __LINE__, #e); return 1; \
} } while (0)

static uint8_t ram[PR_U3C2_057B_RAM_SIZE], before[PR_U3C2_057B_RAM_SIZE];

static void word(unsigned offset, uint16_t value)
{
    ram[0x20000u + offset] = (uint8_t)value;
    ram[0x20001u + offset] = (uint8_t)(value >> 8);
}

static pr_u3c2_057b_cpu setup(uint8_t mode, uint8_t target_x)
{
    pr_u3c2_057b_cpu c = {
        0xA123u, 0x2345u, 0x3456u, 0x4567u, 0x5678u, 0x6789u,
        0x7777u, 0x8000u, 0x5000u, 0x057Bu, 0x1000u, 0x3333u,
        0x2000u, 0x7202u
    };
    memset(ram, 0xA5, sizeof ram);
    memset(ram + 0x12736, 0, 265u); /* synthetic, overlapping table windows */
    word(0x8000, 0x7000);
    word(0x8002, 0x3000);
    word(0x8004, (uint16_t)(0xAB00u | mode));
    word(0x8006, 20); /* target Y */
    word(0x8008, target_x);
    word(0x800A, 10); /* origin Y */
    word(0x800C, 10); /* origin X */
    return c;
}

static int modes(void)
{
    unsigned mode;
    for (mode = 9; mode < 255; ++mode) {
        pr_u3c2_057b_cpu c = setup((uint8_t)mode, 20);
        uint8_t inherited = (uint8_t)(mode ^ 0x5Au);
        ram[0x27FFD] = inherited; /* local result is intentionally not initialized */
        CHECK(pr_u3c2_entry4_057b(ram, sizeof ram, &c) == PR_U3C2_057B_OK);
        CHECK((uint8_t)c.ax == inherited && ram[0x27FFD] == inherited);
        CHECK(ram[0x28004] == mode && ram[0x28005] == 0xABu);
        CHECK(c.bp == 0x7777u && c.sp == 0x800Eu && c.cs == 0x3000u && c.ip == 0x7000u);
        CHECK(c.bx == 0x2345u && c.si == 0x5678u && c.es == 0x3333u);
    }
    {
        pr_u3c2_057b_cpu c = setup(255, 20);
        CHECK(pr_u3c2_entry4_057b(ram, sizeof ram, &c) == PR_U3C2_057B_OK);
        CHECK((uint8_t)c.ax == 1u && ram[0x28004] == 8u && ram[0x28005] == 0xABu);
    }
    {
        pr_u3c2_057b_cpu c = setup(255, 50);
        CHECK(pr_u3c2_entry4_057b(ram, sizeof ram, &c) == PR_U3C2_057B_OK);
        CHECK(c.ax == 0xA100u && ram[0x28004] == 255u);
        CHECK(ram[0x27FFC] == 0xA5u && ram[0x27FFB] == 0xA5u);
    }
    {
        pr_u3c2_057b_cpu c = setup(100, 10);
        word(0x8006, 10); /* equality supersedes the unrecognized mode */
        CHECK(pr_u3c2_entry4_057b(ram, sizeof ram, &c) == PR_U3C2_057B_OK);
        CHECK((uint8_t)c.ax == 1u);
    }
    return 0;
}

static int rejects(void)
{
    pr_u3c2_057b_cpu c = setup(0, 20), original = c;
    memcpy(before, ram, sizeof ram);
    CHECK(pr_u3c2_entry4_057b(NULL, sizeof ram, &c) == PR_U3C2_057B_REJECT);
    CHECK(pr_u3c2_entry4_057b(ram, sizeof ram - 1u, &c) == PR_U3C2_057B_REJECT);
    CHECK(pr_u3c2_entry4_057b(ram, sizeof ram, NULL) == PR_U3C2_057B_REJECT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    c.ip = 0;
    original = c;
    CHECK(pr_u3c2_entry4_057b(ram, sizeof ram, &c) == PR_U3C2_057B_REJECT);
    CHECK(memcmp(&c, &original, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    if (modes() || rejects()) return 1;
    puts("U3C2 +057B: inherited result, FF rewrite, bounds and rejection checks passed");
    return 0;
}
