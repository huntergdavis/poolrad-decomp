/* Focused synthetic test for GAME.OVR unit 38h +09C0, the 009E slot-28 handler (docs/re/OVL_0038_09C0_CPU.md). No game
 * bytes: only the four relocated far-call segment words, the loaded-stub forms of unit 9E entries 8 and 2 and the dice
 * chain's fixture (the +0E0E call site, the SYSTEM multiplier word and the seed), a synthetic source string at CS:09B2,
 * and synthetic records and DS bytes. */
#include "poolrad/recovered/overlay_u38_handler_09c0_cpu.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 09c0 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, OV = 0x5000, SYS = 0x0AF8, STUB = 0x00B0, RES = 0x10BA, U9E = 0x6000, DS0 = 0x1C7C,
       SS0 = 0x7000, SP0 = 0xF000, ASEG = 0x3000, AOFF = 0x0100, BSEG = 0x3400, BOFF = 0x0200 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* The frame, the fixup words, the roll stub and its dice fixture (DICE_CS, seed), the +0026 stub, a 5-character source
 * string at CS:09B2, record A's byte +3, the DS bytes 681B/6820 and the helper's 6817/6818. */
static pr_u38_09c0_cpu world(uint8_t a3, uint8_t b681b, uint8_t b6820, uint32_t seed)
{
    pr_u38_09c0_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, OV, 0x09C0u, DS0, 0x1111u, SS0, 0x0002u};
    memset(ram, 0x5A, RAM);
    dice_site(ram, OV, 0x09D4u, 0x48u, 0x0DE3u);           /* 9A 48 00 B0 00 at CS:09D4, the stub, U9E +0E0E's call site */
    dice_system(ram, DS0, seed);                            /* SYSTEM multiplier word and DS:4428 seed */
    put16(OV, 0x0A05u, SYS); put16(OV, 0x0A10u, RES); put16(OV, 0x0A35u, STUB);
    ram[at(STUB, 0x002Au)] = 0xEA; put16(STUB, 0x002Bu, 0x0026u); put16(STUB, 0x002Du, U9E);
    memcpy(ram + at(OV, 0x09B2u), "\x05HELLO", 6);
    ram[at(ASEG, AOFF + 3u)] = a3;
    ram[at(DS0, 0x681Bu)] = b681b; ram[at(DS0, 0x6820u)] = b6820;
    ram[at(DS0, 0x6817u)] = 0x33; ram[at(DS0, 0x6818u)] = 0x44;
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, 0x5A5Au); put16(SS0, SP0 + 6u, AOFF); put16(SS0, SP0 + 8u, ASEG);
    put16(SS0, SP0 + 10u, BOFF); put16(SS0, SP0 + 12u, BSEG);
    return c;
}

static void returned(const pr_u38_09c0_cpu *c)
{
    CHECK(c->cs == 0x9001u && c->ip == 0x7000u && c->sp == SP0 + 14u && c->bp == 0x7777u && c->ds == DS0);
}

/* The DS bytes the helper clears and the buffer are untouched after an early return. */
static void untouched(void)
{
    CHECK(ram[at(DS0, 0x6817u)] == 0x33 && ram[at(DS0, 0x6818u)] == 0x44 && ram[at(SS0, SP0 - 16u)] == 0x5A);
}

int main(void)
{
    pr_u38_09c0_cpu c, before;
    pr_u38_09c0_limits lim = {256, {64, 64}};
    pr_u38_09c0_diagnostic d;
    uint32_t seed, s;
    unsigned i, low = 0, high = 0;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* A+3 = 0: the range is 1, so the roll is 1 and the body returns at once (CMP AL,1; JBE). The seed still advances. */
    c = world(0, 0x05, 0, 0x12345678u);
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_OK);
    CHECK(d.callee == 1 && d.callee_status == PR_U9E_ENTRY8_COMPOSE_RETURNED && d.site == 0x09D4u && (c.ax & 0xFFu) == 1u);
    CHECK(dice_seed(ram, DS0) == 0x12345678u * 0x08088405u + 1u && get16(SS0, SP0 - 18u) == 0x1201u && get16(SS0, SP0 - 20u) == 0x0001u);
    returned(&c); untouched();
    /* A+3 = FFh: the range byte wraps to 0, the generator yields 0 and the roll is 1: return. */
    c = world(0xFF, 0x05, 0, 0x12345678u);
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_OK && (c.ax & 0xFFu) == 1u && get16(SS0, SP0 - 20u) == 0x0100u);
    returned(&c); untouched();

    /* A+3 = 63h: a 1d100. Over 64 seeds both outcomes occur; a roll above 1 with DS:681B = 0 returns at the second test. */
    for (seed = 1; seed <= 64; ++seed) {
        s = seed * 0x9E3779B9u;
        c = world(0x63, 0, 0, s);
        CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_OK && d.callee == 1);
        CHECK((c.ax & 0xFFu) == dice_roll(&s, 1, 100));
        returned(&c); untouched();
        if ((c.ax & 0xFFu) <= 1u) ++low; else ++high;
    }
    CHECK(low > 0 && high > 0);

    /* Roll above 1, DS:681B nonzero, DS:6820 nonzero: the third test returns. */
    for (seed = 1; seed <= 64; ++seed) {
        s = seed * 0x9E3779B9u;
        if (dice_roll(&s, 1, 100) <= 1u) continue;
        c = world(0x63, 0x0F, 0x01, seed * 0x9E3779B9u);
        CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_OK && (c.ax & 0xFFu) > 1u);
        returned(&c); untouched();
    }

    /* Roll above 1, DS:681B nonzero, DS:6820 zero: helper(0) clears 6817/6818, StrLoad fills the buffer, and the body stops
     * PENDING at +0A0D before 00BA:16EC with the six words (B, SS:BP-0Eh, AH:0Ah, AH:01h) pushed and nothing for the call. */
    for (seed = 1; seed <= 64; ++seed) {
        s = seed * 0x9E3779B9u;
        if (dice_roll(&s, 1, 100) <= 1u) continue;
        c = world(0x63, 0x0F, 0, seed * 0x9E3779B9u);
        CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_PENDING);
        CHECK(d.callee == 4 && d.site == 0x0A0Du && c.cs == OV && c.ip == 0x0A0Du && c.bp == SP0 - 2u && c.sp == SP0 - 28u);
        CHECK((get16(SS0, SP0 - 28u) & 0xFFu) == 0x01u && (get16(SS0, SP0 - 26u) & 0xFFu) == 0x0Au);
        CHECK(get16(SS0, SP0 - 24u) == SP0 - 16u && get16(SS0, SP0 - 22u) == SS0 && get16(SS0, SP0 - 20u) == BOFF && get16(SS0, SP0 - 18u) == BSEG);
        CHECK(memcmp(ram + at(SS0, SP0 - 16u), "\x05HELLO", 6) == 0 && ram[at(SS0, SP0 - 10u)] == 0x5A);
        CHECK(ram[at(DS0, 0x6817u)] == 0 && ram[at(DS0, 0x6818u)] == 0 && ram[at(ASEG, AOFF + 3u)] == 0x63);
        CHECK(c.es == SS0 && c.di == SP0 - 10u && c.cx == 0 && c.ds == DS0 && (c.ax & 0xFFu) == 1u);
        CHECK(get16(SS0, SP0 - 30u) == OV && get16(SS0, SP0 - 32u) == 0x0A07u);   /* StrLoad's popped return frame, left below SP */
        break;
    }
    CHECK(seed <= 64);

    /* The roll stub is not a loaded far JMP to +0DE3: CODE at +09D4 with the two words pushed and nothing for the call. */
    c = world(0x63, 0x0F, 0, 7u); ram[at(STUB, 0x0048u)] = 0xCD;
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_CODE);
    CHECK(d.site == 0x09D4u && d.callee == -1 && c.cs == OV && c.ip == 0x09D4u && c.sp == SP0 - 20u);
    CHECK(dice_seed(ram, DS0) == 7u);
    c = world(0x63, 0x0F, 0, 7u); put16(STUB, 0x0049u, 0x0DE4u);
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_CODE && c.ip == 0x09D4u);

    /* A zero SYSTEM-call cap stops the roll at its nested call: CALLEE with the roll's LIMIT, at the roll's segment. */
    c = world(0x63, 0x0F, 0, 7u); lim.roll_system_calls = 0;
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_CALLEE);
    CHECK(d.callee == 1 && d.callee_status == PR_U9E_ENTRY8_COMPOSE_LIMIT && d.site == 0x09D4u && c.cs == DICE_CS && c.ip == 0x0E0Eu);
    lim.roll_system_calls = 256;

    /* The live AH travels with the count word: entry AH 12h gives 1201h at [SP-18]. (Checked in the first case.) The frame's
     * far pointer A is reread at each use: an A offset that wraps at 16 bits. */
    c = world(0, 0x05, 0, 1u); put16(SS0, SP0 + 6u, 0xFFFEu); put16(SS0, SP0 + 8u, ASEG + 0x0011u);
    ram[at(ASEG + 0x0011u, 0x0001u)] = 0;
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_OK && get16(SS0, SP0 - 20u) == 0x0001u);
    returned(&c);
    /* The same without a diagnostic. */
    c = world(0, 0x05, 0, 1u);
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, NULL) == PR_U38_09C0_OK);
    returned(&c);

    /* Arguments: nothing changes. */
    c = world(0, 0x05, 0, 1u); before = c;
    CHECK(pr_u38_handler_09c0_cpu(NULL, RAM, &c, &lim, &d) == PR_U38_09C0_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM - 1u, &c, &lim, &d) == PR_U38_09C0_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, NULL, &lim, &d) == PR_U38_09C0_ARGUMENT);
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, NULL, &d) == PR_U38_09C0_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    c.ip = 0x09C1u; before = c;
    CHECK(pr_u38_handler_09c0_cpu(ram, RAM, &c, &lim, &d) == PR_U38_09C0_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    for (i = 0; i < RAM; ++i)
        if (ram[i] != 0x5A && !(i >= at(OV, 0x09B2u) && i < at(OV, 0x09B8u)) && !(i >= at(OV, 0x09D4u) && i < at(OV, 0x09D9u))
            && !(i >= at(OV, 0x0A05u) && i < at(OV, 0x0A07u)) && !(i >= at(OV, 0x0A10u) && i < at(OV, 0x0A12u))
            && !(i >= at(OV, 0x0A35u) && i < at(OV, 0x0A37u)) && !(i >= at(STUB, 0x0048u) && i < at(STUB, 0x004Du))
            && !(i >= at(STUB, 0x002Au) && i < at(STUB, 0x002Fu)) && !(i >= at(DICE_CS, 0x0E0Eu) && i < at(DICE_CS, 0x0E13u))
            && !(i >= at(SYS, 0x1245u) && i < at(SYS, 0x1247u)) && !(i >= at(DS0, 0x4428u) && i < at(DS0, 0x442Cu))
            && !(i >= at(ASEG, AOFF + 3u) && i < at(ASEG, AOFF + 4u)) && i != at(DS0, 0x681Bu) && i != at(DS0, 0x6820u)
            && i != at(DS0, 0x6817u) && i != at(DS0, 0x6818u) && !(i >= at(SS0, SP0) && i < at(SS0, SP0 + 14u)))
            break;
    CHECK(i == RAM);

    free(ram);
    printf("PASS u38 09c0: %u checks\n", checks);
    return 0;
}
