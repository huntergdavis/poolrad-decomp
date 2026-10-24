/* Focused synthetic test for the raw-CPU unit 0x25 +1815 string compare (docs/re/OVL_0025_1815_CPU.md), composed with the raw SYSTEM StrStore 0B25,
 * FillChar 16A8 and string compare 0C10. No game bytes: the nine call sites, the strings and the state are this test's (the models read no other code). */
#include "poolrad/recovered/overlay_u25_compare_strings_1815_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "1815 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_U25_1815_RAM_SIZE
static uint8_t *m;
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & (RAM - 1u); }
static void put(unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & (RAM - 1u)] = (uint8_t)(v >> 8); }

enum { CS = 0x6000, SYS = 0x1AF8, DS = 0x1C7C, SS = 0x8000, SP0 = 0xF000, STR = 0x5000, A = 0x0100, B = 0x0400 };
static const unsigned sites[9][2] = {{0x182B, 0x0B25}, {0x183F, 0x0B25}, {0x1850, 0x16A8}, {0x1861, 0x0C10}, {0x1879, 0x0C10}, {0x1891, 0x0C10}, {0x18A9, 0x0C10}, {0x18C1, 0x0C10}, {0x18D9, 0x0C10}};
static void pstr(unsigned off, const char *s) { size_t n = strlen(s); m[p(STR, off)] = (uint8_t)n; memcpy(m + p(STR, off + 1u), s, n); }
static pr_u25_1815_cpu setup(const char *a, const char *b)
{
    pr_u25_1815_cpu c = {0x1234, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, SP0, CS, 0x1815, DS, 0x8888, SS, 0x0202};
    unsigned k;
    memset(m, 0x5A, RAM);
    for (k = 0; k < 9u; ++k) { m[p(CS, sites[k][0])] = 0x9A; put(CS, sites[k][0] + 1u, sites[k][1]); put(CS, sites[k][0] + 3u, SYS); }
    pstr(A, a); pstr(B, b);
    put(SS, SP0, 0x7000); put(SS, SP0 + 2u, 0x9001); put(SS, SP0 + 4u, B); put(SS, SP0 + 6u, STR); put(SS, SP0 + 8u, A); put(SS, SP0 + 10u, STR);
    return c;
}
static int flags6(const char *a, const char *b, const char *want)
{
    pr_u25_1815_cpu c = setup(a, b); size_t left = 1000; unsigned k;
    if (pr_u25_compare_strings_1815_cpu(m, RAM, &c, &left) != PR_U25_1815_RETURNED) return 0;
    if (c.cs != 0x9001 || c.ip != 0x7000 || c.sp != SP0 + 12u || c.bp != 0x7777) return 0;
    for (k = 0; k < 6u; ++k) if (m[p(DS, 0x6F78 + k)] != (uint8_t)(want[k] - '0')) return 0;
    return 1;
}

int main(void)
{
    static uint8_t before[RAM];
    pr_u25_1815_cpu c, saved; size_t left;
    m = malloc(RAM); CHECK(m != NULL);
    /* the six bytes: =, <>, <, >, <=, >= on Pascal order (unsigned bytes, then length) */
    CHECK(flags6("ABC", "ABC", "100011"));
    CHECK(flags6("ABC", "ABD", "011010"));
    CHECK(flags6("ABD", "ABC", "010101"));
    CHECK(flags6("AB", "ABC", "011010"));
    CHECK(flags6("ABC", "AB", "010101"));
    CHECK(flags6("", "", "100011"));
    CHECK(flags6("", "A", "011010"));
    CHECK(flags6("\x80", "\x7f", "010101"));
    CHECK(flags6("a", "A", "010101"));
    /* the copies come before the clear: a string over the flag bytes is compared as on entry */
    c = setup("XY", "XY"); m[p(DS, 0x6F78)] = 2; m[p(DS, 0x6F79)] = 'X'; m[p(DS, 0x6F7A)] = 'Y'; put(SS, SP0 + 8u, 0x6F78); put(SS, SP0 + 10u, DS); left = 1000;
    CHECK(pr_u25_compare_strings_1815_cpu(m, RAM, &c, &left) == PR_U25_1815_RETURNED && m[p(DS, 0x6F78)] == 1 && m[p(DS, 0x6F79)] == 0 && m[p(DS, 0x6F7C)] == 1);
    /* a changed call site: CODE at the call, before its frame */
    c = setup("A", "B"); put(CS, 0x1850 + 1u, 0x16A9); left = 1000;
    CHECK(pr_u25_compare_strings_1815_cpu(m, RAM, &c, &left) == PR_U25_1815_CODE && c.ip == 0x1850 && c.sp == SP0 - 2u - 0x200u - 8u);
    /* the budget: LIMIT at the next unexecuted instruction; budget 0 changes nothing */
    c = setup("A", "B"); left = 3; CHECK(pr_u25_compare_strings_1815_cpu(m, RAM, &c, &left) == PR_U25_1815_LIMIT && c.ip == 0x181C && left == 0);
    c = setup("A", "B"); saved = c; memcpy(before, m, RAM); left = 0;
    CHECK(pr_u25_compare_strings_1815_cpu(m, RAM, &c, &left) == PR_U25_1815_LIMIT && !memcmp(&c, &saved, sizeof c) && !memcmp(m, before, RAM));
    /* argument rejection is atomic */
    c = setup("A", "B"); memcpy(before, m, RAM); left = 100;
    c.ip = 0x1816; saved = c; CHECK(pr_u25_compare_strings_1815_cpu(m, RAM, &c, &left) == PR_U25_1815_ARGUMENT);
    c.ip = 0x1815; c.sp = 0x021F; saved = c; CHECK(pr_u25_compare_strings_1815_cpu(m, RAM, &c, &left) == PR_U25_1815_ARGUMENT);
    c.sp = 0xFFF5; saved = c; CHECK(pr_u25_compare_strings_1815_cpu(m, RAM, &c, &left) == PR_U25_1815_ARGUMENT);
    CHECK(!memcmp(&c, &saved, sizeof c) && !memcmp(m, before, RAM) && left == 100); c.sp = SP0;
    CHECK(pr_u25_compare_strings_1815_cpu(NULL, RAM, &c, &left) == PR_U25_1815_ARGUMENT);
    CHECK(pr_u25_compare_strings_1815_cpu(m, RAM - 1u, &c, &left) == PR_U25_1815_ARGUMENT);
    CHECK(pr_u25_compare_strings_1815_cpu(m, RAM, NULL, &left) == PR_U25_1815_ARGUMENT);
    CHECK(pr_u25_compare_strings_1815_cpu(m, RAM, &c, NULL) == PR_U25_1815_ARGUMENT);
    free(m); printf("OVL_0025_1815 synthetic, order, alias, code, budget and guard cases passed (%u checks)\n", checks); return 0;
}
