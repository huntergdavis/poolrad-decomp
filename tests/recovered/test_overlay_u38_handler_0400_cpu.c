/* Focused synthetic test for GAME.OVR unit 38h +0400, the 009E slot-11 handler (docs/re/OVL_0038_0400_CPU.md). No game
 * bytes: only the relocated far-call segment word and synthetic records, party list and DS bytes. */
#include "poolrad/recovered/overlay_u38_handler_0400_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 0400 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, OV = 0x5000, RES = 0x10BA, DS0 = 0x1C7C, SS0 = 0x7000, SP0 = 0xF000,
       ASEG = 0x3000, AOFF = 0x0100, BSEG = 0x3400, BOFF = 0x0200, CSEG = 0x3800, COFF = 0x0300, PSEG = 0x4000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* Records A and B, B+108h -> C, a two-character party (+10Dh 1, +10Eh 0 and 1) for 23DB, and the call frame. */
static pr_u38_0400_cpu world(uint16_t sel, uint8_t a3, uint8_t b10e, uint8_t b84)
{
    pr_u38_0400_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, OV, 0x0400u, DS0, 0x1111u, SS0, 0x0002u};
    memset(ram, 0x5A, RAM);
    put16(OV, 0x04AFu, RES);
    ram[at(ASEG, AOFF + 3u)] = a3;
    ram[at(BSEG, BOFF + 0x10Eu)] = b10e; ram[at(BSEG, BOFF + 0x10Fu)] = 0x22; ram[at(BSEG, BOFF + 0x84u)] = b84;
    put16(BSEG, BOFF + 0x108u, COFF); put16(BSEG, BOFF + 0x10Au, CSEG);
    put16(CSEG, COFF + 0x0Au, 0xAAAAu); put16(CSEG, COFF + 0x0Cu, 0xBBBBu);
    put16(DS0, 0x5D96u, 0x0010u); put16(DS0, 0x5D98u, PSEG);
    ram[at(PSEG, 0x0010u + 0x10Du)] = 1; ram[at(PSEG, 0x0010u + 0x10Eu)] = 0;
    put16(PSEG, 0x0010u + 0x104u, 0x0010u); put16(PSEG, 0x0010u + 0x106u, PSEG + 0x20u);
    ram[at(PSEG + 0x20u, 0x0010u + 0x10Du)] = 1; ram[at(PSEG + 0x20u, 0x0010u + 0x10Eu)] = 1;
    put16(PSEG + 0x20u, 0x0010u + 0x104u, 0); put16(PSEG + 0x20u, 0x0010u + 0x106u, 0);
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    put16(SS0, SP0 + 4u, sel); put16(SS0, SP0 + 6u, AOFF); put16(SS0, SP0 + 8u, ASEG);
    put16(SS0, SP0 + 10u, BOFF); put16(SS0, SP0 + 12u, BSEG);
    return c;
}

static void returned(const pr_u38_0400_cpu *c)
{
    CHECK(c->cs == 0x9001u && c->ip == 0x7000u && c->sp == SP0 + 14u && c->bp == 0x7777u);
}

int main(void)
{
    pr_u38_0400_cpu c;
    int callee;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Selector set: B+10Eh := bit 6 of A+3; B+84h B3h -> 0. Nothing else moves; 23DB does not run. */
    c = world(1, 0x40, 0x09, 0xB3);
    CHECK(pr_u38_handler_0400_cpu(ram, RAM, &c, 1000, &callee) == PR_U38_0400_OK && callee == -1);
    CHECK(ram[at(BSEG, BOFF + 0x10Eu)] == 1 && ram[at(BSEG, BOFF + 0x84u)] == 0 && ram[at(DS0, 0x6825u)] == 0x5A);
    returned(&c);
    /* Bit 6 clear gives 0; B+84h other than B3h is kept. */
    c = world(0x00FF, 0xBF, 0x09, 0x12);
    CHECK(pr_u38_handler_0400_cpu(ram, RAM, &c, 1000, &callee) == PR_U38_0400_OK);
    CHECK(ram[at(BSEG, BOFF + 0x10Eu)] == 0 && ram[at(BSEG, BOFF + 0x84u)] == 0x12);

    /* Selector zero (only the low byte counts), A+3 bit 5 set: only DS:6825 := 64h. */
    c = world(0x5A00, 0x20, 0x01, 0x10);
    CHECK(pr_u38_handler_0400_cpu(ram, RAM, &c, 1000, &callee) == PR_U38_0400_OK && callee == -1);
    CHECK(ram[at(DS0, 0x6825u)] == 0x64 && ram[at(ASEG, AOFF + 3u)] == 0x20 && ram[at(BSEG, BOFF + 0x10Fu)] == 0x22);
    returned(&c);

    /* The fold: A+3 := 05 + 20 + (1 << 6) = 65h; B+10Eh := 65h >> 7 = 0; B+10Fh := 1; B+84h 10h <= 7Fh -> B3h; C+0Ah/+0Ch := 0;
     * 23DB counts the two characters into DS:6814 and DS:6815; DS:6825 := 64h. */
    c = world(0, 0x05, 0x01, 0x10);
    CHECK(pr_u38_handler_0400_cpu(ram, RAM, &c, 1000, &callee) == PR_U38_0400_OK && callee == PR_RES_23DB_RETURNED);
    CHECK(ram[at(ASEG, AOFF + 3u)] == 0x65 && ram[at(BSEG, BOFF + 0x10Eu)] == 0 && ram[at(BSEG, BOFF + 0x10Fu)] == 1);
    CHECK(ram[at(BSEG, BOFF + 0x84u)] == 0xB3 && get16(CSEG, COFF + 0x0Au) == 0 && get16(CSEG, COFF + 0x0Cu) == 0);
    CHECK(ram[at(DS0, 0x6814u)] == 1 && ram[at(DS0, 0x6815u)] == 1 && ram[at(DS0, 0x6825u)] == 0x64);
    returned(&c);
    /* B+10Eh FFh is CBW -1: 05 + 20 + FFC0h -> E5h, whose bit 7 makes B+10Eh 1; B+84h 80h is above 7Fh and kept. */
    c = world(0, 0x05, 0xFF, 0x80);
    CHECK(pr_u38_handler_0400_cpu(ram, RAM, &c, 1000, &callee) == PR_U38_0400_OK);
    CHECK(ram[at(ASEG, AOFF + 3u)] == 0xE5 && ram[at(BSEG, BOFF + 0x10Eu)] == 1 && ram[at(BSEG, BOFF + 0x84u)] == 0x80);

    /* A cyclic party list exhausts 23DB's budget: CALLEE with its LIMIT status; DS:6825 is not written. */
    c = world(0, 0x05, 0x01, 0x10);
    put16(PSEG + 0x20u, 0x0010u + 0x104u, 0x0010u); put16(PSEG + 0x20u, 0x0010u + 0x106u, PSEG);
    CHECK(pr_u38_handler_0400_cpu(ram, RAM, &c, 200, &callee) == PR_U38_0400_CALLEE && callee == PR_RES_23DB_LIMIT);
    CHECK(ram[at(DS0, 0x6825u)] == 0x5A);

    /* Bad arguments change nothing. */
    c = world(1, 0x40, 0x09, 0xB3);
    CHECK(pr_u38_handler_0400_cpu(ram, RAM - 1u, &c, 1000, &callee) == PR_U38_0400_ARGUMENT && c.ip == 0x0400u && c.sp == SP0);
    c.ip = 0x0401u;
    CHECK(pr_u38_handler_0400_cpu(ram, RAM, &c, 1000, NULL) == PR_U38_0400_ARGUMENT && ram[at(BSEG, BOFF + 0x10Eu)] == 0x09);

    free(ram);
    printf("u38 0400: %u checks\n", checks);
    return 0;
}
