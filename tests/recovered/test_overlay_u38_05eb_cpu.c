/* Focused synthetic test for GAME.OVR unit 38h +05EB, the 009E slot-15 handler (docs/re/OVL_0038_05EB_CPU.md). No game
 * bytes: only the relocated far-call segment words, the 001F binding operands its own test uses, synthetic records A and
 * B, the heap words 001F's append walks, and the call frame. */
#include "poolrad/recovered/overlay_u38_05eb_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 05eb line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, OV = 0x5000, STUB52 = 0x6000, DICE = 0x7800, SYS = 0x0AF8, STUBB0 = 0x6B00, RES = 0x10BA,
       DS0 = 0x1C7C, SS0 = 0x7000, SP0 = 0xF000, HEAP = 0x3400,
       ASEG = 0x3000, AOFF = 0x0100, BSEG = 0x3C00, BOFF = 0x0200 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* The two live segment words, 001F's binding chain (CS:004A LCALL stub 0052 -> EA jump -> U9E +0E52 -> SYSTEM GetMem),
 * the heap words 001F's append model reads, records A (A+3 = a3) and B (B+11Bh = b11b, empty list head at B+7Fh/+81h), DS:[681F]
 * = gate, and the frame: return CS:IP 9001:7000, [BP+6] = unused, A, B. AH enters as `ah`. */
static pr_u38_05eb_cpu world(uint8_t ah, uint8_t gate, uint8_t a3, uint8_t b11b, uint16_t unused)
{
    pr_u38_05eb_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, OV, 0x05EBu, DS0, 0x1111u, SS0, 0x0ED7u};
    c.ax = (uint16_t)(((uint16_t)ah << 8) | 0x34u);
    memset(ram, 0x5A, RAM);
    put16(OV, 0x062Du, STUBB0); put16(OV, 0x0641u, RES);
    ram[at(OV, 0x004Au)] = 0x9A; put16(OV, 0x004Bu, 0x0052u); put16(OV, 0x004Du, STUB52);
    ram[at(STUB52, 0x0052u)] = 0xEA; put16(STUB52, 0x0053u, 0x0E52u); put16(STUB52, 0x0055u, DICE);
    ram[at(DICE, 0x0E5Cu)] = 0x9A; put16(DICE, 0x0E5Du, 0x023Fu); put16(DICE, 0x0E5Fu, SYS);
    put16(DS0, 0x4404u, 0); put16(DS0, 0x4406u, HEAP); put16(DS0, 0x4408u, 0); put16(DS0, 0x440Au, HEAP + 0x100u);
    put16(DS0, 0x440Cu, 0); put16(DS0, 0x440Eu, HEAP); put16(DS0, 0x4414u, 0xA9u); put16(DS0, 0x4416u, SYS);
    ram[at(DS0, 0x681Fu)] = gate;
    ram[at(ASEG, AOFF + 3u)] = a3;
    ram[at(BSEG, BOFF + 0x11Bu)] = b11b; put16(BSEG, BOFF + 0x7Fu, 0); put16(BSEG, BOFF + 0x81u, 0);
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, unused); put16(SS0, SP0 + 6u, AOFF); put16(SS0, SP0 + 8u, ASEG);
    put16(SS0, SP0 + 10u, BOFF); put16(SS0, SP0 + 12u, BSEG);
    return c;
}

static void returned(const pr_u38_05eb_cpu *c)
{
    CHECK(c->cs == 0x9001u && c->ip == 0x7000u && c->sp == SP0 + 14u && c->bp == 0x7777u);
}

/* The append 001F performs when DS:[681F] is 0: node (b 0Fh, a 000Ah, c A+3, 1, link 0:0) at HEAP, linked from B+81h. */
static void appended(uint8_t a3)
{
    CHECK(ram[at(HEAP, 0)] == 0x0F && get16(HEAP, 1) == 0x000Au && ram[at(HEAP, 3)] == a3 && ram[at(HEAP, 4)] == 1);
    CHECK(get16(HEAP, 5) == 0 && get16(HEAP, 7) == 0 && get16(BSEG, BOFF + 0x7Fu) == 0 && get16(BSEG, BOFF + 0x81u) == HEAP);
}

int main(void)
{
    pr_u38_05eb_cpu c, before;
    pr_u38_05eb_diagnostic d;
    pr_u38_05eb_limits lim = {64u, 64u};
    uint16_t ah;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Gate set: 001F stores result 0 and appends nothing; AL 0 returns at once. OR AL,AL on 0: ZF, PF set. */
    c = world(0x11, 0x80, 0x33, 0xFF, 0x1234u);
    CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, &lim, &d) == PR_U38_05EB_OK);
    CHECK(d.callee == 1 && d.callee_status == PR_U38_001F_OK && d.site == 0x0604u && d.d001f.append_status == -1);
    CHECK((c.ax & 0xFFu) == 0 && c.flags == 0x0646u && ram[at(DS0, 0x6819u)] == 0x5A && ram[at(HEAP, 0)] == 0x5A);
    CHECK(get16(BSEG, BOFF + 0x81u) == 0 && ram[at(BSEG, BOFF + 0x11Bu)] == 0xFF);
    returned(&c);
    /* The frame 001F saw, left below SP: B, 0Fh and A+3 as AX words carrying the ENTRY AH, 000Ah (which cleared AH),
     * PUSH CS and the return IP 0607; then 001F's own BP save and its result byte 0 at its BP-1. */
    CHECK(get16(SS0, SP0 - 4u) == BSEG && get16(SS0, SP0 - 6u) == BOFF && get16(SS0, SP0 - 8u) == 0x110Fu);
    CHECK(get16(SS0, SP0 - 10u) == 0x1133u && get16(SS0, SP0 - 12u) == 0x000Au && get16(SS0, SP0 - 14u) == OV);
    CHECK(get16(SS0, SP0 - 16u) == 0x0607u && get16(SS0, SP0 - 18u) == SP0 - 2u && ram[at(SS0, SP0 - 19u)] == 0);
    /* The gate byte is tested for zero, not for a sign or a bit. */
    c = world(0x11, 0x01, 0x33, 0xFF, 0x1234u);
    CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, &lim, &d) == PR_U38_05EB_OK && d.d001f.append_status == -1);
    returned(&c);

    /* Gate clear: 001F appends (b 0Fh, a 0Ah, c A+3, 1) to B and returns AL 1; B+11Bh 1 is not above 1: return.
     * The last flags writer is CMP B+11Bh,1 (1 - 1: ZF, PF); ES:DI is B from the LES. [BP+6] is never read. */
    for (ah = 0; ah < 256; ++ah) {
        c = world((uint8_t)ah, 0, 0xC7, 0x01, (uint16_t)(ah * 0x0101u));
        CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, &lim, &d) == PR_U38_05EB_OK);
        CHECK(d.callee == 1 && d.callee_status == PR_U38_001F_OK && d.d001f.append_status == 0 && d.d001f.getmem_status == 0);
        CHECK((c.ax & 0xFFu) == 1 && c.flags == 0x0646u && c.es == BSEG && c.di == BOFF && ram[at(DS0, 0x6819u)] == 0x5A);
        appended(0xC7);
        returned(&c);
    }
    /* B+11Bh 0 also returns: 0 - 1 borrows (CF, AF, SF, PF of FFh). */
    c = world(0x22, 0, 0x05, 0x00, 0);
    CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, &lim, &d) == PR_U38_05EB_OK && ram[at(DS0, 0x6819u)] == 0x5A && c.flags == 0x0697u);
    appended(0x05);
    returned(&c);

    /* B+11Bh 2 (and FFh: the compare is unsigned): DS:[6819] := 0, then the LCALL at 062A to the live stub word is the
     * exact-call boundary. CPU saved as the LCALL left it: frame (OV, 062F) on the stack under (B, 1, 0, 0) pushed as AX
     * words carrying 001F's AH, CS the word at OV:062D, IP 007F, BP still this body's frame, A/B records untouched. */
    {
        static const uint8_t b11[] = {0x02, 0xFF, 0x80};
        size_t k;
        for (k = 0; k < sizeof b11; ++k) {
            uint16_t high;
            c = world(0x77, 0, 0x05, b11[k], 0xFFFFu);
            CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, &lim, &d) == PR_U38_05EB_STOP_1344);
            CHECK(d.callee == 2 && d.callee_status == -1 && d.site == 0x062Au && d.d001f.append_status == 0);
            high = (uint16_t)(c.ax & 0xFF00u);
            CHECK(c.cs == STUBB0 && c.ip == 0x007Fu && c.sp == SP0 - 16u && c.bp == SP0 - 2u && (c.ax & 0xFFu) == 0);
            CHECK(get16(SS0, SP0 - 16u) == 0x062Fu && get16(SS0, SP0 - 14u) == OV);
            CHECK(get16(SS0, SP0 - 12u) == (high | 0u) && get16(SS0, SP0 - 10u) == (high | 0u) && get16(SS0, SP0 - 8u) == (high | 1u));
            CHECK(get16(SS0, SP0 - 6u) == BOFF && get16(SS0, SP0 - 4u) == BSEG && get16(SS0, SP0 - 2u) == 0x7777u);
            CHECK(ram[at(DS0, 0x6819u)] == 0 && ram[at(BSEG, BOFF + 0x11Bu)] == b11[k] && ram[at(ASEG, AOFF + 3u)] == 0x05);
            CHECK(c.es == BSEG && c.di == BOFF && c.ds == DS0 && c.ss == SS0);
            appended(0x05);
        }
    }
    /* The stop is a frozen prefix: DS:[49F3] and DS:[5D92]/[5D94] are not read, nothing after the LCALL is pushed. Below
     * the frame the words are 001F's earlier BP save and result byte, not the 0767 argument words. */
    c = world(0x77, 0, 0x05, 0x02, 0);
    ram[at(DS0, 0x49F3u)] = 5; put16(DS0, 0x5D92u, 0xDEADu); put16(DS0, 0x5D94u, 0xBEEFu);
    CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, &lim, &d) == PR_U38_05EB_STOP_1344 && c.ip == 0x007Fu);
    CHECK(get16(SS0, SP0 - 18u) == SP0 - 2u && ram[at(SS0, SP0 - 19u)] == 1 && get16(DS0, 0x5D92u) == 0xDEADu);
    /* NULL limits: both caps zero keep 001F's dependency meanings (its own test: OK on this world). */
    c = world(0x00, 0, 0x05, 0x01, 0);
    CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, NULL, NULL) == PR_U38_05EB_OK);
    appended(0x05);
    returned(&c);

    /* 001F's own stops propagate: a changed CS:004A byte is its CODE stop; the state is as 001F left it. */
    c = world(0x11, 0, 0x05, 0x02, 0);
    ram[at(OV, 0x004Au)] = 0x90;
    CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, &lim, &d) == PR_U38_05EB_CALLEE);
    CHECK(d.callee == 1 && d.callee_status == PR_U38_001F_CODE && d.site == 0x0604u && d.d001f.append_status == -1);
    CHECK(c.cs == OV && c.ip == 0x004Au && ram[at(DS0, 0x6819u)] == 0x5A);
    /* ... and its CALLEE stop (GetMem RunError from an aliased free list) with the nested statuses. */
    c = world(0x11, 0, 0x05, 0x02, 0);
    put16(DS0, 0x440Au, HEAP);
    CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, &lim, &d) == PR_U38_05EB_CALLEE);
    CHECK(d.callee_status == PR_U38_001F_CALLEE && d.d001f.append_status == 3 && d.d001f.getmem_status == 2 && c.ip == 0xE2u && c.ax == 203u);

    /* Bad arguments change nothing. */
    c = world(0x11, 0, 0x05, 0x02, 0); before = c;
    CHECK(pr_u38_05eb_cpu_run(NULL, RAM, &c, &lim, &d) == PR_U38_05EB_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(d.callee == -1 && d.callee_status == -1 && d.site == 0 && d.d001f.append_status == -1 && d.d001f.getmem_status == -1);
    CHECK(pr_u38_05eb_cpu_run(ram, RAM - 1u, &c, &lim, &d) == PR_U38_05EB_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_05eb_cpu_run(ram, RAM, NULL, &lim, &d) == PR_U38_05EB_ARGUMENT);
    c.ip = 0x05ECu;
    CHECK(pr_u38_05eb_cpu_run(ram, RAM, &c, &lim, NULL) == PR_U38_05EB_ARGUMENT && c.ip == 0x05ECu && c.sp == SP0);
    CHECK(ram[at(DS0, 0x6819u)] == 0x5A && ram[at(HEAP, 0)] == 0x5A);

    free(ram);
    printf("u38 05eb: %u checks\n", checks);
    return 0;
}
