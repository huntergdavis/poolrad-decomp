#include "poolrad/recovered/overlay_u3c2_entry5_0422.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE PR_U3C2_0422_RAM_SIZE

static void word(uint8_t *ram, size_t p, uint16_t v)
{
    ram[p] = (uint8_t)v;
    ram[(p + 1u) & (SIZE - 1u)] = (uint8_t)(v >> 8);
}

static uint16_t get(const uint8_t *ram, size_t p)
{
    return (uint16_t)(ram[p] | ((uint16_t)ram[p + 1u] << 8));
}

static pr_u3c2_0422_cpu fixture(uint8_t *ram, unsigned radius)
{
    const uint16_t frame[] = {0x7000, 0x3000, 0x100, 0x6000, 0x100, 0x5000,
                             0x102, 0x5000, 0x104, 0x5000, 0, 0};
    pr_u3c2_0422_cpu c = {0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
                          0x7777, 0x8000, 0x3000, 0x422, 0x1000, 0x8000,
                          0x2000, 0x7202};
    memset(ram, 0xA5, SIZE);
    memset(ram + 0x60000, 0, 65536u);
    ram[0x60106] = 1;
    ram[0x12745] = 0;
    ram[0x12746] = 0;
    for (unsigned i = 0; i < 9u; ++i) ram[0x125C0 + i] = (uint8_t)(23u * i);
    word(ram, 0x50100, (uint16_t)radius);
    ram[0x50102] = 3;
    ram[0x50104] = 7;
    for (unsigned i = 0; i < 12u; ++i) word(ram, 0x28000 + i * 2u, frame[i]);
    return c;
}

static void returned(const pr_u3c2_0422_cpu *c, unsigned result)
{
    assert(c->ax == result);
    assert(c->cs == 0x3000 && c->ip == 0x7000);
    assert(c->sp == 0x8018 && c->bp == 0x7777);
    assert(c->ss == 0x2000 && c->ds == 0x1000 && c->bx == 0x1357);
    assert(c->flags == 0x7246);
}

static void paths(uint8_t *ram)
{
    pr_u3c2_0422_cpu c = fixture(ram, 100);
    assert(pr_u3c2_line_check_0422(ram, SIZE, &c, 512) == PR_U3C2_0422_OK);
    returned(&c, 1);
    assert(get(ram, 0x50100) == 17);
    assert(ram[0x50102] == 3 && ram[0x50104] == 7);

    c = fixture(ram, 0);
    assert(pr_u3c2_line_check_0422(ram, SIZE, &c, 512) == PR_U3C2_0422_OK);
    returned(&c, 0);
    assert(get(ram, 0x50100) == 2);
    assert(ram[0x50102] == 0 && ram[0x50104] == 1);

    c = fixture(ram, 100);
    ram[0x60106] = 0;
    ram[0x12746] = 1;
    assert(pr_u3c2_line_check_0422(ram, SIZE, &c, 512) == PR_U3C2_0422_OK);
    returned(&c, 0);
    assert(get(ram, 0x50100) == 0);
    assert(ram[0x50102] == 0 && ram[0x50104] == 0);

    /* The final count write changes the actual outer return IP. */
    c = fixture(ram, 100);
    word(ram, 0x28008, 0x8000);
    word(ram, 0x2800A, 0x2000);
    assert(pr_u3c2_line_check_0422(ram, SIZE, &c, 512) == PR_U3C2_0422_OK);
    assert(c.ip == 17 && c.cs == 0x3000 && c.sp == 0x8018);

    for (unsigned budget = 0; budget < 2u; ++budget) {
        c = fixture(ram, 100);
        assert(pr_u3c2_line_check_0422(ram, SIZE, &c, budget) == PR_U3C2_0422_LIMIT);
        assert(c.ip == 0x4DD && c.cs == 0x3000 && c.sp == 0x7FC4 && c.bp == 0x7FFE);
        assert(get(ram, 0x50100) == 100);
        assert(get(ram, 0x27FF7) == budget); /* current x */
        assert(get(ram, 0x27FF9) == 0);      /* current y */
        assert(ram[0x27FFB] == budget * 2u);
    }
}

static void rejected(uint8_t *ram, uint8_t *copy)
{
    pr_u3c2_0422_cpu c = fixture(ram, 100), before;
    memcpy(copy, ram, SIZE);
    assert(pr_u3c2_line_check_0422(NULL, SIZE, &c, 1) == PR_U3C2_0422_ARGUMENT);
    assert(pr_u3c2_line_check_0422(ram, SIZE, NULL, 1) == PR_U3C2_0422_ARGUMENT);
    for (unsigned i = 0; i < 8u; ++i) {
        size_t size = SIZE;
        c = fixture(ram, 100);
        if (i == 0) size = SIZE - 1u;
        if (i == 1) size = SIZE + 1u;
        if (i == 2) c.ip = 0;
        if (i == 3) c.sp = 0x53;
        if (i == 4) c.sp = 0xFFE9;
        if (i >= 5) {
            c.cs = c.ss;
            c.sp = i == 5 ? 0x54 : i == 6 ? 0x1A1 : 0x57B;
        }
        before = c;
        assert(pr_u3c2_line_check_0422(ram, size, &c, 1) ==
               (i >= 5 ? PR_U3C2_0422_CODE_ALIAS : PR_U3C2_0422_ARGUMENT));
        assert(memcmp(&c, &before, sizeof(c)) == 0);
        assert(memcmp(ram, copy, SIZE) == 0);
    }
}

int main(void)
{
    uint8_t *ram = malloc(SIZE), *copy = malloc(SIZE);
    assert(ram && copy);
    paths(ram);
    rejected(ram, copy);
    free(copy);
    free(ram);
    puts("U3C2 +0422 full-chain paths, actual return alias, limits and atomic guards PASS");
    return 0;
}
