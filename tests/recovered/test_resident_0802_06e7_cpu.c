/* Focused synthetic test for the raw-CPU resident 0802:06E7 (docs/re/RES_0802_06E7_CPU.md). No game bytes: the four far-call
 * operands are written at their sites. */
#include "poolrad/recovered/resident_0802_06e7_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "06e7 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { CS = 0x1802, SYS = 0x1AF8, DS = 0x1C7C, SS = 0x7000, F = 0x5800 };
static int calls, refuse, fail_on; static unsigned handles[4];
static int dos(void *ctx, const pr_system_close_cpu *b, pr_system_close_cpu *a)
{
    (void)ctx;
    handles[calls++] = b->bx; *a = *b; a->ip = (uint16_t)(b->ip + 2u);
    if (refuse) return 1;
    if ((int)b->bx == fail_on) { a->ax = 6; a->flags = (uint16_t)(b->flags | 1u); }
    else { a->ax = (uint16_t)((b->ax & 0xFF00u) | 1u); a->flags = (uint16_t)(b->flags & ~1u); }
    return 0;
}
static pr_r0802_06e7_cpu setup(uint8_t *m, unsigned ha, unsigned ma, unsigned hb, unsigned mb)
{
    pr_r0802_06e7_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, CS, 0x06E7, DS, 0x1111, SS, 0x0002};
    static const unsigned sites[][2] = {{0x06EF, 0x1491}, {0x06F4, 0x04A9}, {0x06FE, 0x1491}, {0x0703, 0x04A9}};
    unsigned k;
    memset(m, 0x5A, PR_R0802_06E7_RAM_SIZE);
    for (k = 0; k < 4; ++k) { m[p(CS, sites[k][0])] = 0x9A; put(m, CS, sites[k][0] + 1u, sites[k][1]); put(m, CS, sites[k][0] + 3u, SYS); }
    put(m, DS, 0x4426, 0);
    put(m, F, 0x100, ha); put(m, F, 0x102, ma); put(m, F, 0x104, 128);           /* file A */
    put(m, F, 0x200, hb); put(m, F, 0x202, mb); put(m, F, 0x204, 128);           /* file B */
    put(m, SS, 0xF000, 0x7000); put(m, SS, 0xF002, 0x9001);
    put(m, SS, 0xF004, 0x0100); put(m, SS, 0xF006, F); put(m, SS, 0xF008, 0x0200); put(m, SS, 0xF00A, F);
    calls = refuse = 0; fail_on = -1;
    return c;
}

int main(void)
{
    uint8_t *m = malloc(PR_R0802_06E7_RAM_SIZE);
    pr_r0802_06e7_cpu c, before;
    pr_r0802_06e7_diagnostic d;
    CHECK(m != NULL);

    /* Both open: Close B (handle 6) then A (handle 7), each IOCheck passes; both magics D7B0h; RETF 8. */
    c = setup(m, 7, 0xD7B3, 6, 0xD7B3);
    CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_RETURNED && calls == 2);
    CHECK(handles[0] == 6 && handles[1] == 7 && get(m, F, 0x202) == 0xD7B0 && get(m, F, 0x102) == 0xD7B0);
    CHECK(c.ip == 0x7000 && c.cs == 0x9001 && c.sp == 0xF00C && c.bp == 0x7777 && get(m, DS, 0x4426) == 0);
    /* A standard handle (2) is never closed through DOS, but its magic still becomes D7B0h. */
    c = setup(m, 2, 0xD7B3, 6, 0xD7B3);
    CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_RETURNED && calls == 1 && handles[0] == 6);
    CHECK(get(m, F, 0x102) == 0xD7B0);
    /* B not open: Close sets InOutRes 103, IOCheck stops at RunError with AX = 103; A untouched, no DOS call. */
    c = setup(m, 7, 0xD7B3, 6, 0xD7B0);
    CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_RUNERROR && calls == 0);
    CHECK(c.cs == SYS && c.ip == 0x00E2 && c.ax == 103 && get(m, F, 0x102) == 0xD7B3 && d.site == 0x06F4);
    /* DOS error closing B (handle 6): InOutRes 6, B's magic still D7B0h, RunError with AX = 6 before A is touched. */
    c = setup(m, 7, 0xD7B3, 6, 0xD7B3); fail_on = 6;
    CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_RUNERROR && calls == 1);
    CHECK(c.ax == 6 && get(m, F, 0x202) == 0xD7B0 && get(m, F, 0x102) == 0xD7B3);
    /* A DOS error closing A: the second IOCheck stops. */
    c = setup(m, 7, 0xD7B3, 6, 0xD7B3); fail_on = 7;
    CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_RUNERROR && calls == 2 && d.site == 0x0703);
    /* The same FileRec twice: the second close finds it closed (103). */
    c = setup(m, 7, 0xD7B3, 6, 0xD7B3); put(m, SS, 0xF004, 0x0200);
    CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_RUNERROR && calls == 1 && c.ax == 103);
    /* The service declines: SERVICE, stopped at Close's INT. */
    c = setup(m, 7, 0xD7B3, 6, 0xD7B3); refuse = 1;
    CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_SERVICE && c.ip == 0x14A6 && d.site == 0x06EF);

    /* ARGUMENT: rejected before any change. */
    c = setup(m, 7, 0xD7B3, 6, 0xD7B3); before = c;
    CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, NULL, NULL, &d) == PR_R0802_06E7_ARGUMENT);
    CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE - 1u, &c, dos, NULL, &d) == PR_R0802_06E7_ARGUMENT);
    c.sp = 0x000B; CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_ARGUMENT);
    c.sp = 0xFFF5; CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x06E8; CHECK(pr_r0802_06e7(m, PR_R0802_06E7_RAM_SIZE, &c, dos, NULL, &d) == PR_R0802_06E7_ARGUMENT);
    c.ip = 0x06E7; CHECK(memcmp(&c, &before, sizeof c) == 0 && calls == 0);

    free(m);
    printf("0802:06E7: %u semantic checks passed\n", checks);
    return 0;
}
