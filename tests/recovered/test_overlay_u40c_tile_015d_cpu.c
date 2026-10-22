/* Focused synthetic test for GAME.OVR unit 40Ch +015D tile placement (docs/re/OVL_040C_015D_CPU.md). No game bytes: only
 * the eight relocated far-call segment words in the unit's code and synthetic tables, tile sets and objects. */
#include "poolrad/recovered/overlay_u40c_tile_015d_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u40c 015d line %d: %s\n", __LINE__, #x);             \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, OV = 0x5000, R0464 = 0x1464, SYS = 0x1AF8, R0899 = 0x1899, DS0 = 0x1C7C, SS0 = 0x7000, SP0 = 0xF000,
       TILE = 0x3200, DST = 0x4800 };

static uint8_t *ram;
static unsigned checks;
static pr_ega ega;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* Tile set for band 2 at TILE:0100 (tile size 6 at +11h, data at +17h); the Move/1951 object at DST:0010; 1951 mode 3
 * (returns at once). Arguments: [BP+6] 01h, [BP+8] flag, [BP+0Ah] number, [BP+0Ch] 3, [BP+0Eh] 2. */
static pr_ovl040c_015d_regs world(uint16_t number, uint16_t flag)
{
    pr_ovl040c_015d_regs c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, OV, 0x015Du, DS0, 0x1111u, SS0, 0x0002u};
    static const uint16_t sites[8][2] = {{0x01C8, R0464}, {0x01DA, SYS}, {0x01DF, SYS}, {0x01E4, R0899}, {0x01EC, R0899},
                                         {0x024C, R0464}, {0x028E, SYS}, {0x02A5, R0464}};
    static const uint16_t base[5] = {0x0001, 0x002E, 0x0074, 0x00BA, 0x0100};
    unsigned k;
    memset(ram, 0x5A, RAM);
    for (k = 0; k < 8; ++k)
        put16(OV, sites[k][0], sites[k][1]);
    for (k = 0; k < 5; ++k) {
        put16(DS0, (uint16_t)(0x2722u + 2u * k), base[k]);
        put16(DS0, (uint16_t)(0x5E3Au + 4u * k), 0); put16(DS0, (uint16_t)(0x5E3Cu + 4u * k), 0);
    }
    put16(DS0, 0x5E3Au + 8u, 0x0100u); put16(DS0, 0x5E3Cu + 8u, TILE);
    put16(TILE, 0x0111u, 6);
    for (k = 0; k < 0x40u; ++k)
        ram[at(TILE, (uint16_t)(0x0117u + k))] = (uint8_t)(0x80u + k);
    put16(DS0, 0x5E4Eu, 0x0010u); put16(DS0, 0x5E50u, DST);
    ram[at(DS0, 0x4A1Eu)] = 3;
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, 0x0001u); put16(SS0, SP0 + 6u, flag); put16(SS0, SP0 + 8u, number);
    put16(SS0, SP0 + 10u, 3); put16(SS0, SP0 + 12u, 2);
    return c;
}

int main(void)
{
    pr_ovl040c_015d_env env;
    pr_ovl040c_015d_diagnostic d;
    pr_ovl040c_015d_regs c;
    pr_bus bus;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;
    memset(&env, 0, sizeof env);
    env.max_loopbacks = 1000;
    bus.ram = ram; bus.ega = NULL; bus.ports = NULL; bus.on_vram_write = NULL; bus.ctx = NULL;

    /* 76h is band 2 (74h..B9h), item 76h - 74h = 2: Move copies tile bytes +17h + 2 * 6 .. +6 into the object's +17h. */
    c = world(0x0076u, 0);
    CHECK(pr_ovl040c_015d_cpu(&bus, RAM, &c, &env, &d) == PR_OVL040C_015D_OK && d.calls == 2 && d.site == 0);
    CHECK(memcmp(&ram[at(DST, 0x0027u)], &ram[at(TILE, 0x0123u)], 6) == 0 && ram[at(DST, 0x0027u)] == 0x8C && ram[at(DST, 0x002Du)] == 0x5A);
    CHECK(ram[at(SS0, SP0 - 3u)] == 2 && get16(SS0, SP0 - 8u) == 0x0100u && get16(SS0, SP0 - 6u) == TILE && get16(SS0, SP0 + 8u) == 2);
    CHECK(c.cs == 0x9001u && c.ip == 0x7000u && c.sp == SP0 + 14u && c.bp == 0x7777u && c.ds == DS0);

    /* A null band pointer returns with no call; the number is not rebased. */
    c = world(0x0010u, 0);
    CHECK(pr_ovl040c_015d_cpu(&bus, RAM, &c, &env, &d) == PR_OVL040C_015D_OK && d.calls == 0 && get16(SS0, SP0 + 8u) == 0x10u);
    CHECK(ram[at(SS0, SP0 - 3u)] == 0 && c.sp == SP0 + 14u && ram[at(DST, 0x0017u)] == 0x5A);

    /* 8000h matches no band test and keeps the stale [BP-1] byte (here 2): band 2, item 8000h - 74h, so the source row is
     * at +17h + (7F8Ch * 6 mod 10000h) = +17h + FD48h; Move copies 6 bytes from TILE:FE5F. */
    c = world(0x8000u, 0);
    ram[at(SS0, SP0 - 3u)] = 2;
    ram[at(TILE, 0xFE5Fu)] = 0x11; ram[at(TILE, 0xFE64u)] = 0x22;
    CHECK(pr_ovl040c_015d_cpu(&bus, RAM, &c, &env, &d) == PR_OVL040C_015D_OK && d.calls == 2);
    CHECK(ram[at(DST, 0x0027u)] == 0x11 && ram[at(DST, 0x002Cu)] == 0x22 && get16(SS0, SP0 + 8u) == 0x7F8Cu);

    /* With an EGA attached, the error branch stops at 00FF's entry with CONTRACT (its callees are flat-RAM models). */
    pr_ega_reset(&ega); bus.ega = &ega;
    c = world(0x0000u, 0);
    CHECK(pr_ovl040c_015d_cpu(&bus, RAM, &c, &env, &d) == PR_OVL040C_015D_CONTRACT && d.site == 0x01C5u && d.calls == 1);
    CHECK(c.cs == R0464 && c.ip == 0x00FFu && get16(SS0, c.sp) == 0x01CAu && get16(SS0, (uint16_t)(c.sp + 2u)) == OV);

    /* ... and a Move destination in A0000..AFFFF stops at Move's entry. */
    c = world(0x0076u, 0);
    put16(DS0, 0x5E50u, 0xA000u);
    CHECK(pr_ovl040c_015d_cpu(&bus, RAM, &c, &env, &d) == PR_OVL040C_015D_CONTRACT && d.site == 0x028Bu);
    CHECK(c.cs == SYS && c.ip == 0x1685u && get16(SS0, c.sp) == 0x0290u && get16(SS0, (uint16_t)(c.sp + 4u)) == 6);
    bus.ega = NULL;

    /* Bad arguments change nothing. */
    c = world(0x0076u, 0);
    CHECK(pr_ovl040c_015d_cpu(&bus, RAM - 1u, &c, &env, &d) == PR_OVL040C_015D_ARGUMENT && c.ip == 0x015Du);
    c.ip = 0x015Eu;
    CHECK(pr_ovl040c_015d_cpu(&bus, RAM, &c, &env, &d) == PR_OVL040C_015D_ARGUMENT && c.sp == SP0);
    CHECK(pr_ovl040c_015d_cpu(&bus, RAM, &c, NULL, &d) == PR_OVL040C_015D_ARGUMENT);

    free(ram);
    printf("u40c 015d: %u checks\n", checks);
    return 0;
}
