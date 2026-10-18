/* Focused synthetic test for raw-CPU U7D +0565 (docs/re/U7D_TWO_DIGITS_0565_CPU.md). No game bytes: only the LCALL
 * operands, the one-character constant '0', the power-of-ten constants and synthetic frames; results predicted by hand. */
#include "poolrad/recovered/overlay_u7d_two_digits_0565_cpu.h"
#include "poolrad/recovered/resident_00ba_byte_string_1037_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U7D_0565_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u7d 0565 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SYS = 0x0AF8, RES = 0x00BA, CS0 = 0x8800, SS = 0x2800, SP0 = 0x4000, BP0 = 0x7777, RET_IP = 0x7000, RET_CS = 0x9001,
       DS0 = 0x1357, RSEG = 0x5000, ROFF = 0x0400 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void put16(unsigned seg, unsigned off, unsigned value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, off + 1u)] = (uint8_t)(value >> 8);
}
static void lcall(unsigned seg, unsigned site, unsigned target_seg, unsigned target)
{
    ram[at(seg, site)] = 0x9A;
    put16(seg, site + 1u, target);
    put16(seg, site + 3u, target_seg);
}
static int pstring_is(unsigned seg, unsigned off, const char *text)
{
    size_t i, n = strlen(text);
    if (ram[at(seg, off)] != n)
        return 0;
    for (i = 0; i < n; ++i)
        if (ram[at(seg, off + 1u + (unsigned)i)] != (uint8_t)text[i])
            return 0;
    return 1;
}

/* The LCALLs, the constant '0' at +0563, the powers 10^9..1 at SYSTEM:12A6, and the frame b (high byte 99h), R. */
static pr_u7d_0565_cpu world(unsigned b)
{
    static const unsigned sites[7][2] = {{0x0576, 0x1037}, {0x0584, 0x0B25}, {0x059A, 0x0B0B}, {0x05A4, 0x0B98},
                                         {0x05B2, 0x0B25}, {0x05CA, 0x0B57}, {0x05D8, 0x0B25}};
    uint32_t power = 1000000000u;
    pr_u7d_0565_cpu cpu;
    size_t i;
    memset(ram, 0xA5, RAM);
    for (i = 0; i < 7u; ++i)
        lcall(CS0, sites[i][0], sites[i][1] == 0x1037u ? RES : SYS, sites[i][1]);
    ram[at(CS0, 0x0563)] = 1;
    ram[at(CS0, 0x0564)] = '0';
    lcall(RES, 0x1054, SYS, 0x1366);
    lcall(RES, 0x1068, SYS, 0x0B25);
    for (i = 0; i < 10u; ++i, power /= 10u) {
        put16(SYS, 0x12A6u + 4u * (unsigned)i, power & 0xFFFFu);
        put16(SYS, 0x12A8u + 4u * (unsigned)i, power >> 16);
    }
    put16(SS, SP0, RET_IP); put16(SS, SP0 + 2u, RET_CS); put16(SS, SP0 + 4u, 0x9900u | b);
    put16(SS, SP0 + 6u, ROFF); put16(SS, SP0 + 8u, RSEG);
    memset(&cpu, 0, sizeof cpu);
    cpu.ax = 0x1111; cpu.bx = 0x2222; cpu.cx = 0x3333; cpu.dx = 0x4444; cpu.si = 0x5555; cpu.di = 0x6666; cpu.bp = BP0;
    cpu.sp = SP0; cpu.cs = CS0; cpu.ip = 0x0565; cpu.ds = DS0; cpu.es = 0x9999; cpu.ss = SS; cpu.flags = 0x0602;
    return cpu;
}

/* b as two characters: R and the locals T = SS:SP0-12Bh and s = SS:SP0-2Bh, with the last StrStore's registers. */
static void two_digits(unsigned b, const char *expected, const char *s_text)
{
    pr_u7d_0565_cpu cpu = world(b);
    pr_u7d_0565_callee callee;
    int status;
    CHECK(pr_u7d_two_digits_0565_cpu(ram, RAM, &cpu, &callee, &status) == PR_U7D_0565_OK);
    CHECK(callee == PR_U7D_0565_STRSTORE && status == 0);
    CHECK(pstring_is(RSEG, ROFF, expected) && pstring_is(SS, SP0 - 0x12Bu, expected) && pstring_is(SS, SP0 - 0x2Bu, s_text));
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 6u && cpu.bp == BP0 && cpu.ds == DS0 && cpu.es == RSEG);
    CHECK(cpu.ax == 0x0002 && cpu.cx == 0 && cpu.di == ROFF + 3u && cpu.si == SP0 - 0x128u && cpu.bx == SP0 - 0x13Au && cpu.dx == DS0);
    CHECK(cpu.flags == 0x0246);
}

int main(void)
{
    pr_u7d_0565_cpu cpu;
    pr_u7d_0565_callee callee;
    int status;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    two_digits(7, "07", "07");
    two_digits(0, "00", "00");
    two_digits(9, "09", "09");
    two_digits(10, "10", "10");
    two_digits(42, "42", "42");
    two_digits(240, "24", "240");
    two_digits(255, "25", "255");

    /* CODE at the Copy LCALL (b = 42): stopped at +05CA with T, s, 1 and 2 pushed; CMP 42,10 = 20h leaves no flag;
     * DF was cleared by the first StrStore. */
    cpu = world(42);
    ram[at(CS0, 0x05CA)] = 0xEA;
    CHECK(pr_u7d_two_digits_0565_cpu(ram, RAM, &cpu, &callee, &status) == PR_U7D_0565_CODE);
    CHECK(callee == PR_U7D_0565_STRSTORE && status == 0 && cpu.ip == 0x05CA && cpu.cs == CS0 && cpu.sp == SP0 - 0x138u);
    CHECK(cpu.ax == 2 && cpu.flags == 0x0202 && ram[at(RSEG, ROFF)] == 0xA5);

    /* CALLEE: Str rejects an altered power table inside 00BA:1037; stopped at Str's entry. */
    cpu = world(7);
    put16(SYS, 0x12CA, 2);
    CHECK(pr_u7d_two_digits_0565_cpu(ram, RAM, &cpu, &callee, &status) == PR_U7D_0565_CALLEE);
    CHECK(callee == PR_U7D_0565_BYTE_STRING && status == PR_RES_1037_CALLEE && cpu.cs == SYS && cpu.ip == 0x1366);

    /* Guards. */
    cpu = world(7);
    cpu.ip = 0x0566;
    CHECK(pr_u7d_two_digits_0565_cpu(ram, RAM, &cpu, &callee, &status) == PR_U7D_0565_ARGUMENT && callee == PR_U7D_0565_NO_CALLEE);
    CHECK(status == -1 && cpu.ip == 0x0566 && cpu.sp == SP0);
    CHECK(pr_u7d_two_digits_0565_cpu(NULL, RAM, &cpu, NULL, NULL) == PR_U7D_0565_ARGUMENT);
    CHECK(pr_u7d_two_digits_0565_cpu(ram, RAM - 1u, &cpu, NULL, NULL) == PR_U7D_0565_ARGUMENT);

    free(ram);
    printf("u7d +0565: %u checks passed\n", checks);
    return 0;
}
