/* Focused synthetic test for raw-CPU U38 +0679 (docs/re/U38_STRING_MATCH_CPU.md), composing the merged raw-CPU SYSTEM
 * string compare. No game bytes: only the LCALL at +06CD, synthetic records and synthetic strings. */
#include "poolrad/recovered/overlay_u38_string_match_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 0679 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, CS0 = 0x3000, DS0 = 0x1000, SS0 = 0x2000, SP0 = 0x8000, A = 0x4000, B = 0x5000, P = 0x6000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t segment, uint16_t offset) { return (((size_t)segment << 4) + offset) & (RAM - 1u); }
static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    ram[at(segment, offset)] = (uint8_t)value;
    ram[at(segment, (uint16_t)(offset + 1u))] = (uint8_t)(value >> 8);
}
static uint16_t get16(uint16_t segment, uint16_t offset) { return (uint16_t)(ram[at(segment, offset)] | (ram[at(segment, (uint16_t)(offset + 1u))] << 8)); }
static void put_string(uint16_t segment, uint16_t offset, const char *text)
{
    size_t i, n = strlen(text);
    ram[at(segment, offset)] = (uint8_t)n;
    for (i = 0; i < n; ++i)
        ram[at(segment, (uint16_t)(offset + 1u + i))] = (uint8_t)text[i];
}

/* [BP+0Ch] = 4000:0100; its +108h = 5000:0200; that +0Ah = P = 6000:0300, P's string "ABC"; table i at DS:0332h + 16i. */
static pr_u38_string_match_cpu frame(uint8_t f9f, uint8_t f6c, const char *const table[4], uint8_t count)
{
    pr_u38_string_match_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, CS0, 0x0679u, DS0, 0x1111u, SS0, 0x7202u};
    static const uint16_t words[7] = {0x7000u, 0x7001u, 0x1111u, 0x2222u, 0x3333u, 0x0100u, A};
    unsigned i;
    memset(ram, 0xA5, RAM);
    ram[at(CS0, 0x06CDu)] = 0x9A;
    put16(CS0, 0x06CEu, 0x0C10u);
    put16(CS0, 0x06D0u, 0x0AF8u);
    for (i = 0; i < 7; ++i)
        put16(SS0, (uint16_t)(SP0 + 2u * i), words[i]);
    put16(A, 0x0208u, 0x0200u); put16(A, 0x020Au, B);
    put16(B, 0x020Au, 0x0300u); put16(B, 0x020Cu, P);
    put_string(P, 0x0300u, "ABC");
    ram[at(P, 0x039Fu)] = f9f;
    ram[at(P, 0x036Cu)] = f6c;
    for (i = 0; i < 4; ++i)
        put_string(DS0, (uint16_t)(0x0332u + 16u * (i + 1u)), table[i]);
    ram[at(DS0, 0x6822u)] = count;
    return cpu;
}

static void returned(const pr_u38_string_match_cpu *cpu)
{
    CHECK(cpu->cs == 0x7001u && cpu->ip == 0x7000u && cpu->sp == SP0 + 0x0Eu && cpu->bp == 0x7777u && cpu->ds == DS0 && cpu->ss == SS0);
    CHECK(get16(DS0, 0x6826u) == 0x0300u && get16(DS0, 0x6828u) == P);
}

int main(void)
{
    static const char *const two[4] = {"ABC", "ABD", "ABC", "AB"}, *const all[4] = {"ABC", "ABC", "ABC", "ABC"};
    pr_u38_string_match_cpu cpu;
    int callee = 99;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Two of four table strings equal P's: [6822h] += 2; the compare leaves AL/AH = the last lengths and DX = DS. */
    cpu = frame(1, 1, two, 5);
    CHECK(pr_u38_string_match_0679_cpu(ram, RAM, &cpu, 64, &callee) == PR_U38_STRING_MATCH_OK && callee == 0);
    CHECK(ram[at(DS0, 0x6822u)] == 7 && cpu.ax == 0x0203u && cpu.dx == DS0 && cpu.flags == 0x7246u);
    returned(&cpu);

    /* All four equal, from FEh: the byte wraps to 02h. */
    cpu = frame(1, 1, all, 0xFE);
    CHECK(pr_u38_string_match_0679_cpu(ram, RAM, &cpu, 64, &callee) == PR_U38_STRING_MATCH_OK && ram[at(DS0, 0x6822u)] == 0x02);
    returned(&cpu);

    /* Byte +9Fh not 1: no compare; CMP 0,1 leaves CF, AF, PF and SF; ES:DI = P, AX:DX = P's words. */
    cpu = frame(0, 1, all, 5);
    CHECK(pr_u38_string_match_0679_cpu(ram, RAM, &cpu, 64, &callee) == PR_U38_STRING_MATCH_OK && callee == -1);
    CHECK(ram[at(DS0, 0x6822u)] == 5 && cpu.flags == 0x7297u && cpu.es == P && cpu.di == 0x0300u && cpu.ax == 0x0300u && cpu.dx == P);
    returned(&cpu);

    /* Byte +6Ch not 1: CMP 2,1 clears every arithmetic flag. */
    cpu = frame(1, 2, all, 5);
    CHECK(pr_u38_string_match_0679_cpu(ram, RAM, &cpu, 64, &callee) == PR_U38_STRING_MATCH_OK && callee == -1);
    CHECK(ram[at(DS0, 0x6822u)] == 5 && cpu.flags == 0x7202u);
    returned(&cpu);

    /* Cap 0: one compare (a match), then LIMIT at the INC +06B3 with CMP 1,4's flags and the local frame live. */
    cpu = frame(1, 1, all, 0);
    CHECK(pr_u38_string_match_0679_cpu(ram, RAM, &cpu, 0, &callee) == PR_U38_STRING_MATCH_LIMIT && cpu.ip == 0x06B3u);
    CHECK(ram[at(DS0, 0x6822u)] == 1 && cpu.sp == SP0 - 4u && cpu.bp == SP0 - 2u && cpu.flags == 0x7293u && ram[at(SS0, SP0 - 3u)] == 1);

    /* CODE at +06CD: P and the first table string DS:0342h pushed, no return address. */
    cpu = frame(1, 1, all, 0);
    ram[at(CS0, 0x06CEu)] = 0x11;
    CHECK(pr_u38_string_match_0679_cpu(ram, RAM, &cpu, 64, &callee) == PR_U38_STRING_MATCH_CODE && cpu.ip == 0x06CDu);
    CHECK(cpu.sp == SP0 - 0x0Cu && get16(SS0, cpu.sp) == 0x0342u && get16(SS0, (uint16_t)(cpu.sp + 2u)) == DS0);
    CHECK(get16(SS0, (uint16_t)(cpu.sp + 4u)) == 0x0300u && get16(SS0, (uint16_t)(cpu.sp + 6u)) == P && ram[at(DS0, 0x6822u)] == 0);

    /* Guards change nothing. */
    cpu = frame(1, 1, all, 0);
    cpu.ip = 0x067Au;
    CHECK(pr_u38_string_match_0679_cpu(ram, RAM, &cpu, 64, &callee) == PR_U38_STRING_MATCH_ARGUMENT && callee == -1 && cpu.sp == SP0);
    CHECK(get16(DS0, 0x6826u) == 0xA5A5u);
    CHECK(pr_u38_string_match_0679_cpu(NULL, RAM, &cpu, 64, NULL) == PR_U38_STRING_MATCH_ARGUMENT);
    CHECK(pr_u38_string_match_0679_cpu(ram, RAM - 1u, &cpu, 64, NULL) == PR_U38_STRING_MATCH_ARGUMENT);

    free(ram);
    printf("u38 +0679: %u checks passed\n", checks);
    return 0;
}
