/* Focused synthetic test for GAME.OVR unit 38h +0B1B, the 009E slot-32 handler (docs/re/OVL_0038_0B1B_CPU.md). No game
 * bytes: only the 10-byte Pascal constant at CS:0B11, the relocated segment word at CS:0B46, synthetic records A and B,
 * the gate byte DS:[681F] and the call frame. StrLoad runs through its accepted leaf model; the near call to +0058 is a
 * stop. */
#include "poolrad/recovered/overlay_u38_0b1b_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 0b1b line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, OV = 0x5000, SYS = 0x6AF8, DS0 = 0x1C7C, SS0 = 0x7000, SP0 = 0xF000,
       ASEG = 0x3000, AOFF = 0x0100, BSEG = 0x3C00, BOFF = 0x0200 };
static const uint8_t COLLAPSES[10] = {0x09, 'c', 'o', 'l', 'l', 'a', 'p', 's', 'e', 's'};

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }
static uint8_t *a_byte(uint16_t off) { return ram + at(ASEG, AOFF + off); }
static uint8_t *b_byte(uint16_t off) { return ram + at(BSEG, BOFF + off); }

/* The constant at CS:0B11, the segment word at CS:0B46, the gate DS:[681F], A with A+3 = a3 and A+4 = 5Ah fill, B with
 * B+84h = b84 and B+98h = b98 (other B bytes fill), and the frame: return CS:IP 9001:7000, [BP+6] = unused, A, B. AH
 * enters as `ah`; CH is 56h. */
static pr_u38_0b1b_cpu world(uint8_t ah, uint8_t gate, uint8_t a3, uint8_t b84, uint8_t b98, uint16_t unused)
{
    pr_u38_0b1b_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, OV, 0x0B1Bu, DS0, 0x1111u, SS0, 0x0ED7u};
    c.ax = (uint16_t)(((uint16_t)ah << 8) | 0x34u);
    memset(ram, 0x5A, RAM);
    memcpy(ram + at(OV, 0x0B11u), COLLAPSES, sizeof COLLAPSES);
    put16(OV, 0x0B46u, SYS);
    ram[at(DS0, 0x681Fu)] = gate;
    *a_byte(3) = a3;
    *b_byte(0x84u) = b84; *b_byte(0x98u) = b98;
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, unused); put16(SS0, SP0 + 6u, AOFF); put16(SS0, SP0 + 8u, ASEG);
    put16(SS0, SP0 + 10u, BOFF); put16(SS0, SP0 + 12u, BSEG);
    return c;
}

static void returned(const pr_u38_0b1b_cpu *c)
{
    CHECK(c->cs == 0x9001u && c->ip == 0x7000u && c->sp == SP0 + 14u && c->bp == 0x7777u);
}

/* The OK path's stores, with B+84h cleared only when it entered as 0B3h; everything else of A and B is fill. */
static void ok_stores(uint8_t a3, uint8_t b84, uint8_t b98)
{
    CHECK(*a_byte(4) == 0 && *a_byte(3) == a3 && *a_byte(5) == 0x5A && *a_byte(2) == 0x5A);
    CHECK(*b_byte(0x10Eu) == (uint8_t)(a3 >> 4) && *b_byte(0x10Fu) == 1 && *b_byte(0x76u) == 0);
    CHECK(*b_byte(0x6Bu) == b98 && *b_byte(0x98u) == b98 && *b_byte(0x72u) == 0x0C && *b_byte(0x9Fu) == 0);
    CHECK(*b_byte(0x84u) == (b84 == 0xB3 ? 0 : b84));
    CHECK(*b_byte(0x10Du) == 0x5A && *b_byte(0x110u) == 0x5A && *b_byte(0x75u) == 0x5A && *b_byte(0x77u) == 0x5A);
    CHECK(*b_byte(0x6Au) == 0x5A && *b_byte(0x6Cu) == 0x5A && *b_byte(0x71u) == 0x5A && *b_byte(0x73u) == 0x5A);
    CHECK(*b_byte(0x83u) == 0x5A && *b_byte(0x85u) == 0x5A && *b_byte(0x9Eu) == 0x5A && *b_byte(0xA0u) == 0x5A);
}

int main(void)
{
    pr_u38_0b1b_cpu c, before;
    pr_u38_0b1b_diagnostic d;
    unsigned v, k;
    static const uint8_t gates[] = {0x01, 0xFF, 0x80};
    /* CMP B+84h,0B3h: the return flags for B+84h = 0B3h (ZF PF), 00h (CF PF AF), 0B4h (none) and 0B2h (CF PF AF SF),
     * on the entry flags 0ED7h with the arithmetic bits cleared (0602h; DF stays set: StrLoad does not run). */
    static const uint8_t b84s[] = {0xB3, 0x00, 0xB4, 0xB2};
    static const uint16_t b84_flags[] = {0x0646u, 0x0617u, 0x0602u, 0x0697u};
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Gate DS:[681F] nonzero: no call; A+4 := 0, the B stores, AX = B+98h (AH 0 after XOR AH,AH and SHR), CL = 4
     * with CH kept, ES:DI = B from the last LES, L never written, [BP+6] never read, the frame word untouched. */
    for (v = 0; v < 256; ++v) {
        for (k = 0; k < sizeof gates; ++k) {
            c = world(0x11, gates[k], (uint8_t)v, 0xB3, (uint8_t)(v ^ 0xA5u), (uint16_t)(v * 0x0101u));
            CHECK(pr_u38_0b1b_cpu_run(ram, RAM, &c, &d) == PR_U38_0B1B_OK);
            CHECK(d.callee == -1 && d.callee_status == -1 && d.site == 0);
            ok_stores((uint8_t)v, 0xB3, (uint8_t)(v ^ 0xA5u));
            CHECK(c.ax == (uint16_t)(v ^ 0xA5u) && c.cx == 0x5604u && c.dx == 0x789Au && c.bx == 0x3456u && c.si == 0x9ABCu);
            CHECK(c.es == BSEG && c.di == BOFF && c.ds == DS0 && c.ss == SS0 && c.flags == 0x0646u);
            CHECK(get16(SS0, SP0 - 2u) == 0x7777u && ram[at(SS0, SP0 - 12u)] == 0x5A && ram[at(SS0, SP0 - 3u)] == 0x5A && ram[at(SS0, SP0 - 14u)] == 0x5A);
            CHECK(ram[at(DS0, 0x681Fu)] == gates[k] && get16(SS0, SP0 + 4u) == (uint16_t)(v * 0x0101u));
            returned(&c);
        }
    }
    /* The B+84h compare decides the clear and the return flags; B+98h = 0 and FFh ride into AX. */
    for (k = 0; k < sizeof b84s; ++k) {
        c = world(0x00, 0x01, 0x7F, b84s[k], 0xFF, 0);
        CHECK(pr_u38_0b1b_cpu_run(ram, RAM, &c, NULL) == PR_U38_0B1B_OK);
        ok_stores(0x7F, b84s[k], 0xFF);
        CHECK(c.ax == 0x00FFu && c.flags == b84_flags[k]);
        returned(&c);
        c = world(0xFF, 0xFF, 0xF0, b84s[k], 0x00, 0xFFFFu);
        CHECK(pr_u38_0b1b_cpu_run(ram, RAM, &c, &d) == PR_U38_0B1B_OK);
        ok_stores(0xF0, b84s[k], 0x00);
        CHECK(c.ax == 0x0000u && c.flags == b84_flags[k] && d.callee == -1);
        returned(&c);
    }

    /* Gate zero, every entry AH: StrLoad copies the 10-byte constant into L = SS:BP-0Ah, then PUSH CS; CALL 0058 is the
     * exact-call boundary. Saved state: BP = SP0-2, L at SP0-12..SP0-3; under it B (SP0-16/-14), the AX word AH|06
     * (SP0-18), SS:L (SP0-22/-20), then the near frame CS (SP0-24) and 0B4C (SP0-26); SP = SP0-26, CS unchanged, IP
     * 0058. The words at SP0-28/-30 are the StrLoad far frame left below (its RETF 4 popped it and the source pair).
     * Registers as StrLoad leaves them: AX AH|09, CX 0, DX DS, BX its entry SP (SP0-30), SI 0B1Bh, DI SP0-2, ES SS, DF
     * clear, flags XOR CH,CH's (0246h). A+4 cleared before the gate; the step-4 stores not done. */
    for (v = 0; v < 256; ++v) {
        uint16_t high = (uint16_t)(v << 8);
        c = world((uint8_t)v, 0x00, 0x77, 0xB3, 0x88, (uint16_t)v);
        CHECK(pr_u38_0b1b_cpu_run(ram, RAM, &c, &d) == PR_U38_0B1B_STOP_0058);
        CHECK(d.callee == 2 && d.callee_status == -1 && d.site == 0x0B49u);
        CHECK(c.cs == OV && c.ip == 0x0058u && c.sp == SP0 - 26u && c.bp == SP0 - 2u);
        CHECK(memcmp(ram + at(SS0, SP0 - 12u), COLLAPSES, sizeof COLLAPSES) == 0);
        CHECK(get16(SS0, SP0 - 2u) == 0x7777u && get16(SS0, SP0 - 14u) == BSEG && get16(SS0, SP0 - 16u) == BOFF);
        CHECK(get16(SS0, SP0 - 18u) == (high | 0x06u) && get16(SS0, SP0 - 20u) == SS0 && get16(SS0, SP0 - 22u) == SP0 - 12u);
        CHECK(get16(SS0, SP0 - 24u) == OV && get16(SS0, SP0 - 26u) == 0x0B4Cu);
        CHECK(get16(SS0, SP0 - 28u) == OV && get16(SS0, SP0 - 30u) == 0x0B48u);
        CHECK(c.ax == (high | 0x09u) && c.cx == 0 && c.dx == DS0 && c.bx == SP0 - 30u && c.si == 0x0B1Bu && c.di == SP0 - 2u);
        CHECK(c.es == SS0 && c.ds == DS0 && c.ss == SS0 && c.flags == 0x0246u);
        CHECK(*a_byte(4) == 0 && *a_byte(3) == 0x77 && *b_byte(0x10Eu) == 0x5A && *b_byte(0x10Fu) == 0x5A && *b_byte(0x76u) == 0x5A);
        CHECK(*b_byte(0x6Bu) == 0x5A && *b_byte(0x72u) == 0x5A && *b_byte(0x84u) == 0xB3 && *b_byte(0x9Fu) == 0x5A);
        CHECK(memcmp(ram + at(OV, 0x0B11u), COLLAPSES, sizeof COLLAPSES) == 0 && get16(OV, 0x0B46u) == SYS);
    }
    /* The segment word is read live: StrLoad's model is addressed through CS:0B46, so a word that does not reach the
     * leaf's entry is still accepted by the leaf (it runs on IP alone) and the body continues to the stop. */
    c = world(0x11, 0x00, 0x77, 0xB3, 0x88, 0);
    put16(OV, 0x0B46u, 0x7AF8u);
    CHECK(pr_u38_0b1b_cpu_run(ram, RAM, &c, &d) == PR_U38_0B1B_STOP_0058 && get16(SS0, SP0 - 30u) == 0x0B48u && c.cs == OV);
    /* The constant is read live too: a shorter string at CS:0B11 copies fewer bytes and leaves the rest of L as fill. */
    c = world(0x11, 0x00, 0x77, 0xB3, 0x88, 0);
    ram[at(OV, 0x0B11u)] = 3;
    CHECK(pr_u38_0b1b_cpu_run(ram, RAM, &c, &d) == PR_U38_0B1B_STOP_0058 && c.si == 0x0B15u && c.di == SP0 - 8u && c.ax == 0x1103u);
    CHECK(ram[at(SS0, SP0 - 12u)] == 3 && ram[at(SS0, SP0 - 11u)] == 'c' && ram[at(SS0, SP0 - 9u)] == 'l' && ram[at(SS0, SP0 - 8u)] == 0x5A);

    /* Bad arguments change nothing. */
    c = world(0x11, 0x01, 0x77, 0xB3, 0x88, 0); before = c;
    CHECK(pr_u38_0b1b_cpu_run(NULL, RAM, &c, &d) == PR_U38_0B1B_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(d.callee == -1 && d.callee_status == -1 && d.site == 0);
    CHECK(pr_u38_0b1b_cpu_run(ram, RAM - 1u, &c, &d) == PR_U38_0B1B_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_0b1b_cpu_run(ram, RAM, NULL, &d) == PR_U38_0B1B_ARGUMENT);
    c.ip = 0x0B1Cu;
    CHECK(pr_u38_0b1b_cpu_run(ram, RAM, &c, NULL) == PR_U38_0B1B_ARGUMENT && c.ip == 0x0B1Cu && c.sp == SP0);
    CHECK(*a_byte(4) == 0x5A && *b_byte(0x10Fu) == 0x5A && ram[at(SS0, SP0 - 12u)] == 0x5A);

    free(ram);
    printf("u38 0b1b: %u checks\n", checks);
    return 0;
}
