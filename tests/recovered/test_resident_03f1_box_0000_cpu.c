/* Focused synthetic test for START.EXE resident 03F1:0000, the tile box (docs/re/RES_03F1_BOX_0000_CPU.md). No game
 * bytes: only relocated far-call segment words, the overlay stub entry and synthetic tables, tile set and object. */
#include "poolrad/recovered/resident_03f1_box_0000_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "03f1 box line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, BOX = 0x13F1, STUB = 0x1428, OV = 0x5000, R0464 = 0x1464, SYS = 0x1AF8, R0899 = 0x1899, DS0 = 0x1C7C,
       SS0 = 0x7000, SP0 = 0xF000, TILE = 0x3400, DST = 0x4800 };

static uint8_t *ram;
static unsigned checks;
static pr_ega ega;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* Style 0 (tiles 114h corner, 115h vertical, 116h horizontal: band 4, items 14h..16h of 6 bytes each); the eight
 * 03F1 stub-call segment words, the loaded stub entry, 015D's eight fixup words; 1951 mode 3 returns at once. */
static pr_r03f1_box_regs world(uint8_t left, uint8_t top, uint8_t right, uint8_t bottom, int loaded)
{
    pr_r03f1_box_regs c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, BOX, 0x0000u, DS0, 0x1111u, SS0, 0x0002u};
    static const uint16_t box_sites[8] = {0x0038, 0x0052, 0x006C, 0x0086, 0x00B9, 0x00D2, 0x010C, 0x0124};
    static const uint16_t tile_sites[8][2] = {{0x01C8, R0464}, {0x01DA, SYS}, {0x01DF, SYS}, {0x01E4, R0899}, {0x01EC, R0899},
                                              {0x024C, R0464}, {0x028E, SYS}, {0x02A5, R0464}};
    unsigned k;
    memset(ram, 0x5A, RAM);
    for (k = 0; k < 8; ++k) {
        put16(BOX, (uint16_t)(box_sites[k] + 3u), STUB);
        put16(OV, tile_sites[k][0], tile_sites[k][1]);
    }
    ram[at(STUB, 0x002Au)] = loaded ? 0xEA : 0xCD;
    put16(STUB, 0x002Bu, loaded ? 0x015Du : 0x5D3Fu); put16(STUB, 0x002Du, loaded ? OV : 0x0001u);
    put16(DS0, 0x2722u + 8u, 0x0100u);
    put16(DS0, 0x5E3Au + 16u, 0x0100u); put16(DS0, 0x5E3Cu + 16u, TILE);
    put16(TILE, 0x0111u, 6);
    for (k = 0; k < 0x100u; ++k)
        ram[at(TILE, (uint16_t)(0x0117u + k))] = (uint8_t)k;
    put16(DS0, 0x5E4Eu, 0x0010u); put16(DS0, 0x5E50u, DST);
    ram[at(DS0, 0x4A1Eu)] = 3;
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, 0); put16(SS0, SP0 + 6u, bottom); put16(SS0, SP0 + 8u, right);
    put16(SS0, SP0 + 10u, top); put16(SS0, SP0 + 12u, left);
    return c;
}

int main(void)
{
    pr_r03f1_box_env env;
    pr_r03f1_box_diagnostic d;
    pr_r03f1_box_regs c;
    pr_bus bus;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;
    memset(&env, 0, sizeof env);
    env.max_loopbacks = 1000; env.tile.max_loopbacks = 1000;
    bus.ram = ram; bus.ega = NULL; bus.ports = NULL; bus.on_vram_write = NULL; bus.ctx = NULL;

    /* Box (2,1)-(3,2): edges adjusted to (1,0)-(4,3); 4 corners, columns 2..3 twice each, rows 1..2 twice each = 12 tile
     * calls. Every call copies its 6-byte tile to the object's +17h (DST:0027); the last is the vertical edge 115h (item 15h). */
    c = world(2, 1, 3, 2, 1);
    CHECK(pr_r03f1_box_0000_cpu(&bus, RAM, &c, &env, &d) == PR_R03F1_BOX_OK && d.calls == 12 && d.site == 0 && d.status == -1);
    CHECK(ram[at(DST, 0x0027u)] == 0x7E && ram[at(DST, 0x002Cu)] == 0x83 && ram[at(DST, 0x002Du)] == 0x5A);
    CHECK(ram[at(SS0, SP0 + 12u)] == 1 && ram[at(SS0, SP0 + 10u)] == 0 && ram[at(SS0, SP0 + 8u)] == 4 && ram[at(SS0, SP0 + 6u)] == 3);
    CHECK(get16(SS0, SP0 - 5u) == 0x0114u && ram[at(SS0, SP0 - 3u)] == 2 && ram[at(SS0, SP0 - 6u)] == 2);
    CHECK(c.cs == 0x9001u && c.ip == 0x7000u && c.sp == SP0 + 14u && c.bp == 0x7777u);

    /* An unloaded stub (INT 3Fh) stops with OVERLAY at 0428:002A with the first call's frame pushed. */
    c = world(2, 1, 3, 2, 0);
    CHECK(pr_r03f1_box_0000_cpu(&bus, RAM, &c, &env, &d) == PR_R03F1_BOX_OVERLAY && d.site == 0x0038u && d.calls == 1);
    CHECK(c.cs == STUB && c.ip == 0x002Au && get16(SS0, c.sp) == 0x003Du && get16(SS0, (uint16_t)(c.sp + 2u)) == BOX);

    /* max_loopbacks 0: the first backward JNE (00DD, after column 2's two calls) stops with LIMIT: 4 + 2 calls. */
    c = world(2, 1, 3, 2, 1);
    env.max_loopbacks = 0;
    CHECK(pr_r03f1_box_0000_cpu(&bus, RAM, &c, &env, &d) == PR_R03F1_BOX_LIMIT && d.calls == 6 && c.ip == 0x00DDu);
    env.max_loopbacks = 1000;

    /* 015D's CONTRACT (EGA attached, Move into A000) propagates from the first corner call. */
    c = world(2, 1, 3, 2, 1);
    put16(DS0, 0x5E50u, 0xA000u);
    pr_ega_reset(&ega); bus.ega = &ega;
    CHECK(pr_r03f1_box_0000_cpu(&bus, RAM, &c, &env, &d) == PR_R03F1_BOX_CONTRACT && d.site == 0x0038u &&
          d.status == PR_OVL040C_015D_CONTRACT && d.tile.site == 0x028Bu && c.ip == 0x1685u);
    bus.ega = NULL;

    /* Bad arguments change nothing. */
    c = world(2, 1, 3, 2, 1);
    CHECK(pr_r03f1_box_0000_cpu(&bus, RAM - 1u, &c, &env, &d) == PR_R03F1_BOX_ARGUMENT && c.ip == 0);
    CHECK(pr_r03f1_box_0000_cpu(&bus, RAM, &c, NULL, &d) == PR_R03F1_BOX_ARGUMENT && c.sp == SP0);

    free(ram);
    printf("03f1 box: %u checks\n", checks);
    return 0;
}
