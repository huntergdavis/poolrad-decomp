/* Focused synthetic test for GAME.OVR unit 38h +0775, the 009E slot-22 handler (docs/re/OVL_0038_0775_CPU.md). No game
 * bytes: only the three relocated far-call segment words, the loaded-stub form of unit 9E entry 2, U9E +0026's checked
 * near-call binding, a synthetic source string and synthetic records, lists and DS bytes. */
#include "poolrad/recovered/overlay_u38_handler_0775_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 0775 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, OV = 0x5000, RES = 0x10BA, SYS = 0x0AF8, STUB = 0x00B0, U9E = 0x6000, DS0 = 0x1C7C,
       SS0 = 0x7000, SP0 = 0xF000, ASEG = 0x3000, AOFF = 0x0100, BSEG = 0x3400, BOFF = 0x0200, NSEG = 0x3800, NOFF = 0x0300 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* The call frame, the fixup words, the loaded stub, U9E +0026's near-call binding, a 5-character source string at CS:0764,
 * record B with an empty list at +7Fh/+81h (the node is linked by the cases that need one). */
static pr_u38_0775_cpu world(void)
{
    pr_u38_0775_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, OV, 0x0775u, DS0, 0x1111u, SS0, 0x0002u};
    memset(ram, 0x5A, RAM);
    put16(OV, 0x078Cu, RES); put16(OV, 0x07A8u, SYS); put16(OV, 0x07C5u, STUB);
    ram[at(STUB, 0x002Au)] = 0xEA; put16(STUB, 0x002Bu, 0x0026u); put16(STUB, 0x002Du, U9E);
    ram[at(U9E, 0x00A1u)] = 0xE8; put16(U9E, 0x00A2u, 0xFF5Cu);
    memcpy(ram + at(OV, 0x0764u), "\x05HELLO", 6);
    put16(BSEG, BOFF + 0x7Fu, 0); put16(BSEG, BOFF + 0x81u, 0);
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, 0x5A5Au); put16(SS0, SP0 + 6u, AOFF); put16(SS0, SP0 + 8u, ASEG);
    put16(SS0, SP0 + 10u, BOFF); put16(SS0, SP0 + 12u, BSEG);
    return c;
}

/* One node of the given type at NSEG:NOFF, byte +4 active, no successor, linked as the head of B's list. */
static void node(uint8_t type, uint8_t active)
{
    ram[at(NSEG, NOFF)] = type; ram[at(NSEG, NOFF + 4u)] = active;
    put16(NSEG, NOFF + 5u, 0); put16(NSEG, NOFF + 7u, 0);
    put16(BSEG, BOFF + 0x7Fu, NOFF); put16(BSEG, BOFF + 0x81u, NSEG);
}

static void returned(const pr_u38_0775_cpu *c)
{
    CHECK(c->cs == 0x9001u && c->ip == 0x7000u && c->sp == SP0 + 14u && c->bp == 0x7777u);
}

int main(void)
{
    pr_u38_0775_cpu c, before;
    pr_u38_0775_limits lim = {16, {64, 64}};
    pr_u38_0775_diagnostic d;
    unsigned i;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Not found (empty list): 2199 answers 0, no message, DS:681F written 1 then 0 around U9E +0026, which searches the
     * empty list for type 0Fh, finds nothing and returns; RETF 0Ah. The byte and A on the frame are never read. */
    c = world();
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_OK);
    CHECK(d.callee == 3 && d.callee_status == PR_U9E_0026_OK && d.site == 0x07C2u && d.d0026.freemem_status == -1);
    CHECK(ram[at(DS0, 0x681Fu)] == 0 && ram[at(SS0, SP0 - 23u)] == 0x5A);
    CHECK(get16(BSEG, BOFF + 0x7Fu) == 0 && get16(BSEG, BOFF + 0x81u) == 0);
    returned(&c);
    /* The same without a diagnostic. */
    c = world();
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, NULL) == PR_U38_0775_OK);
    returned(&c);
    /* A node of another type (not 37h, not 0Fh): neither callee matches; the list is untouched. */
    c = world(); node(0x38, 1);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_OK && d.callee == 3);
    CHECK(get16(BSEG, BOFF + 0x7Fu) == NOFF && get16(BSEG, BOFF + 0x81u) == NSEG && ram[at(DS0, 0x681Fu)] == 0);
    returned(&c);

    /* Found (a node of type 37h): StrLoad copies the 5-character string into SS:BP-15h, then the body stops PENDING at
     * +07AB before CALL +0058 with CS and the five words (B, AX = AH:06h, SS:BP-15h) pushed. DS:681F is not written. */
    c = world(); node(0x37, 0);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_PENDING);
    CHECK(d.callee == 4 && d.site == 0x07ABu && c.cs == OV && c.ip == 0x07ABu && c.bp == SP0 - 2u && c.sp == SP0 - 36u);
    CHECK(get16(SS0, SP0 - 36u) == OV && get16(SS0, SP0 - 34u) == SP0 - 23u && get16(SS0, SP0 - 32u) == SS0);
    CHECK((get16(SS0, SP0 - 30u) & 0xFFu) == 0x06u && get16(SS0, SP0 - 28u) == BOFF && get16(SS0, SP0 - 26u) == BSEG);
    CHECK(memcmp(ram + at(SS0, SP0 - 23u), "\x05HELLO", 6) == 0 && ram[at(SS0, SP0 - 17u)] == 0x5A);
    /* StrLoad leaves AL the length, ES:DI past the copied bytes, CX 0 and DS restored. */
    CHECK((c.ax & 0xFFu) == 5u && c.es == SS0 && c.di == SP0 - 17u && c.cx == 0 && c.ds == DS0 && ram[at(DS0, 0x681Fu)] == 0x5A);
    CHECK(get16(SS0, SP0 - 6u) == NOFF && get16(SS0, SP0 - 4u) == NSEG); /* 2199 left the node in the cursor cell [BP-4] */
    /* A found node that is also type 37h in a two-node list: the key is matched on the second node. */
    c = world(); node(0x12, 0);
    put16(NSEG, NOFF + 5u, NOFF + 0x20u); put16(NSEG, NOFF + 7u, NSEG);
    ram[at(NSEG, NOFF + 0x20u)] = 0x37; put16(NSEG, NOFF + 0x25u, 0); put16(NSEG, NOFF + 0x27u, 0);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_PENDING && d.callee == 4);
    CHECK(get16(SS0, SP0 - 6u) == NOFF + 0x20u && get16(SS0, SP0 - 4u) == NSEG);

    /* Not found, but U9E +0026 finds a node of type 0Fh whose byte +4 is set: it stops PENDING at its own dispatcher
     * boundary; reported as CALLEE with DS:681F still 1 and the CPU as +0026 left it. */
    c = world(); node(0x0F, 1);
    put16(DS0, 0x6828u + 4u * 0x0Fu, 0x1234u); put16(DS0, 0x682Au + 4u * 0x0Fu, 0x6543u);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_CALLEE);
    CHECK(d.callee == 3 && d.callee_status == PR_U9E_0026_PENDING && d.site == 0x07C2u);
    CHECK(d.d0026.handler_offset == 0x1234u && d.d0026.handler_segment == 0x6543u && d.d0026.freemem_status == -1);
    CHECK(c.cs == U9E && c.ip == 0x001Eu && ram[at(DS0, 0x681Fu)] == 1);
    CHECK(get16(SS0, SP0 - 34u) == 0 && get16(SS0, SP0 - 32u) == 0 && (get16(SS0, SP0 - 30u) & 0xFFu) == 0x0Fu);
    CHECK(get16(SS0, SP0 - 28u) == BOFF && get16(SS0, SP0 - 26u) == BSEG && get16(SS0, SP0 - 38u) == 0x07C7u && get16(SS0, SP0 - 36u) == OV);

    /* The loaded stub is not a far JMP to +0026: CODE at +07C2 with the five words pushed and nothing for the call. */
    c = world(); ram[at(STUB, 0x002Au)] = 0xCD;
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_CODE);
    CHECK(d.site == 0x07C2u && d.callee == 1 && c.cs == OV && c.ip == 0x07C2u && c.sp == SP0 - 34u && ram[at(DS0, 0x681Fu)] == 1);
    c = world(); put16(STUB, 0x002Bu, 0x0027u);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_CODE && c.ip == 0x07C2u);

    /* A cyclic list without the key exhausts 2199's loop-back cap: CALLEE with its LIMIT, at its loop head. */
    c = world(); node(0x38, 0); put16(NSEG, NOFF + 5u, NOFF); put16(NSEG, NOFF + 7u, NSEG);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_CALLEE);
    CHECK(d.callee == 1 && d.callee_status == PR_RES_2199_LIMIT && d.site == 0x0789u && c.cs == RES && c.ip == 0x21B9u);
    CHECK(ram[at(DS0, 0x681Fu)] == 0x5A);

    /* AH is never cleared: the key word pushed for 2199 is entry AH:37h (2199 reads only its low byte), and the AX word
     * left for +0058 carries 2199's own AH (the high byte of its last OR, 0300h | 3800h = 3B00h) over 06h. */
    c = world(); node(0x37, 0); c.ax = 0x12FFu;
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_PENDING);
    CHECK(get16(SS0, SP0 - 30u) == 0x3B06u);

    /* The frame's far pointers are reread at each use: a frame whose B offset wraps at 16 bits. */
    c = world(); put16(SS0, SP0 + 10u, 0xFFF0u); put16(SS0, SP0 + 12u, BSEG + 0x0021u);
    put16(BSEG + 0x0021u, (uint16_t)(0xFFF0u + 0x7Fu), 0); put16(BSEG + 0x0021u, (uint16_t)(0xFFF0u + 0x81u), 0);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_OK);
    returned(&c);

    /* Arguments: nothing changes. */
    c = world(); before = c;
    CHECK(pr_u38_handler_0775_cpu(NULL, RAM, &c, &lim, &d) == PR_U38_0775_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM - 1u, &c, &lim, &d) == PR_U38_0775_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, NULL, &lim, &d) == PR_U38_0775_ARGUMENT);
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, NULL, &d) == PR_U38_0775_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    c.ip = 0x0776u; before = c;
    CHECK(pr_u38_handler_0775_cpu(ram, RAM, &c, &lim, &d) == PR_U38_0775_ARGUMENT && memcmp(&c, &before, sizeof c) == 0);
    for (i = 0; i < RAM; ++i)
        if (ram[i] != 0x5A && !(i >= at(OV, 0x0764u) && i < at(OV, 0x076Au)) && !(i >= at(OV, 0x078Cu) && i < at(OV, 0x078Eu))
            && !(i >= at(OV, 0x07A8u) && i < at(OV, 0x07AAu)) && !(i >= at(OV, 0x07C5u) && i < at(OV, 0x07C7u))
            && !(i >= at(STUB, 0x002Au) && i < at(STUB, 0x002Fu)) && !(i >= at(U9E, 0x00A1u) && i < at(U9E, 0x00A4u))
            && !(i >= at(BSEG, BOFF + 0x7Fu) && i < at(BSEG, BOFF + 0x83u)) && !(i >= at(SS0, SP0) && i < at(SS0, SP0 + 14u)))
            break;
    CHECK(i == RAM);

    free(ram);
    printf("PASS u38 0775: %u checks\n", checks);
    return 0;
}
