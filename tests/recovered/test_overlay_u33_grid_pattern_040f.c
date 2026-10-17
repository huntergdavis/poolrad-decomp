#include "poolrad/recovered/overlay_u33_grid_pattern_040f.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SIZE PR_U33_01AD_RAM_SIZE

static void word(uint8_t *ram, size_t p, uint16_t value)
{
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u33_040f_cpu fixture(uint8_t *ram, uint8_t category)
{
    pr_u33_040f_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                         0x7777, 0x8000, 0x3000, 0x40F, 0x1000, 0x1111,
                         0x2000, 0x7202};
    memset(ram, 0xA5, SIZE);
    memset(ram + 0x50100, 0, 0x400);
    memset(ram + 0x12736, 0, 0x10A);
    ram[0x184DC] = 5; ram[0x16AAE] = 3;
    word(ram, 0x16A5C, 0x100); word(ram, 0x16A5E, 0x5000);
    word(ram, 0x16716, 0x100); word(ram, 0x16718, 0x6000);
    word(ram, 0x28000, 0x7000); word(ram, 0x28002, 0x3000);
    word(ram, 0x28004, 0x9000);
    ram[0x28FF7] = 0; ram[0x28FF8] = category == 2u ? 2u : 0u;
    ram[0x28FF9] = category;
    ram[0x28FFC] = 4; ram[0x28FFD] = 3;
    ram[0x28FFE] = 0; ram[0x28FFF] = 0;
    /* Synthetic loaded-call metadata consumed by the C lookup adapter. */
    ram[0x30155] = 0x9A; word(ram, 0x30156, 0x2F); word(ram, 0x30158, 0x3D5);
    ram[0x30170] = 0x9A; word(ram, 0x30171, 0x2A); word(ram, 0x30173, 0x3D5);
    ram[0x3D7F] = 0xEA; word(ram, 0x3D80, 0x350); word(ram, 0x3D82, 0x4000);
    ram[0x3D7A] = 0xEA; word(ram, 0x3D7B, 0x47B); word(ram, 0x3D7D, 0x4000);
    return c;
}

static void paths(uint8_t *ram)
{
    const uint8_t expected[3][4] = {{23, 23, 23, 23}, {16, 6, 17, 11},
                                   {0xA6, 0xA6, 0xA6, 0xA6}};
    for (unsigned category = 0; category < 3u; ++category) {
        pr_u33_040f_cpu c = fixture(ram, (uint8_t)category);
        assert(pr_u33_grid_pattern_040f(ram, SIZE, &c) == PR_U33_040F_OK);
        assert(c.cs == 0x3000 && c.ip == 0x7000 && c.sp == 0x8006 && c.bp == 0x7777);
        assert(c.ss == 0x2000 && c.ds == 0x1000);
        assert(ram[0x60311] == expected[category][0]);
        assert(ram[0x60312] == expected[category][1]);
        assert(ram[0x60343] == expected[category][2]);
        assert(ram[0x60344] == expected[category][3]);
        assert(ram[0x60310] == 0xA5 && ram[0x60345] == 0xA5);
    }
    {
        pr_u33_040f_cpu c = fixture(ram, 1);
        /* Nonzero first pair skips the second pair and clears the flag. */
        ram[0x50133] = 0x11; ram[0x50233] = 0x11;
        ram[0x50433] = 0;
        assert(pr_u33_grid_pattern_040f(ram, SIZE, &c) == PR_U33_040F_OK);
        assert(ram[0x60311] == 6 && ram[0x60312] == 6);
        assert(ram[0x60343] == 11 && ram[0x60344] == 11);
    }
    {
        pr_u33_040f_cpu c = fixture(ram, 0);
        ram[0x28FFE] = 0x80;
        assert(pr_u33_grid_pattern_040f(ram, SIZE, &c) == PR_U33_040F_OK);
        for (unsigned i = 0; i < 0x600u; ++i) assert(ram[0x60100 + i] == 0xA5);
    }
    for (unsigned seam = 0xFFFEu; seam <= 0xFFFFu; ++seam) {
        pr_u33_040f_cpu c = fixture(ram, 2);
        c.sp = seam == 0xFFFEu ? 0xFFFAu : 0xFFF9u;
        word(ram, 0x20000u + c.sp, 0x7000);
        word(ram, 0x20002u + c.sp, 0x3000);
        word(ram, 0x20004u + c.sp, 0x9000);
        word(ram, 0x16716, (uint16_t)(c.sp - 0x259u));
        word(ram, 0x16718, 0x2000);
        ram[0x20000u + c.sp - 5u] = (uint8_t)(seam - 3u);
        word(ram, 0x20000u + (uint16_t)(seam + 4u), 0x9200);
        ram[0x291FE] = 0x80; ram[0x291FF] = 0;
        word(ram, 0x20000u + (uint16_t)(seam + 2u), 0x3333);
        ram[0x20000u + seam + 2u] = 0x66;
        ram[0x20000u + seam + 3u] = 0x77;
        assert(pr_u33_grid_pattern_040f(ram, SIZE, &c) == PR_U33_040F_OK);
        assert(c.cs == 0x7766 && c.sp == (uint16_t)(seam + 6u));
        assert(c.ip == (seam == 0xFFFEu ? 0x9000u : 0xA5A5u));
    }
    {
        pr_u33_040f_cpu c = fixture(ram, 0);
        ram[0x3D7F] = 0;
        assert(pr_u33_grid_pattern_040f(ram, SIZE, &c) == PR_U33_040F_CALLEE);
        assert(c.ip == 0x12B && c.sp < 0x8000);
    }
}

static void guards(uint8_t *ram, uint8_t *copy)
{
    pr_u33_040f_cpu c = fixture(ram, 0), before = c;
    memcpy(copy, ram, SIZE);
    assert(pr_u33_grid_pattern_040f(NULL, SIZE, &c) == PR_U33_040F_ARGUMENT);
    assert(memcmp(&c, &before, sizeof(c)) == 0);
    assert(pr_u33_grid_pattern_040f(ram, SIZE, NULL) == PR_U33_040F_ARGUMENT);
    for (unsigned i = 0; i < 8u; ++i) {
        size_t size = SIZE;
        c = fixture(ram, 0);
        if (i == 0) size = SIZE - 1u;
        if (i == 1) size = SIZE + 1u;
        if (i == 2) c.ip = 0;
        if (i == 3) c.sp = 0x7F;
        if (i == 4) c.sp = 0xFFFB;
        if (i >= 5) {
            c.cs = c.ss;
            c.sp = i == 5 ? 0x12C : i == 6 ? 0x222 : 0x410;
        }
        before = c;
        assert(pr_u33_grid_pattern_040f(ram, size, &c) ==
               (i >= 5 ? PR_U33_040F_CODE_ALIAS : PR_U33_040F_ARGUMENT));
        assert(memcmp(&c, &before, sizeof(c)) == 0);
        assert(memcmp(ram, copy, SIZE) == 0);
    }
}

int main(void)
{
    uint8_t *ram = malloc(SIZE), *copy = malloc(SIZE);
    assert(ram && copy);
    paths(ram); guards(ram, copy);
    free(copy); free(ram);
    puts("U33 +040F real lookup/write composition, stale glyphs, clipping and guards PASS");
    return 0;
}
