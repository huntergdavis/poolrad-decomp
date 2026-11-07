/* Focused synthetic test for GAME.OVR unit 38h +070F, the 009E slot-21 handler (docs/re/OVL_0038_070F_CPU.md). No game
 * bytes: only the 12-byte Pascal constant at CS:0703, the two relocated segment words, synthetic records B and C, and the
 * call frame. StrLoad runs through its accepted leaf model; 16EC is a stop. */
#include "poolrad/recovered/overlay_u38_070f_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 070f line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, OV = 0x5000, SYS = 0x6AF8, RES = 0x10BA, DS0 = 0x1C7C, SS0 = 0x7000, SP0 = 0xF000,
       ASEG = 0x3000, AOFF = 0x0100, BSEG = 0x3C00, BOFF = 0x0200, CSEG = 0x4400, COFF = 0x0300 };
static const uint8_t SILENCED[12] = {0x0B, 'i', 's', ' ', 's', 'i', 'l', 'e', 'n', 'c', 'e', 'd'};

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* The constant at CS:0703, the segment words at CS:0737/0742, B with B+108h -> C, C+1 = c1 and C+2 = c2, and the frame:
 * return CS:IP 9001:7000, [BP+6] = unused, A (unread), B. AH enters as `ah`. */
static pr_u38_070f_cpu world(uint8_t ah, uint8_t c1, uint8_t c2, uint16_t unused)
{
    pr_u38_070f_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, OV, 0x070Fu, DS0, 0x1111u, SS0, 0x0ED7u};
    c.ax = (uint16_t)(((uint16_t)ah << 8) | 0x34u);
    memset(ram, 0x5A, RAM);
    memcpy(ram + at(OV, 0x0703u), SILENCED, sizeof SILENCED);
    put16(OV, 0x0737u, SYS); put16(OV, 0x0742u, RES);
    put16(BSEG, BOFF + 0x108u, COFF); put16(BSEG, BOFF + 0x10Au, CSEG);
    ram[at(CSEG, COFF + 1u)] = c1; ram[at(CSEG, COFF + 2u)] = c2;
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, unused); put16(SS0, SP0 + 6u, AOFF); put16(SS0, SP0 + 8u, ASEG);
    put16(SS0, SP0 + 10u, BOFF); put16(SS0, SP0 + 12u, BSEG);
    return c;
}

static void returned(const pr_u38_070f_cpu *c)
{
    CHECK(c->cs == 0x9001u && c->ip == 0x7000u && c->sp == SP0 + 14u && c->bp == 0x7777u);
}

int main(void)
{
    pr_u38_070f_cpu c, before;
    pr_u38_070f_diagnostic d;
    uint16_t ah;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* C+2 = 0: no calls; C+1 := 0 (C+2 stays 0); the local is never written; ES:DI is C from the last LES; the return
     * flags are CMP C+2,0's (ZF, PF) and the registers StrLoad would have changed are untouched. */
    c = world(0x11, 0x33, 0x00, 0x1234u);
    CHECK(pr_u38_070f_cpu_run(ram, RAM, &c, &d) == PR_U38_070F_OK);
    CHECK(d.callee == -1 && d.callee_status == -1 && d.site == 0);
    CHECK(ram[at(CSEG, COFF + 1u)] == 0 && ram[at(CSEG, COFF + 2u)] == 0 && ram[at(SS0, SP0 - 14u)] == 0x5A && ram[at(SS0, SP0 - 3u)] == 0x5A);
    CHECK(c.es == CSEG && c.di == COFF && c.flags == 0x0646u && c.ax == 0x1134u && c.cx == 0x5678u && c.dx == 0x789Au && c.bx == 0x3456u);
    CHECK(get16(SS0, SP0 - 2u) == 0x7777u && get16(SS0, SP0 - 16u) == 0x5A5Au);
    returned(&c);
    /* C+1 already 0 and C+2 0: still a plain return; [BP+6] and A are never read. */
    c = world(0x00, 0x00, 0x00, 0xFFFFu);
    ram[at(ASEG, AOFF + 3u)] = 0x77;
    CHECK(pr_u38_070f_cpu_run(ram, RAM, &c, NULL) == PR_U38_070F_OK && ram[at(CSEG, COFF + 1u)] == 0 && ram[at(ASEG, AOFF + 3u)] == 0x77);
    returned(&c);

    /* C+2 nonzero (1, FFh, 80h), every entry AH: StrLoad copies the 12-byte constant into SS:BP-0Ch, then the LCALL to
     * 16EC is the exact-call boundary. Saved state: frame (OV, 0744) under (wait AH|01, row AH|0A, SS:L, B) with the entry
     * AH in the high bytes (StrLoad keeps AH); below them the StrLoad frame remnants (its own frame and S were popped by
     * RETF 4: the words at SP0-22..SP0-30 are the pushed CS:0703 pair overwritten by the two AX words and the 16EC frame).
     * Registers as StrLoad leaves them (then AL := 0Ah, 1): AX AH|01, CX 0, DX DS, BX its entry SP, SI 070Fh, DI SP0-2, ES SS, DF clear,
     * flags XOR CH,CH's. C+1/C+2 untouched (the clears follow 16EC). */
    for (ah = 0; ah < 256; ++ah) {
        static const uint8_t c2s[] = {0x01, 0xFF, 0x80};
        size_t k;
        for (k = 0; k < sizeof c2s; ++k) {
            uint16_t high = (uint16_t)(ah << 8);
            c = world((uint8_t)ah, 0x33, c2s[k], (uint16_t)(ah * 0x0101u));
            CHECK(pr_u38_070f_cpu_run(ram, RAM, &c, &d) == PR_U38_070F_STOP_16EC);
            CHECK(d.callee == 2 && d.callee_status == -1 && d.site == 0x073Fu);
            CHECK(c.cs == RES && c.ip == 0x16ECu && c.sp == SP0 - 30u && c.bp == SP0 - 2u);
            CHECK(memcmp(ram + at(SS0, SP0 - 14u), SILENCED, sizeof SILENCED) == 0);
            CHECK(get16(SS0, SP0 - 30u) == 0x0744u && get16(SS0, SP0 - 28u) == OV);
            CHECK(get16(SS0, SP0 - 26u) == (high | 0x01u) && get16(SS0, SP0 - 24u) == (high | 0x0Au));
            CHECK(get16(SS0, SP0 - 22u) == SP0 - 14u && get16(SS0, SP0 - 20u) == SS0);
            CHECK(get16(SS0, SP0 - 18u) == BOFF && get16(SS0, SP0 - 16u) == BSEG && get16(SS0, SP0 - 2u) == 0x7777u);
            CHECK(c.ax == (high | 0x01u) && c.cx == 0 && c.dx == DS0 && c.bx == SP0 - 30u && c.si == 0x070Fu && c.di == SP0 - 2u);
            CHECK(c.es == SS0 && c.ds == DS0 && c.ss == SS0 && c.flags == 0x0246u);
            CHECK(ram[at(CSEG, COFF + 1u)] == 0x33 && ram[at(CSEG, COFF + 2u)] == c2s[k]);
            CHECK(memcmp(ram + at(OV, 0x0703u), SILENCED, sizeof SILENCED) == 0 && get16(OV, 0x0742u) == RES);
        }
    }
    /* The segment words are read live: a different word at CS:0742 is the saved CS. */
    c = world(0x11, 0x33, 0x01, 0);
    put16(OV, 0x0742u, 0x7BA0u);
    CHECK(pr_u38_070f_cpu_run(ram, RAM, &c, &d) == PR_U38_070F_STOP_16EC && c.cs == 0x7BA0u && c.ip == 0x16ECu);
    /* The constant is read live too: a shorter string at CS:0703 copies fewer bytes and leaves the rest of L as fill. */
    c = world(0x11, 0x33, 0x01, 0);
    ram[at(OV, 0x0703u)] = 3;
    CHECK(pr_u38_070f_cpu_run(ram, RAM, &c, &d) == PR_U38_070F_STOP_16EC && c.si == 0x0707u && c.di == SP0 - 10u);
    CHECK(ram[at(SS0, SP0 - 14u)] == 3 && ram[at(SS0, SP0 - 12u)] == 's' && ram[at(SS0, SP0 - 11u)] == ' ' && ram[at(SS0, SP0 - 10u)] == 0x5A);

    /* Bad arguments change nothing. */
    c = world(0x11, 0x33, 0x01, 0); before = c;
    CHECK(pr_u38_070f_cpu_run(NULL, RAM, &c, &d) == PR_U38_070F_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(d.callee == -1 && d.callee_status == -1 && d.site == 0);
    CHECK(pr_u38_070f_cpu_run(ram, RAM - 1u, &c, &d) == PR_U38_070F_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_070f_cpu_run(ram, RAM, NULL, &d) == PR_U38_070F_ARGUMENT);
    c.ip = 0x0710u;
    CHECK(pr_u38_070f_cpu_run(ram, RAM, &c, NULL) == PR_U38_070F_ARGUMENT && c.ip == 0x0710u && c.sp == SP0);
    CHECK(ram[at(CSEG, COFF + 1u)] == 0x33 && ram[at(SS0, SP0 - 14u)] == 0x5A);

    free(ram);
    printf("u38 070f: %u checks\n", checks);
    return 0;
}
