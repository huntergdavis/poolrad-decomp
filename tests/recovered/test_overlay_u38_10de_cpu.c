/* Focused synthetic test for GAME.OVR unit 38h +10DE, the 009E slot-43 handler (docs/re/OVL_0038_10DE_CPU.md). No game
 * bytes: only the four relocated far-call segment words, +001F's own binding word, the loaded-stub form of unit 9E entry
 * 10, unit 9E's GetMem binding, the SYSTEM heap words, a synthetic 11-character source string at CS:10D2, synthetic
 * records A and B, a synthetic list and the DS gate byte. Whole-RAM comparisons bound each path's writes. */
#include "poolrad/recovered/overlay_u38_10de_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 10de line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, OV = 0x5000, SYS = 0x0AF8, RES = 0x10BA, STUB = 0x00B0, STUB2 = 0x00C0, U9E = 0x6000, DS0 = 0x1C7C,
       SS0 = 0x7000, SP0 = 0xF000, HEAP = 0x3400, ASEG = 0x3000, AOFF = 0x0100, BSEG = 0x3200, BOFF = 0x0200,
       OSEG = 0x3300, OOFF = 0x0010 };

static uint8_t *ram, *snap;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* The frame, the fixup words, +001F's binding (CS:004A LCALL 00B0:0052), the stub, the U9E GetMem binding, the heap words
 * (free list empty, heap pointer HEAP:0000, heap end HEAP+100h, HeapError SYS:00A9), record A's byte +3, record B's byte
 * +10h and list head (nil, or an old node at OSEG:OOFF whose byte 0 is `oldkey` and whose link is nil), the gate DS:681F
 * and an 11-character string at CS:10D2. */
static pr_u38_10de_cpu world(uint8_t a3, uint8_t b10, uint8_t gate, int old, uint8_t oldkey)
{
    pr_u38_10de_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, OV, 0x10DEu, DS0, 0x1111u, SS0, 0x0002u};
    memset(ram, 0x5A, RAM);
    put16(OV, 0x111Eu, SYS); put16(OV, 0x1129u, RES); put16(OV, 0x1145u, RES); put16(OV, 0x1160u, STUB);
    ram[at(OV, 0x004Au)] = 0x9A; put16(OV, 0x004Bu, 0x0052u); put16(OV, 0x004Du, STUB);
    ram[at(STUB, 0x0052u)] = 0xEA; put16(STUB, 0x0053u, 0x0E52u); put16(STUB, 0x0055u, U9E);
    ram[at(U9E, 0x0E5Cu)] = 0x9A; put16(U9E, 0x0E5Du, 0x023Fu); put16(U9E, 0x0E5Fu, SYS);
    put16(DS0, 0x4404u, 0); put16(DS0, 0x4406u, HEAP); put16(DS0, 0x4408u, 0); put16(DS0, 0x440Au, HEAP + 0x100u);
    put16(DS0, 0x440Cu, 0); put16(DS0, 0x440Eu, HEAP); put16(DS0, 0x4414u, 0x00A9u); put16(DS0, 0x4416u, SYS);
    memcpy(ram + at(OV, 0x10D2u), "\x0bWXYZWXYZWXY", 12);
    ram[at(ASEG, AOFF + 3u)] = a3;
    ram[at(BSEG, BOFF + 0x10u)] = b10;
    put16(BSEG, BOFF + 0x7Fu, old ? OOFF : 0); put16(BSEG, BOFF + 0x81u, old ? OSEG : 0);
    ram[at(OSEG, OOFF)] = oldkey; put16(OSEG, OOFF + 5u, 0); put16(OSEG, OOFF + 7u, 0);
    ram[at(DS0, 0x681Fu)] = gate;
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, 0x5A5Au); put16(SS0, SP0 + 6u, AOFF); put16(SS0, SP0 + 8u, ASEG);
    put16(SS0, SP0 + 10u, BOFF); put16(SS0, SP0 + 12u, BSEG);
    memcpy(snap, ram, RAM);
    return c;
}

/* Every RAM byte is as the world left it except the stack below SP0 (down to 128 bytes), the heap words DS:4400..4418,
 * GetMem's saved size word DS:8754, the heap area HEAP:[0,40h), record B's list head and the old node's link. */
static int only_changed(void)
{
    size_t i;
    for (i = 0; i < RAM; ++i)
        if (ram[i] != snap[i] && !(i >= at(SS0, SP0 - 128u) && i < at(SS0, SP0)) && !(i >= at(DS0, 0x4400u) && i < at(DS0, 0x4418u))
            && !(i >= at(DS0, 0x8754u) && i < at(DS0, 0x8756u)) && !(i >= at(HEAP, 0) && i < at(HEAP, 0x40u))
            && !(i >= at(BSEG, BOFF + 0x7Fu) && i < at(BSEG, BOFF + 0x83u)) && !(i >= at(OSEG, OOFF + 5u) && i < at(OSEG, OOFF + 9u)))
            return 0;
    return 1;
}

static void returned(const pr_u38_10de_cpu *c)
{
    CHECK(c->cs == 0x9001u && c->ip == 0x7000u && c->sp == SP0 + 14u && c->bp == 0x7777u && c->ds == DS0 && c->ss == SS0);
}

/* A 9-byte node at physical p: byte 0, word 1, byte 3, byte 4, link +5/+7. */
static int node_is(size_t p, uint8_t b0, uint16_t w1, uint8_t b3, uint8_t b4, uint16_t loff, uint16_t lseg)
{
    return ram[p] == b0 && (ram[p + 1] | (ram[p + 2] << 8)) == w1 && ram[p + 3] == b3 && ram[p + 4] == b4
        && (ram[p + 5] | (ram[p + 6] << 8)) == loff && (ram[p + 7] | (ram[p + 8] << 8)) == lseg;
}

int main(void)
{
    pr_u38_10de_cpu c, before;
    pr_u38_10de_limits lim = {64, 64, 64};
    pr_u38_10de_diagnostic d;
    unsigned i;
    size_t n1, n2;
    ram = malloc(RAM); snap = malloc(RAM);
    if (ram == NULL || snap == NULL)
        return 1;

    /* The gate DS:681F is nonzero: +001F stores 0 and allocates nothing; OR AL,AL; JE returns. The argument words pushed for
     * +001F carry the entry AH (122Bh, 12xxh with A+3) and then 003Ch (MOV AX,3Ch clears AH) and stay below SP. */
    for (i = 1; i < 256; i += 17) {
        c = world(0x77, 5, (uint8_t)i, 0, 0);
        CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_OK);
        CHECK(d.callee == 1 && d.callee_status == PR_U38_001F_OK && d.site == 0x10FAu && d.d001f.append_status == -1 && d.getmem_status == -1);
        CHECK(c.ax == 0 && (c.flags & 0x08D5u) == 0x0044u);                       /* MOV AX,3Ch cleared AH before the call */
        CHECK(get16(SS0, SP0 - 24u) == 0x122Bu && get16(SS0, SP0 - 26u) == 0x1277u && get16(SS0, SP0 - 28u) == 0x003Cu && get16(SS0, SP0 - 30u) == OV && get16(SS0, SP0 - 32u) == 0x10FDu);
        CHECK(get16(DS0, 0x440Eu) == HEAP && get16(BSEG, BOFF + 0x7Fu) == 0 && ram[at(BSEG, BOFF + 0x10u)] == 5);
        CHECK(only_changed());
        returned(&c);
    }

    /* Gate zero, B+10h > 3: +001F appends node 1 (2Bh, 003Ch, A+3, 1) at the heap pointer and makes it B's head, then the
     * string is copied into [BP-10h] and the body stops PENDING at +1126 before 00BA:16EC with the six words pushed
     * (B, SS:BP-10h, AH:0Ah, AH:01h), nothing pushed for the call, B+10h not yet decremented. */
    for (i = 4; i < 256; i += 50) {
        c = world((uint8_t)(i ^ 0x5Au), (uint8_t)i, 0, 0, 0);
        CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_PENDING);
        CHECK(d.callee == 3 && d.site == 0x1126u && d.d001f.append_status == PR_U9E_0E52_OK && d.d001f.getmem_status == 0);
        CHECK(c.cs == OV && c.ip == 0x1126u && c.bp == SP0 - 2u && c.sp == SP0 - 30u && c.ss == SS0 && c.ds == DS0);
        n1 = at(get16(BSEG, BOFF + 0x81u), get16(BSEG, BOFF + 0x7Fu));
        CHECK(n1 == at(HEAP, 0) && node_is(n1, 0x2Bu, 0x003Cu, (uint8_t)(i ^ 0x5Au), 1u, 0, 0));
        CHECK(memcmp(ram + at(SS0, SP0 - 18u), "\x0bWXYZWXYZWXY", 12) == 0 && get16(SS0, SP0 - 2u) == 0x7777u);
        CHECK(get16(SS0, SP0 - 20u) == BSEG && get16(SS0, SP0 - 22u) == BOFF && get16(SS0, SP0 - 24u) == SS0 && get16(SS0, SP0 - 26u) == SP0 - 18u);
        CHECK((get16(SS0, SP0 - 28u) & 0xFFu) == 0x0Au && (get16(SS0, SP0 - 30u) & 0xFFu) == 0x01u && (c.ax & 0xFFu) == 1u);
        CHECK(get16(SS0, SP0 - 28u) == ((c.ax & 0xFF00u) | 0x0Au) && get16(SS0, SP0 - 32u) == OV && get16(SS0, SP0 - 34u) == 0x1120u);
        CHECK(c.es == SS0 && c.di == SP0 - 6u && c.cx == 0 && ram[at(BSEG, BOFF + 0x10u)] == i);
        CHECK(only_changed());
    }

    /* Gate zero, B+10h <= 3, head nil: node 1 is appended, 2199 walks the one-node list for key 1Fh and fails (cursor nil),
     * so the second append links node 2 (1Fh, 0, FFh, 0) after node 1 and the body returns with +0E52's exit state. */
    for (i = 0; i <= 3; ++i) {
        c = world(0x42, (uint8_t)i, 0, 0, 0);
        CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_OK);
        CHECK(d.callee == 5 && d.callee_status == PR_U9E_0E52_OK && d.site == 0x115Du && d.getmem_status == 0 && d.d001f.append_status == 0);
        n1 = at(get16(BSEG, BOFF + 0x81u), get16(BSEG, BOFF + 0x7Fu));
        n2 = at((uint16_t)(ram[n1 + 7] | (ram[n1 + 8] << 8)), (uint16_t)(ram[n1 + 5] | (ram[n1 + 6] << 8)));
        CHECK(n1 == at(HEAP, 0) && n2 == at(HEAP, 0x10u) && node_is(n2, 0x1Fu, 0, 0xFFu, 0, 0, 0) && ram[n1] == 0x2Bu && ram[n1 + 3] == 0x42u);
        CHECK(get16(SS0, SP0 - 6u) == 0 && get16(SS0, SP0 - 4u) == 0);                 /* 2199's cursor cell: nil */
        CHECK(at(c.es, c.di) == n2 && c.ax == 0x00FFu && (c.flags & 0x08D5u) == 0x0044u);
        CHECK(get16(SS0, SP0 - 24u) == 0x1Fu + (get16(SS0, SP0 - 24u) & 0xFF00u) && get16(SS0, SP0 - 26u) == 0 && get16(SS0, SP0 - 28u) == 0x00FFu && get16(SS0, SP0 - 30u) == 0);
        CHECK(only_changed());
        returned(&c);
    }

    /* An old node with key 1Fh already heads the list: node 1 goes after it, 2199 finds the old node (cursor = it, AL = 1)
     * and the body returns without a second append (the heap pointer advanced once). */
    c = world(0x42, 2, 0, 1, 0x1Fu);
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_OK);
    CHECK(d.callee == 4 && d.callee_status == PR_RES_2199_OK && d.site == 0x1142u && d.getmem_status == -1);
    CHECK(get16(OSEG, OOFF + 5u) == 0 && get16(OSEG, OOFF + 7u) == HEAP && node_is(at(HEAP, 0), 0x2Bu, 0x003Cu, 0x42u, 1u, 0, 0));
    CHECK(ram[at(HEAP, 0x10u)] == 0x5A && get16(SS0, SP0 - 6u) == OOFF && get16(SS0, SP0 - 4u) == OSEG && (c.ax & 0xFFu) == 1u);
    CHECK((c.flags & 0x08D5u) == 0 && only_changed());
    returned(&c);
    /* An old node with another key: not found, node 2 is appended after node 1 (old -> node 1 -> node 2). */
    c = world(0x42, 0, 0, 1, 0x2Bu);
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_OK && d.callee == 5);
    CHECK(get16(OSEG, OOFF + 7u) == HEAP && node_is(at(HEAP, 0), 0x2Bu, 0x003Cu, 0x42u, 1u, 0, HEAP + 1u) && node_is(at(HEAP, 0x10u), 0x1Fu, 0, 0xFFu, 0, 0, 0));
    CHECK(only_changed());
    returned(&c);

    /* The +115D stub (read through the fixup word CS:1160, distinct from +001F's binding word CS:004D) is not a far JMP to
     * +0E52: CODE at +115D with the six words pushed and nothing for the call; node 2 is not allocated. */
    c = world(0x42, 1, 0, 0, 0); put16(OV, 0x1160u, STUB2); ram[at(STUB2, 0x0052u)] = 0xCD; put16(STUB2, 0x0053u, 0x0E52u); put16(STUB2, 0x0055u, U9E);
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_CODE);
    CHECK(d.site == 0x115Du && d.callee == 4 && c.cs == OV && c.ip == 0x115Du && c.sp == SP0 - 30u && ram[at(HEAP, 0x10u)] == 0x5A);
    CHECK(get16(SS0, SP0 - 20u) == BSEG && get16(SS0, SP0 - 22u) == BOFF && (get16(SS0, SP0 - 24u) & 0xFFu) == 0x1Fu && get16(SS0, SP0 - 30u) == 0);
    c = world(0x42, 1, 0, 0, 0); put16(OV, 0x1160u, STUB2); ram[at(STUB2, 0x0052u)] = 0xEA; put16(STUB2, 0x0053u, 0x0E53u); put16(STUB2, 0x0055u, U9E);
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_CODE && c.ip == 0x115Du);

    /* Callee stops surface as CALLEE with the callee's state: an exhausted heap stops +001F's GetMem at RunError (AX = 203);
     * a zero loop-back cap stops 2199 at its loop head; a zero link cap stops the second append's tail walk at +0EA8 on a
     * list old -> node 1 (the first append followed no link: the old node's link was nil; the second must follow one). */
    c = world(0x42, 1, 0, 0, 0); put16(DS0, 0x440Au, HEAP);
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_CALLEE);
    CHECK(d.callee == 1 && d.callee_status == PR_U38_001F_CALLEE && d.d001f.append_status == PR_U9E_0E52_CALLEE && d.d001f.getmem_status == 2 && c.ax == 203u);
    c = world(0x42, 1, 0, 0, 0); lim.lookup_loopbacks = 0;
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_CALLEE);
    CHECK(d.callee == 4 && d.callee_status == PR_RES_2199_LIMIT && d.site == 0x1142u && c.cs == RES && c.ip == 0x21B9u);
    lim.lookup_loopbacks = 64;
    c = world(0x42, 1, 0, 1, 0x2Bu); lim.links = 0;
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_CALLEE);
    CHECK(d.callee == 5 && d.callee_status == PR_U9E_0E52_LIMIT && d.site == 0x115Du && c.cs == U9E && c.ip == 0x0EA8u && d.getmem_status == 0);
    lim.links = 64;

    /* The far pointer A is reread at each use: an A offset that wraps at 16 bits. The same without a diagnostic. */
    c = world(0, 5, 0x33, 0, 0); put16(SS0, SP0 + 6u, 0xFFFEu); put16(SS0, SP0 + 8u, ASEG + 0x0011u); ram[at(ASEG + 0x0011u, 0x0001u)] = 0x99;
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_OK && get16(SS0, SP0 - 26u) == 0x1299u);
    returned(&c);
    c = world(0x42, 1, 0, 0, 0);
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, NULL) == PR_U38_10DE_OK && ram[at(HEAP, 0x10u)] == 0x1Fu);
    returned(&c);

    /* Arguments: nothing changes. */
    c = world(0x42, 1, 0, 0, 0); before = c;
    CHECK(pr_u38_handler_10de_cpu(NULL, RAM, &c, &lim, &d) == PR_U38_10DE_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_handler_10de_cpu(ram, RAM - 1u, &c, &lim, &d) == PR_U38_10DE_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, NULL, &lim, &d) == PR_U38_10DE_ARGUMENT);
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, NULL, &d) == PR_U38_10DE_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    c.ip = 0x10DFu; before = c;
    CHECK(pr_u38_handler_10de_cpu(ram, RAM, &c, &lim, &d) == PR_U38_10DE_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(memcmp(ram, snap, RAM) == 0);
    for (i = 0; i < RAM; ++i)
        if (ram[i] != 0x5A && !(i >= at(OV, 0x10D2u) && i < at(OV, 0x10DEu)) && !(i >= at(OV, 0x111Eu) && i < at(OV, 0x1120u))
            && !(i >= at(OV, 0x1129u) && i < at(OV, 0x112Bu)) && !(i >= at(OV, 0x1145u) && i < at(OV, 0x1147u))
            && !(i >= at(OV, 0x1160u) && i < at(OV, 0x1162u)) && !(i >= at(OV, 0x004Au) && i < at(OV, 0x004Fu))
            && !(i >= at(STUB, 0x0052u) && i < at(STUB, 0x0057u)) && !(i >= at(U9E, 0x0E5Cu) && i < at(U9E, 0x0E61u))
            && !(i >= at(DS0, 0x4404u) && i < at(DS0, 0x4418u)) && i != at(ASEG, AOFF + 3u) && i != at(BSEG, BOFF + 0x10u)
            && !(i >= at(BSEG, BOFF + 0x7Fu) && i < at(BSEG, BOFF + 0x83u)) && !(i >= at(OSEG, OOFF) && i < at(OSEG, OOFF + 9u))
            && i != at(DS0, 0x681Fu) && !(i >= at(SS0, SP0) && i < at(SS0, SP0 + 14u)))
            break;
    CHECK(i == RAM);

    free(ram); free(snap);
    printf("PASS u38 10de: %u checks\n", checks);
    return 0;
}
