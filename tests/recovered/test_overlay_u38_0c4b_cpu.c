/* Focused synthetic test for GAME.OVR unit 38h +0C4B, the 009E slot-39 handler (docs/re/OVL_0038_0C4B_CPU.md). No game
 * bytes: only the two relocated far-call segment words, a synthetic 4-character source string at CS:0C46, a synthetic
 * record A and the DS byte 681A. The whole-RAM comparisons prove that each path writes nothing but its declared bytes. */
#include "poolrad/recovered/overlay_u38_0c4b_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 0c4b line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, OV = 0x5000, SYS = 0x0AF8, RES = 0x10BA, DS0 = 0x1C7C, SS0 = 0x7000, SP0 = 0xF000,
       ASEG = 0x3000, AOFF = 0x0100, BSEG = 0x3400, BOFF = 0x0200 };

static uint8_t *ram, *snap;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* The frame, the two fixup words, a 4-character source string at CS:0C46, record A's byte +3 and DS:681A. Record B is
 * never read on any completed path (its +30h is incremented only after the 16EC stop), so its bytes stay the fill. */
static pr_u38_0c4b_cpu world(uint8_t a3, uint8_t b681a)
{
    pr_u38_0c4b_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, OV, 0x0C4Bu, DS0, 0x1111u, SS0, 0x0002u};
    memset(ram, 0x5A, RAM);
    put16(OV, 0x0C84u, SYS); put16(OV, 0x0C8Fu, RES);
    memcpy(ram + at(OV, 0x0C46u), "\x04WXYZ", 5);
    ram[at(ASEG, AOFF + 3u)] = a3;
    ram[at(DS0, 0x681Au)] = b681a;
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, 0x5A5Au); put16(SS0, SP0 + 6u, AOFF); put16(SS0, SP0 + 8u, ASEG);
    put16(SS0, SP0 + 10u, BOFF); put16(SS0, SP0 + 12u, BSEG);
    memcpy(snap, ram, RAM);
    return c;
}

/* Every RAM byte outside [lo, hi) of SS0 and the listed single bytes is as the world left it. */
static int only_changed(size_t lo, size_t hi, size_t x1, size_t x2)
{
    size_t i;
    for (i = 0; i < RAM; ++i)
        if (ram[i] != snap[i] && !(i >= lo && i < hi) && i != x1 && i != x2)
            return 0;
    return 1;
}

static void returned(const pr_u38_0c4b_cpu *c)
{
    CHECK(c->cs == 0x9001u && c->ip == 0x7000u && c->sp == SP0 + 14u && c->bp == 0x7777u && c->ds == DS0 && c->ss == SS0);
}

int main(void)
{
    static const uint8_t bytes[6] = {0x00, 0x05, 0x07, 0x5A, 0x80, 0xFF};
    static const uint16_t flags_after[6] = {0x0056u, 0x0016u, 0x0012u, 0x0016u, 0x0016u, 0x0012u};
    pr_u38_0c4b_cpu c, before;
    pr_u38_0c4b_diagnostic d;
    unsigned i;
    ram = malloc(RAM); snap = malloc(RAM);
    if (ram == NULL || snap == NULL)
        return 1;

    /* A+3 bit 4 set: the body skips to +0C98, doubles DS:681A (the byte's top bit is lost: 80h -> 00h, FFh -> FEh) and
     * returns. FLAGS after SHL AX,1 on the zero-extended byte: CF = OF = SF = 0, AF = 1 (pinned), ZF only for 0, PF of the
     * low result byte (0 -> 00h even, 05h -> 0Ah even, 07h -> 0Eh odd, 5Ah -> B4h even, 80h -> 00h even, FFh -> FEh odd),
     * bit 1 kept. Only [SP-2] (the saved BP) and DS:681A change in RAM. */
    for (i = 0; i < 6; ++i) {
        c = world(0x10u | (uint8_t)i, bytes[i]);
        CHECK(pr_u38_handler_0c4b_cpu(ram, RAM, &c, &d) == PR_U38_0C4B_OK);
        CHECK(d.callee == -1 && d.callee_status == -1 && d.site == 0);
        CHECK(ram[at(DS0, 0x681Au)] == (uint8_t)(bytes[i] << 1) && c.ax == (uint16_t)(bytes[i] << 1));
        CHECK(c.flags == flags_after[i] && c.es == ASEG && c.di == AOFF);
        CHECK(ram[at(ASEG, AOFF + 3u)] == (0x10u | i) && get16(SS0, SP0 - 2u) == 0x7777u);
        CHECK(only_changed(at(SS0, SP0 - 2u), at(SS0, SP0), at(DS0, 0x681Au), at(DS0, 0x681Au)));
        returned(&c);
    }
    /* The same with a far pointer A whose offset wraps at 16 bits (ES:FFFE + 3 = ES:0001). */
    c = world(0, 0x05); put16(SS0, SP0 + 6u, 0xFFFEu); put16(SS0, SP0 + 8u, ASEG + 0x0011u);
    ram[at(ASEG + 0x0011u, 0x0001u)] = 0x90; memcpy(snap, ram, RAM);
    CHECK(pr_u38_handler_0c4b_cpu(ram, RAM, &c, &d) == PR_U38_0C4B_OK && ram[at(DS0, 0x681Au)] == 0x0A && c.flags == 0x0016u);
    CHECK(only_changed(at(SS0, SP0 - 2u), at(SS0, SP0), at(DS0, 0x681Au), at(DS0, 0x681Au)));
    returned(&c);
    /* The same without a diagnostic. */
    c = world(0x1F, 0x05);
    CHECK(pr_u38_handler_0c4b_cpu(ram, RAM, &c, NULL) == PR_U38_0C4B_OK && ram[at(DS0, 0x681Au)] == 0x0A);
    returned(&c);

    /* A+3 bit 4 clear: A+3 += 10h (EFh -> FFh without carry; 00h -> 10h; 0Fh -> 1Fh), B and SS:BP-5 and CS:0C46 pushed,
     * StrLoad copies the 5 string bytes into [BP-5, BP), then AH:0Ah and AH:01h are pushed and the body stops PENDING at
     * +0C8C before 00BA:16EC: six words below the saved BP, nothing pushed for the call, DS:681A untouched, [BP-6] untouched.
     * The two words below SP are StrLoad's popped return frame (OV, 0C86h), left as the original leaves them; the
     * popped source pointer words were overwritten by the two pushes. */
    for (i = 0; i < 3; ++i) {
        static const uint8_t a3[3] = {0xEF, 0x00, 0x0F};
        c = world(a3[i], 0x05);
        CHECK(pr_u38_handler_0c4b_cpu(ram, RAM, &c, &d) == PR_U38_0C4B_PENDING);
        CHECK(d.callee == 2 && d.site == 0x0C8Cu && d.callee_status == PR_SYSTEM_LEAF_RETURNED);
        CHECK(c.cs == OV && c.ip == 0x0C8Cu && c.bp == SP0 - 2u && c.sp == SP0 - 20u && c.ss == SS0 && c.ds == DS0);
        CHECK(ram[at(ASEG, AOFF + 3u)] == (uint8_t)(a3[i] + 0x10u) && ram[at(DS0, 0x681Au)] == 0x05);
        CHECK(get16(SS0, SP0 - 2u) == 0x7777u && ram[at(SS0, SP0 - 8u)] == 0x5A);
        CHECK(memcmp(ram + at(SS0, SP0 - 7u), "\x04WXYZ", 5) == 0);
        CHECK(get16(SS0, SP0 - 10u) == BSEG && get16(SS0, SP0 - 12u) == BOFF);
        CHECK(get16(SS0, SP0 - 14u) == SS0 && get16(SS0, SP0 - 16u) == SP0 - 7u);
        CHECK(get16(SS0, SP0 - 18u) == ((c.ax & 0xFF00u) | 0x0Au) && get16(SS0, SP0 - 20u) == ((c.ax & 0xFF00u) | 0x01u));
        CHECK(get16(SS0, SP0 - 22u) == OV && get16(SS0, SP0 - 24u) == 0x0C86u && get16(SS0, SP0 - 26u) == 0x5A5Au);
        CHECK(c.es == SS0 && c.di == SP0 - 2u && c.cx == 0 && (c.ax & 0xFFu) == 1u);
        CHECK(only_changed(at(SS0, SP0 - 24u), at(SS0, SP0), at(ASEG, AOFF + 3u), at(ASEG, AOFF + 3u)));
    }

    /* Arguments: nothing changes. */
    c = world(0x10, 0x05); before = c;
    CHECK(pr_u38_handler_0c4b_cpu(NULL, RAM, &c, &d) == PR_U38_0C4B_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_handler_0c4b_cpu(ram, RAM - 1u, &c, &d) == PR_U38_0C4B_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_handler_0c4b_cpu(ram, RAM, NULL, &d) == PR_U38_0C4B_ARGUMENT);
    c.ip = 0x0C4Cu; before = c;
    CHECK(pr_u38_handler_0c4b_cpu(ram, RAM, &c, &d) == PR_U38_0C4B_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(only_changed(0, 0, 0, 0));
    for (i = 0; i < RAM; ++i)
        if (ram[i] != 0x5A && !(i >= at(OV, 0x0C46u) && i < at(OV, 0x0C4Bu)) && !(i >= at(OV, 0x0C84u) && i < at(OV, 0x0C86u))
            && !(i >= at(OV, 0x0C8Fu) && i < at(OV, 0x0C91u)) && i != at(ASEG, AOFF + 3u) && i != at(DS0, 0x681Au)
            && !(i >= at(SS0, SP0) && i < at(SS0, SP0 + 14u)))
            break;
    CHECK(i == RAM);

    free(ram); free(snap);
    printf("PASS u38 0c4b: %u checks\n", checks);
    return 0;
}
