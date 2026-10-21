/* Focused synthetic test for the raw-CPU resident 0802:02EB FileExists (docs/re/RES_0802_02EB_CPU.md). No game bytes: the
 * five far-call operands are written at their sites. */
#include "poolrad/recovered/resident_0802_02eb_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "02eb line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { CS = 0x1802, SYS = 0x1AF8, DS = 0x1C7C, SS = 0x7000, N = 0x5800 };
static int present, opens, closes, refuse_open; static unsigned open_al;
static int dos_open(void *ctx, const pr_reset_cpu *b, pr_reset_cpu *a)
{
    (void)ctx;
    ++opens; open_al = b->ax & 0xFFu; *a = *b; a->ip = (uint16_t)(b->ip + 2u);
    if (refuse_open) return 1;
    if (present) { a->ax = 6; a->flags = (uint16_t)(b->flags & ~1u); }
    else { a->ax = 2; a->flags = (uint16_t)(b->flags | 1u); }
    return 0;
}
static int dos_close(void *ctx, const pr_system_close_cpu *b, pr_system_close_cpu *a)
{
    (void)ctx;
    ++closes; *a = *b; a->ip = (uint16_t)(b->ip + 2u); a->ax = (uint16_t)((b->ax & 0xFF00u) | 1u); a->flags = (uint16_t)(b->flags & ~1u);
    return 0;
}
static pr_r0802_02eb_cpu setup(uint8_t *m, const char *name)
{
    pr_r0802_02eb_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, CS, 0x02EB, DS, 0x1111, SS, 0x0002};
    static const unsigned sites[][2] = {{0x0300, 0x0B25}, {0x0310, 0x13E2}, {0x031F, 0x1410}, {0x032A, 0x1491}, {0x032F, 0x04A2}};
    unsigned k; size_t n = strlen(name);
    memset(m, 0x5A, PR_R0802_02EB_RAM_SIZE);
    for (k = 0; k < 5; ++k) { m[p(CS, sites[k][0])] = 0x9A; put(m, CS, sites[k][0] + 1u, sites[k][1]); put(m, CS, sites[k][0] + 3u, SYS); }
    put(m, DS, 0x4426, 0); m[p(DS, 0x442C)] = 2; put(m, DS, 0x49CE, 0xEEEE);
    m[p(N, 0x10)] = (uint8_t)n; memcpy(m + p(N, 0x11), name, n);
    put(m, SS, 0xF000, 0x7000); put(m, SS, 0xF002, 0x9001); put(m, SS, 0xF004, 0x0010); put(m, SS, 0xF006, N);
    present = 1; opens = closes = refuse_open = 0;
    return c;
}

int main(void)
{
    uint8_t *m = malloc(PR_R0802_02EB_RAM_SIZE);
    pr_r0802_02eb_env env = {dos_open, NULL, dos_close, NULL};
    pr_r0802_02eb_cpu c, before;
    pr_r0802_02eb_diagnostic d;
    CHECK(m != NULL);

    /* Present: Assign, Reset (3Dh with AL = FileMode 2), Close (3Eh, handle 6), IOResult 0 -> AL = 1; [49CE] = 0; RETF 4. */
    c = setup(m, "ECL1.DAX");
    CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, &env, &d) == PR_R0802_02EB_RETURNED && opens == 1 && closes == 1);
    CHECK((c.ax & 0xFF) == 1 && get(m, DS, 0x49CE) == 0 && open_al == 2);
    CHECK(c.ip == 0x7000 && c.cs == 0x9001 && c.sp == 0xF008 && c.bp == 0x7777);
    CHECK(m[p(SS, 0xEFFE - 0x2A)] == 8 && memcmp(m + p(SS, 0xEFFE - 0x29), "ECL1.DAX", 8) == 0);
    /* Absent: the open fails (2), Close finds the record not open and sets InOutRes 103, which IOResult returns: AL = 0. */
    c = setup(m, "NONE.DAX"); present = 0;
    CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, &env, &d) == PR_R0802_02EB_RETURNED && opens == 1 && closes == 0);
    CHECK((c.ax & 0xFF) == 0 && get(m, DS, 0x49CE) == 103 && get(m, DS, 0x4426) == 0);
    /* Empty name: Reset opens no file (handle 0), Close of handle 0 issues no INT; IOResult 0 but the name is empty: AL = 0. */
    c = setup(m, "");
    CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, &env, &d) == PR_R0802_02EB_RETURNED && opens == 0 && closes == 0);
    CHECK((c.ax & 0xFF) == 0 && get(m, DS, 0x49CE) == 0);
    /* A name longer than 40 is truncated to 40 by StrStore. */
    c = setup(m, "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJXYZ");
    CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, &env, &d) == PR_R0802_02EB_RETURNED && m[p(SS, 0xEFFE - 0x2A)] == 40);
    /* A pending InOutRes (5): Reset still opens and Close still closes, neither clears it, so IOResult returns the stale 5
     * and FileExists answers 0 for a file that exists. */
    c = setup(m, "ECL1.DAX"); put(m, DS, 0x4426, 5);
    CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, &env, &d) == PR_R0802_02EB_RETURNED && (c.ax & 0xFF) == 0);
    CHECK(opens == 1 && closes == 1 && get(m, DS, 0x49CE) == 5 && get(m, DS, 0x4426) == 0);
    /* The open service declines: SERVICE at Reset's INT (1456h). */
    c = setup(m, "ECL1.DAX"); refuse_open = 1;
    CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, &env, &d) == PR_R0802_02EB_SERVICE && c.ip == 0x1456 && d.site == 0x031F);

    /* ARGUMENT: rejected before any change. */
    c = setup(m, "A"); before = c;
    CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, NULL, &d) == PR_R0802_02EB_ARGUMENT);
    CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE - 1u, &c, &env, &d) == PR_R0802_02EB_ARGUMENT);
    c.sp = 0x00FF; CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, &env, &d) == PR_R0802_02EB_ARGUMENT);
    c.sp = 0xFFF9; CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, &env, &d) == PR_R0802_02EB_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x02EC; CHECK(pr_r0802_02eb(m, PR_R0802_02EB_RAM_SIZE, &c, &env, &d) == PR_R0802_02EB_ARGUMENT);
    c.ip = 0x02EB; CHECK(memcmp(&c, &before, sizeof c) == 0 && opens == 0);

    free(m);
    printf("0802:02EB: %u semantic checks passed\n", checks);
    return 0;
}
