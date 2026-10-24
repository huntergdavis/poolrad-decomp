/* Focused synthetic test for GAME.OVR unit 3B6h +0000 (the map window) and unit 3CEh +00F1 (its tile placement)
 * (docs/re/OVL_03B6_0000_CPU.md, docs/re/OVL_03CE_00F1_CPU.md). No game bytes: only relocated far-call segment words,
 * the overlay stub entry and synthetic map, records, window-0 words, tile sets and object. */
#include "poolrad/recovered/overlay_u3b6_window_0000_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "03b6 window line %d: %s\n", __LINE__, #x);           \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, B6 = 0x5200, CE = 0x5000, STUB = 0x13E9, R0464 = 0x1464, SYS = 0x1AF8, DS0 = 0x1C7C,
       SS0 = 0x7000, SP0 = 0xF000, LOW = 0x3000, HIGH = 0x3100, OBJ = 0x4400, W0 = 0x5400 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* Map bytes 12h (tile 11h) everywhere, records off the map (0200h), window-0 words 0, 08D4 in mode 3 (the plane-byte path:
 * no Move), one-row one-byte tiles in both sets, the object at OBJ:0200. Unit 3B6h's stub-call segment word, the loaded
 * stub entry and 00F1's two 08D4 segment words are the relocated values. */
static void world(int loaded)
{
    unsigned k;
    memset(ram, 0x5A, RAM);
    put16(B6, 0x01E4u, STUB);
    ram[at(STUB, 0x002Au)] = loaded ? 0xEA : 0xCD;
    put16(STUB, 0x002Bu, loaded ? 0x00F1u : 0xF13Fu); put16(STUB, 0x002Du, loaded ? CE : 0x0000u);
    put16(CE, 0x0120u, R0464); put16(CE, 0x0148u, R0464);
    put16(R0464, 0x0B33u, SYS); put16(R0464, 0x0D2Du, SYS);
    for (k = 0; k < 0x2Cu * 0x10u; ++k)
        ram[at(DS0, (uint16_t)(0x3648u + k))] = 0x12;
    for (k = 1; k <= 9; ++k) {
        put16(DS0, (uint16_t)(0x0C30u + 6u * k), 0x0200u); put16(DS0, (uint16_t)(0x0C32u + 6u * k), 0x0200u);
        put16(DS0, (uint16_t)(0x0C34u + 6u * k), (uint16_t)(0x30u + k));
    }
    put16(DS0, 0x49D2u, 0x0000u); put16(DS0, 0x49D4u, W0);
    for (k = 0; k < 0x400u; k += 2)
        put16(W0, (uint16_t)k, 0);
    put16(DS0, 0x5E52u, 0x0100u); put16(DS0, 0x5E54u, LOW);
    put16(DS0, 0x5E56u, 0x0100u); put16(DS0, 0x5E58u, HIGH);
    put16(LOW, 0x0100u, 1); put16(LOW, 0x0102u, 1); put16(LOW, 0x0111u, 1); put16(LOW, 0x0113u, 0); put16(LOW, 0x0115u, 0);
    put16(HIGH, 0x0100u, 1); put16(HIGH, 0x0102u, 1); put16(HIGH, 0x0111u, 1); put16(HIGH, 0x0113u, 0); put16(HIGH, 0x0115u, 0);
    put16(DS0, 0x4A20u, 0x0200u); put16(DS0, 0x4A22u, OBJ); put16(OBJ, 0x0202u, 40);
    ram[at(DS0, 0x4A1Eu)] = 3;
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
}

static pr_ovl03b6_0000_regs window_call(uint16_t x0, uint16_t y0)
{
    pr_ovl03b6_0000_regs c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, B6, 0x0000u, DS0, 0x1111u, SS0, 0x0002u};
    put16(SS0, SP0 + 4u, y0); put16(SS0, SP0 + 6u, x0);
    return c;
}

int main(void)
{
    pr_ovl03b6_0000_env env;
    pr_ovl03b6_0000_diagnostic d;
    pr_ovl03b6_0000_regs c;
    pr_ovl03ce_00f1_env tenv;
    pr_ovl03ce_00f1_diagnostic td;
    pr_ovl03ce_00f1_regs t;
    pr_bus bus;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;
    memset(&env, 0, sizeof env);
    env.max_loopbacks = 1000; env.tile.max_loopbacks = 1000; tenv.max_loopbacks = 1000;
    bus.ram = ram; bus.ega = NULL; bus.ports = NULL; bus.on_vram_write = NULL; bus.ctx = NULL;

    /* 00F1 direct: tile 85h takes the DS:5E56 set with tile 05h; the 08D4 frame stays below SP: (object, set, flag 0,
     * tile, y, x). The flag word [BP+6] is not read. */
    world(1);
    {
        pr_ovl03ce_00f1_regs t0 = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, CE, 0x00F1u, DS0, 0x1111u, SS0, 0x0002u};
        t = t0;
    }
    put16(SS0, SP0 + 4u, 0xFFFFu); put16(SS0, SP0 + 6u, 0x0085u); put16(SS0, SP0 + 8u, 0x0003u); put16(SS0, SP0 + 10u, 0x0002u);
    CHECK(pr_ovl03ce_00f1_cpu(&bus, RAM, &t, &tenv, &td) == PR_OVL03CE_00F1_OK && td.calls == 1 && td.site == 0 && td.status == -1);
    CHECK(get16(SS0, SP0 - 4u) == 0x0002u && get16(SS0, SP0 - 6u) == 0x0003u && (get16(SS0, SP0 - 8u) & 0xFFu) == 0x05u);
    CHECK((get16(SS0, SP0 - 10u) & 0xFFu) == 0 && get16(SS0, SP0 - 12u) == HIGH && get16(SS0, SP0 - 14u) == 0x0100u);
    CHECK(get16(SS0, SP0 - 16u) == OBJ && get16(SS0, SP0 - 18u) == 0x0200u && get16(SS0, SP0 - 22u) == 0x0122u);
    CHECK(t.cs == 0x9001u && t.ip == 0x7000u && t.sp == SP0 + 12u && t.bp == 0x7777u);
    /* Tile 7Fh takes the DS:5E52 set unchanged, through the second call site. */
    t.cs = CE; t.ip = 0x00F1u; t.sp = SP0;
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u); put16(SS0, SP0 + 6u, 0x007Fu);
    CHECK(pr_ovl03ce_00f1_cpu(&bus, RAM, &t, &tenv, &td) == PR_OVL03CE_00F1_OK && td.calls == 1);
    CHECK((get16(SS0, SP0 - 8u) & 0xFFu) == 0x7Fu && get16(SS0, SP0 - 12u) == LOW && get16(SS0, SP0 - 22u) == 0x014Au);

    /* The window at (3, 2): 25 tile calls; the last cell is (x 7, y 6) at screen (12, 12). With map byte 90h there the
     * tile is 8Fh: 00F1 takes the high set with tile 0Fh. */
    world(1);
    ram[at(DS0, (uint16_t)(0x3648u + 6u * 0x2Cu + 7u))] = 0x90;
    c = window_call(3, 2);
    CHECK(pr_ovl03b6_0000_cpu(&bus, RAM, &c, &env, &d) == PR_OVL03B6_0000_OK && d.calls == 25 && d.site == 0 && d.status == -1);
    CHECK(d.tile.calls == 1 && get16(SS0, SP0 - 12u) == 12u && get16(SS0, SP0 - 14u) == 12u && (get16(SS0, SP0 - 16u) & 0xFFu) == 0x8Fu);
    CHECK((get16(SS0, SP0 - 18u) & 0xFFu) == 1u && (get16(SS0, SP0 - 30u) & 0xFFu) == 0x0Fu && get16(SS0, SP0 - 34u) == HIGH);
    CHECK(ram[at(SS0, SP0 - 3u)] == 15u && ram[at(SS0, SP0 - 4u)] == 15u && ram[at(SS0, SP0 - 8u)] == 0x8Fu);
    CHECK(c.cs == 0x9001u && c.ip == 0x7000u && c.sp == SP0 + 8u && c.bp == 0x7777u);

    /* Record 6 at (7, 6) replaces the tile with 56h - 1 = 55h (always); record 3 there with window-0 +318h = 00FFh does not. */
    world(1);
    put16(DS0, 0x0C30u + 36u, 7); put16(DS0, 0x0C32u + 36u, 6); put16(DS0, 0x0C34u + 36u, 0x0056u);
    c = window_call(3, 2);
    CHECK(pr_ovl03b6_0000_cpu(&bus, RAM, &c, &env, &d) == PR_OVL03B6_0000_OK && (get16(SS0, SP0 - 16u) & 0xFFu) == 0x55u);
    CHECK(get16(SS0, SP0 - 34u) == LOW && (get16(SS0, SP0 - 30u) & 0xFFu) == 0x55u);
    world(1);
    put16(DS0, 0x0C30u + 18u, 7); put16(DS0, 0x0C32u + 18u, 6); put16(DS0, 0x0C34u + 18u, 0x0056u); put16(W0, 0x0318u, 0x00FFu);
    c = window_call(3, 2);
    CHECK(pr_ovl03b6_0000_cpu(&bus, RAM, &c, &env, &d) == PR_OVL03B6_0000_OK && (get16(SS0, SP0 - 16u) & 0xFFu) == 0x11u);

    /* An unloaded stub (INT 3Fh) stops with OVERLAY at 03E9:002A with the first call's frame pushed. */
    world(0);
    c = window_call(3, 2);
    CHECK(pr_ovl03b6_0000_cpu(&bus, RAM, &c, &env, &d) == PR_OVL03B6_0000_OVERLAY && d.site == 0x01E1u && d.calls == 1);
    CHECK(c.cs == STUB && c.ip == 0x002Au && get16(SS0, c.sp) == 0x01E6u && get16(SS0, (uint16_t)(c.sp + 2u)) == B6);

    /* max_loopbacks 0: the first backward jump is the record loop's JMP at 01C9, before any tile call. */
    world(1);
    c = window_call(3, 2);
    env.max_loopbacks = 0;
    CHECK(pr_ovl03b6_0000_cpu(&bus, RAM, &c, &env, &d) == PR_OVL03B6_0000_LIMIT && d.calls == 0 && c.ip == 0x01C9u);
    env.max_loopbacks = 1000;

    /* 08D4's own LIMIT (its loopback cap 0) stops 00F1 with CALLEE, and 03B6 with CALLEE at its first tile call. */
    world(1);
    c = window_call(3, 2);
    put16(LOW, 0x0100u, 2); ram[at(DS0, 0x4A1Eu)] = 0;   /* mode 0, two rows: 08D4's row loop jumps back once */
    env.tile.max_loopbacks = 0;
    CHECK(pr_ovl03b6_0000_cpu(&bus, RAM, &c, &env, &d) == PR_OVL03B6_0000_CALLEE && d.site == 0x01E1u && d.calls == 1 &&
          d.status == PR_OVL03CE_00F1_CALLEE && d.tile.site == 0x0145u && d.tile.status == PR_R0464_COMPOSE_CPU_LIMIT);
    env.tile.max_loopbacks = 1000;

    /* Bad arguments change nothing. */
    world(1);
    c = window_call(3, 2);
    CHECK(pr_ovl03b6_0000_cpu(&bus, RAM - 1u, &c, &env, &d) == PR_OVL03B6_0000_ARGUMENT && c.ip == 0 && c.sp == SP0);
    CHECK(pr_ovl03b6_0000_cpu(&bus, RAM, &c, NULL, &d) == PR_OVL03B6_0000_ARGUMENT && c.cs == B6);
    t.ip = 0x00F0u;
    CHECK(pr_ovl03ce_00f1_cpu(&bus, RAM, &t, &tenv, &td) == PR_OVL03CE_00F1_ARGUMENT && t.ip == 0x00F0u);

    free(ram);
    printf("03b6 window: %u checks\n", checks);
    return 0;
}
