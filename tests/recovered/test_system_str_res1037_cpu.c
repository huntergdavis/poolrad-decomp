/* Focused synthetic test for raw-CPU SYSTEM Str 0AF8:1366 (with its core 1254) and resident 00BA:1037
 * (docs/re/SYSTEM_STR_1366_RES_1037.md). No game bytes: the power-of-ten constants, two LCALL operands and synthetic
 * frames; every result predicted by hand. */
#include "poolrad/recovered/resident_00ba_byte_string_1037_cpu.h"
#include "poolrad/recovered/system_str_1366_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_LEAF_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "str/1037 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SYS = 0x0AF8, RES = 0x00BA, SS = 0x2800, SP0 = 0x4000, BP0 = 0x7777, RET_IP = 0x7000, RET_CS = 0x9001, DS0 = 0x1357,
       RSEG = 0x5000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static unsigned get16(unsigned seg, unsigned off) { return ram[at(seg, off)] | ((unsigned)ram[at(seg, off + 1u)] << 8); }
static void put16(unsigned seg, unsigned off, unsigned value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, off + 1u)] = (uint8_t)(value >> 8);
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

/* A fresh world: the ten powers 10^9..1 at SYSTEM:12A6, 1037's two LCALL operands, and the frame RET, words at SS:SP0. */
static void world(const unsigned *words, size_t count)
{
    static const uint8_t str_call[5] = {0x9A, 0x66, 0x13, 0xF8, 0x0A}, store_call[5] = {0x9A, 0x25, 0x0B, 0xF8, 0x0A};
    uint32_t power = 1000000000u;
    size_t i;
    memset(ram, 0xA5, RAM);
    for (i = 0; i < 10u; ++i, power /= 10u) {
        put16(SYS, 0x12A6u + 4u * (unsigned)i, power & 0xFFFFu);
        put16(SYS, 0x12A8u + 4u * (unsigned)i, power >> 16);
    }
    memcpy(ram + at(RES, 0x1054), str_call, sizeof str_call);
    memcpy(ram + at(RES, 0x1068), store_call, sizeof store_call);
    put16(SS, SP0, RET_IP); put16(SS, SP0 + 2u, RET_CS);
    for (i = 0; i < count; ++i)
        put16(SS, SP0 + 4u + 2u * (unsigned)i, words[i]);
}

static void registers(uint16_t *ax, uint16_t *bx, uint16_t *cx, uint16_t *dx, uint16_t *si, uint16_t *di)
{
    *ax = 0x1111; *bx = 0x2222; *cx = 0x3333; *dx = 0x4444; *si = 0x5555; *di = 0x6666;
}

/* Str(value : width, R at 5000:0100, M) from a fresh world. */
static pr_system_leaf_cpu str(int32_t value, unsigned width, unsigned maximum)
{
    uint32_t x = (uint32_t)value;
    const unsigned words[6] = {maximum, 0x0100, RSEG, width & 0xFFFFu, x & 0xFFFFu, x >> 16};
    pr_system_leaf_cpu cpu;
    world(words, 6);
    memset(&cpu, 0, sizeof cpu);
    registers(&cpu.ax, &cpu.bx, &cpu.cx, &cpu.dx, &cpu.si, &cpu.di);
    cpu.bp = BP0; cpu.sp = SP0; cpu.cs = SYS; cpu.ip = 0x1366; cpu.ds = DS0; cpu.es = 0x9999; cpu.ss = SS; cpu.flags = 0x0602;
    CHECK(pr_system_str_1366(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 16u && cpu.bp == BP0 && cpu.ds == DS0 && cpu.es == RSEG);
    CHECK(cpu.bx == 0 && cpu.cx == 0 && cpu.dx == maximum);
    return cpu;
}

int main(void)
{
    pr_system_leaf_cpu cpu;
    pr_res_1037_cpu res;
    pr_res_1037_callee callee;
    int status;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Str(12345 : 0): five digits in the temporary SS:SP0-22h, no padding (SUB 5,5: ZF, PF), DF cleared. Below the
     * temporary, PUSH DS left DS over the call's return word and PUSH SS left SS over the core's saved DI. */
    cpu = str(12345, 0, 255);
    CHECK(pstring_is(RSEG, 0x0100, "12345") && cpu.ax == 0 && cpu.di == 0x0106 && cpu.si == SP0 - 0x1Du && cpu.flags == 0x0246);
    CHECK(memcmp(ram + at(SS, SP0 - 0x22u), "12345", 5) == 0 && get16(SS, SP0 - 0x24u) == DS0 && get16(SS, SP0 - 0x26u) == SS);
    /* Str(-42 : 6): three spaces of padding; AX = (3 & FF00h) | ' '; SUB 6,3 leaves PF; PUSH CX left the count. */
    cpu = str(-42, 6, 255);
    CHECK(pstring_is(RSEG, 0x0100, "   -42") && cpu.ax == 0x0020 && cpu.di == 0x0107 && cpu.flags == 0x0206);
    CHECK(get16(SS, SP0 - 0x26u) == 3);
    /* The edges of the power table and the sign. */
    cpu = str(0, 0, 255);
    CHECK(pstring_is(RSEG, 0x0100, "0"));
    cpu = str((int32_t)0x80000000u, 0, 255);
    CHECK(pstring_is(RSEG, 0x0100, "-2147483648"));
    cpu = str(1000000000, 0, 255);
    CHECK(pstring_is(RSEG, 0x0100, "1000000000"));
    cpu = str(999999999, 0, 255);
    CHECK(pstring_is(RSEG, 0x0100, "999999999"));
    /* M = 3 keeps the first three digits. */
    cpu = str(12345, 0, 3);
    CHECK(pstring_is(RSEG, 0x0100, "123") && cpu.ax == 0);
    /* Width 10 above M = 8: eight characters, seven spaces; SUB 8,1 = 7 has odd parity. */
    cpu = str(7, 10, 8);
    CHECK(pstring_is(RSEG, 0x0100, "       7") && cpu.ax == 0x0020 && cpu.flags == 0x0202);
    /* A negative width (-5) loses to the digit count. */
    cpu = str(77, 0xFFFB, 255);
    CHECK(pstring_is(RSEG, 0x0100, "77") && cpu.ax == 0);
    /* An altered power table: ARGUMENT before any change. */
    {
        const unsigned words[6] = {255, 0x0100, RSEG, 0, 5, 0};
        pr_system_leaf_cpu before;
        world(words, 6);
        put16(SYS, 0x12CA, 2);
        memset(&cpu, 0, sizeof cpu);
        cpu.sp = SP0; cpu.cs = SYS; cpu.ip = 0x1366; cpu.ss = SS;
        before = cpu;
        CHECK(pr_system_str_1366(ram, RAM, &cpu) == PR_SYSTEM_LEAF_ARGUMENT && memcmp(&cpu, &before, sizeof cpu) == 0);
        CHECK(get16(SS, SP0 - 2u) == 0xA5A5u && ram[at(RSEG, 0x0100)] == 0xA5);
    }

    /* 00BA:1037 with b = 200 (C8h; the word's high byte is ignored): "200" through T = SS:SP0-102h and StrStore. */
    {
        const unsigned words[3] = {0x99C8, 0x0300, RSEG};
        world(words, 3);
        memset(&res, 0, sizeof res);
        registers(&res.ax, &res.bx, &res.cx, &res.dx, &res.si, &res.di);
        res.bp = BP0; res.sp = SP0; res.cs = RES; res.ip = 0x1037; res.ds = DS0; res.es = 0x9999; res.ss = SS; res.flags = 0x0602;
        CHECK(pr_res_00ba_1037_cpu(ram, RAM, &res, &callee, &status) == PR_RES_1037_OK);
        CHECK(callee == PR_RES_1037_STRSTORE && status == 0);
        CHECK(pstring_is(RSEG, 0x0300, "200") && pstring_is(SS, SP0 - 0x102u, "200"));
        CHECK(res.cs == RET_CS && res.ip == RET_IP && res.sp == SP0 + 6u && res.bp == BP0 && res.ds == DS0 && res.es == RSEG);
        CHECK(res.ax == 0x0003 && res.cx == 0 && res.di == 0x0304 && res.si == SP0 - 0xFEu && res.bx == SP0 - 0x110u && res.dx == DS0);
        CHECK(res.flags == 0x0246);
    }
    /* CODE at the Str LCALL: stopped at +1054 with the six words pushed and XOR AX,AX's flags (DF kept). */
    {
        const unsigned words[3] = {0x0007, 0x0300, RSEG};
        world(words, 3);
        ram[at(RES, 0x1055)] = 0x67;
        memset(&res, 0, sizeof res);
        res.bp = BP0; res.sp = SP0; res.cs = RES; res.ip = 0x1037; res.ds = DS0; res.ss = SS; res.flags = 0x0602;
        CHECK(pr_res_00ba_1037_cpu(ram, RAM, &res, &callee, &status) == PR_RES_1037_CODE && callee == PR_RES_1037_NO_CALLEE && status == -1);
        CHECK(res.ip == 0x1054 && res.cs == RES && res.sp == SP0 - 0x10Eu && res.ax == 0x00FF && res.flags == 0x0646);
        CHECK(get16(SS, SP0 - 0x106u) == 7 && get16(SS, SP0 - 0x10Eu) == 0x00FF);
    }
    /* CALLEE: Str rejects an altered power table; stopped at its entry after the LCALL. */
    {
        const unsigned words[3] = {0x0007, 0x0300, RSEG};
        world(words, 3);
        put16(SYS, 0x12CA, 2);
        memset(&res, 0, sizeof res);
        res.bp = BP0; res.sp = SP0; res.cs = RES; res.ip = 0x1037; res.ds = DS0; res.ss = SS; res.flags = 0x0602;
        CHECK(pr_res_00ba_1037_cpu(ram, RAM, &res, &callee, &status) == PR_RES_1037_CALLEE);
        CHECK(callee == PR_RES_1037_STR && status == PR_SYSTEM_LEAF_ARGUMENT && res.cs == SYS && res.ip == 0x1366 && res.sp == SP0 - 0x112u);
    }
    /* Guards. */
    memset(&res, 0, sizeof res);
    res.ip = 0x1038;
    CHECK(pr_res_00ba_1037_cpu(ram, RAM, &res, NULL, NULL) == PR_RES_1037_ARGUMENT && res.ip == 0x1038);
    CHECK(pr_res_00ba_1037_cpu(NULL, RAM, &res, NULL, NULL) == PR_RES_1037_ARGUMENT);
    CHECK(pr_system_str_1366(ram, RAM - 1u, &cpu) == PR_SYSTEM_LEAF_ARGUMENT && pr_system_str_1366(ram, RAM, NULL) == PR_SYSTEM_LEAF_ARGUMENT);

    free(ram);
    printf("str/1037: %u checks passed\n", checks);
    return 0;
}
