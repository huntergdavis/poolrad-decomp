/* Focused synthetic test for raw-CPU SYSTEM Text Assign 0AF8:04F7 and Insert 0AF8:0C68
 * (docs/re/SYSTEM_TEXTASSIGN_INSERT.md). No game bytes: synthetic strings and records, every result predicted by hand. */
#include "poolrad/recovered/system_textassign_insert_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_LEAF_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "textassign/insert line %d: %s\n", __LINE__, #x);     \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SYS = 0x0AF8, SS = 0x2800, SP0 = 0x4000, RET_IP = 0x7000, RET_CS = 0x9001, DS0 = 0x1357, SSEG = 0x4000, TSEG = 0x5000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static unsigned get16(unsigned seg, unsigned off) { return ram[at(seg, off)] | ((unsigned)ram[at(seg, off + 1u)] << 8); }
static void put16(unsigned seg, unsigned off, unsigned value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, off + 1u)] = (uint8_t)(value >> 8);
}
static void pstring(unsigned seg, unsigned off, const char *text)
{
    ram[at(seg, off)] = (uint8_t)strlen(text);
    memcpy(ram + at(seg, off + 1u), text, strlen(text));
}
static int pstring_is(unsigned seg, unsigned off, const char *text)
{
    return ram[at(seg, off)] == strlen(text) && memcmp(ram + at(seg, off + 1u), text, strlen(text)) == 0;
}
static int zeros(unsigned seg, unsigned off, unsigned count)
{
    unsigned i;
    for (i = 0; i < count; ++i)
        if (ram[at(seg, off + i)] != 0)
            return 0;
    return 1;
}

static pr_system_leaf_cpu frame(uint16_t ip, const unsigned *words, size_t count)
{
    pr_system_leaf_cpu cpu;
    size_t i;
    memset(ram, 0xA5, RAM);
    put16(SS, SP0, RET_IP); put16(SS, SP0 + 2u, RET_CS);
    for (i = 0; i < count; ++i)
        put16(SS, SP0 + 4u + 2u * (unsigned)i, words[i]);
    memset(&cpu, 0, sizeof cpu);
    cpu.ax = 0x1111; cpu.bx = 0x2222; cpu.cx = 0x3333; cpu.dx = 0x4444; cpu.si = 0x5555; cpu.di = 0x6666; cpu.bp = 0x7777;
    cpu.sp = SP0; cpu.cs = SYS; cpu.ip = ip; cpu.ds = DS0; cpu.es = 0x9999; cpu.ss = SS; cpu.flags = 0x0602;
    return cpu;
}

/* Insert(Source "ABC" at 5000:0300, S "HELLO" at 4000:0200 with maximum M, Index). */
static pr_system_leaf_cpu insert(unsigned index, unsigned maximum)
{
    const unsigned words[6] = {index, maximum, 0x0200, SSEG, 0x0300, TSEG};
    pr_system_leaf_cpu cpu = frame(0x0C68, words, 6);
    pstring(SSEG, 0x0200, "HELLO");
    pstring(TSEG, 0x0300, "ABC");
    CHECK(pr_system_insert_0c68(ram, RAM, &cpu) == PR_SYSTEM_INSERT_RETURNED);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 16u && cpu.bp == 0x7777 && cpu.ds == DS0 && cpu.es == SSEG);
    CHECK(cpu.cx == 0 && cpu.flags == 0x0246 && pstring_is(TSEG, 0x0300, "ABC"));
    return cpu;
}

int main(void)
{
    pr_system_leaf_cpu cpu;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Text Assign: the TextRec words, the buffer pointer T+80h (DI 010Ch + 74h), the open routine CS:060E, 28 zero
     * bytes, then "PHLAN" and a NUL at T+48. */
    {
        const unsigned words[4] = {0x0200, SSEG, 0x0100, TSEG};
        cpu = frame(0x04F7, words, 4);
        pstring(SSEG, 0x0200, "PHLAN");
        CHECK(pr_system_text_assign_04f7(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        CHECK(get16(TSEG, 0x0100) == 0 && get16(TSEG, 0x0102) == 0xD7B0 && get16(TSEG, 0x0104) == 0x0080 && zeros(TSEG, 0x0106, 6));
        CHECK(get16(TSEG, 0x010C) == 0x0180 && get16(TSEG, 0x010E) == TSEG && get16(TSEG, 0x0110) == 0x060E && get16(TSEG, 0x0112) == SYS);
        CHECK(zeros(TSEG, 0x0114, 28) && memcmp(ram + at(TSEG, 0x0130), "PHLAN\0", 6) == 0);
        CHECK(cpu.ax == 0 && cpu.cx == 0 && cpu.di == 0x0136 && cpu.si == 0x0206 && cpu.bx == SP0 && cpu.es == TSEG && cpu.ds == DS0);
        CHECK(cpu.sp == SP0 + 12u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.flags == 0x0246 && get16(SS, SP0 - 2u) == DS0);
    }

    /* Insert "ABC" into "HELLO" at 3: T1 = "HE" + "ABC" + "LLO" in SS:SP0-102h, T2 = "LLO" in SS:SP0-202h; StrStore leaves
     * AX = 8 and DI past S. */
    cpu = insert(3, 255);
    CHECK(pstring_is(SSEG, 0x0200, "HEABCLLO") && pstring_is(SS, SP0 - 0x102u, "HEABCLLO") && pstring_is(SS, SP0 - 0x202u, "LLO"));
    CHECK(cpu.ax == 0x0008 && cpu.di == 0x0209);
    /* Index 0: Copy's negative count gives an empty head and Copy(S, 0, 255) the whole of S, so Source goes first. */
    cpu = insert(0, 255);
    CHECK(pstring_is(SSEG, 0x0200, "ABCHELLO"));
    /* Index past the end appends; M = 6 truncates. */
    cpu = insert(10, 255);
    CHECK(pstring_is(SSEG, 0x0200, "HELLOABC"));
    cpu = insert(3, 6);
    CHECK(pstring_is(SSEG, 0x0200, "HEABCL") && cpu.ax == 0x0006);

    /* Guards. */
    {
        const unsigned words[1] = {0};
        pr_system_leaf_cpu before;
        cpu = frame(0x0C69, words, 1);
        before = cpu;
        CHECK(pr_system_insert_0c68(ram, RAM, &cpu) == PR_SYSTEM_INSERT_ARGUMENT && memcmp(&cpu, &before, sizeof cpu) == 0);
        CHECK(pr_system_insert_0c68(NULL, RAM, &cpu) == PR_SYSTEM_INSERT_ARGUMENT);
        CHECK(pr_system_text_assign_04f7(ram, RAM - 1u, &cpu) == PR_SYSTEM_LEAF_ARGUMENT && pr_system_text_assign_04f7(ram, RAM, NULL) == PR_SYSTEM_LEAF_ARGUMENT);
    }

    free(ram);
    printf("textassign/insert: %u checks passed\n", checks);
    return 0;
}
