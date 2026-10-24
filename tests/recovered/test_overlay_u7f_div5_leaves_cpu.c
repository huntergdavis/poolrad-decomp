/* Focused synthetic test for the raw-CPU unit 0x7F div 5 leaves, entries 10 (+0130) and 11 (+0185) (docs/re/OVL_007F_DIV5_LEAVES_CPU.md). The models
 * read no code bytes; the record, the DS block and the frame are this test's. */
#include "poolrad/recovered/overlay_u7f_div5_leaves_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "u7f div5 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_BUS_RAM_SIZE
static uint8_t *m;
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & (RAM - 1u); }
static void put(unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & (RAM - 1u)] = (uint8_t)(v >> 8); }
static unsigned get(unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | ((unsigned)m[(a + 1u) & (RAM - 1u)] << 8); }

enum { CS = 0x3000, DS = 0x4000, SS = 0x6000, SP0 = 0x8000, RSEG = 0x5000, ROFF = 0x0100 };
static pr_u7f_div5_cpu setup(unsigned entry, unsigned value)
{
    pr_u7f_div5_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, SP0, CS, (uint16_t)entry, DS, 0x8888, SS, 0x0202};
    memset(m, 0xA5, RAM);
    put(DS, 0x5D92, ROFF); put(DS, 0x5D94, RSEG);
    put(SS, SP0, 0x7000); put(SS, SP0 + 2u, 0x9001); put(SS, SP0 + 4u, value);
    return c;
}

int main(void)
{
    static pr_ega ega; static uint8_t before[RAM];
    pr_bus bus; pr_u7f_div5_cpu c, saved; size_t left; unsigned v, k;
    m = malloc(RAM); CHECK(m != NULL);
    memset(&bus, 0, sizeof bus); bus.ram = m; bus.ega = &ega; pr_ega_reset(&ega);
    /* entry 10: words +88..+8C := 0, +8E := value mod 5, +90 := value div 5; RETF 2 back with SP+6 and BP restored */
    for (v = 0; v < 70000u; v += 4999u) {
        unsigned value = v & 0xFFFFu;
        c = setup(0x0130, value); left = 1000;
        CHECK(pr_u7f_div5_0130_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_RETURNED);
        CHECK(get(RSEG, ROFF + 0x88) == 0 && get(RSEG, ROFF + 0x8A) == 0 && get(RSEG, ROFF + 0x8C) == 0);
        CHECK(get(RSEG, ROFF + 0x8E) == value % 5u && get(RSEG, ROFF + 0x90) == value / 5u && get(RSEG, ROFF + 0x92) == 0xA5A5u && get(RSEG, ROFF + 0x86) == 0xA5A5u);
        CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP0 + 6u && c.bp == 0x7777 && c.cx == 5 && c.ax == value % 5u && c.dx == value / 5u);
    }
    /* entry 10's flags at return are the second DIV's (pinned DIVW): value 7 -> quotient 1, remainder 2: CF (rem&3 = 2), ZF clear, PF set (02h^01h = 03h, two bits: even) */
    c = setup(0x0130, 7); left = 1000; CHECK(pr_u7f_div5_0130_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_RETURNED);
    CHECK((c.flags & 0x08D5u) == 0x0005u && (c.flags & 0x0200u));
    /* value 5 -> quotient 1, remainder 0: ZF set (rem 0, quotient odd), CF clear, PF clear (01h) */
    c = setup(0x0130, 5); left = 1000; CHECK(pr_u7f_div5_0130_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_RETURNED);
    CHECK((c.flags & 0x08D5u) == 0x0040u);
    /* entry 11: the twenty bytes DS:67F4..6807 as dwords 0,0,0, value mod 5, value div 5; the flags are XOR DX,DX's */
    for (v = 0; v < 70000u; v += 6007u) {
        unsigned value = v & 0xFFFFu;
        c = setup(0x0185, value); left = 1000;
        CHECK(pr_u7f_div5_0185_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_RETURNED);
        for (k = 0; k < 6u; ++k) CHECK(get(DS, 0x67F4 + 2u * k) == 0);
        CHECK(get(DS, 0x6800) == value % 5u && get(DS, 0x6802) == 0 && get(DS, 0x6804) == value / 5u && get(DS, 0x6806) == 0);
        CHECK(get(DS, 0x6808) == 0xA5A5u && get(DS, 0x67F2) == 0xA5A5u && (c.flags & 0x08D5u) == 0x0044u);
        CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP0 + 6u && c.bp == 0x7777 && c.di == 16u);
    }
    /* a record over the return address: the loop's zeros overwrite it and RETF 2 goes elsewhere (TRANSFER) */
    c = setup(0x0130, 9); put(DS, 0x5D92, (SP0 - 0x88u) & 0xFFFFu); put(DS, 0x5D94, SS); left = 1000;
    CHECK(pr_u7f_div5_0130_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_TRANSFER && c.ip == 0 && c.cs == 0);
    /* the record's word +8Ah (pass 1) over the loop counter [BP-1]: pass 1 re-zeroes it every time, so the loop never ends (LIMIT) */
    c = setup(0x0130, 9); put(DS, 0x5D92, (SP0 - 4u - 0x8Au) & 0xFFFFu); put(DS, 0x5D94, SS); left = 2000;
    CHECK(pr_u7f_div5_0130_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_LIMIT && left == 0 && c.ip >= 0x013C && c.ip < 0x0158);
    /* the budget: 9 instructions stop entry 11 at the second SHL (019C) with AF set by the first */
    c = setup(0x0185, 9); left = 9; CHECK(pr_u7f_div5_0185_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_LIMIT && c.ip == 0x019C && (c.flags & 0x0010u));
    /* budget 0 changes nothing */
    c = setup(0x0130, 3); saved = c; memcpy(before, m, RAM); left = 0;
    CHECK(pr_u7f_div5_0130_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_LIMIT && !memcmp(&c, &saved, sizeof c) && !memcmp(m, before, RAM));
    /* argument rejection is atomic */
    c = setup(0x0185, 3); memcpy(before, m, RAM); left = 100;
    c.ip = 0x0130; saved = c; CHECK(pr_u7f_div5_0185_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_ARGUMENT);
    c.ip = 0x0185; c.sp = 0x0003; saved = c; CHECK(pr_u7f_div5_0185_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_ARGUMENT);
    c.sp = 0xFFFB; saved = c; CHECK(pr_u7f_div5_0185_cpu(&bus, RAM, &c, &left) == PR_U7F_DIV5_CPU_ARGUMENT);
    CHECK(!memcmp(&c, &saved, sizeof c) && !memcmp(m, before, RAM) && left == 100); c.sp = SP0;
    CHECK(pr_u7f_div5_0185_cpu(NULL, RAM, &c, &left) == PR_U7F_DIV5_CPU_ARGUMENT);
    CHECK(pr_u7f_div5_0185_cpu(&bus, RAM - 1u, &c, &left) == PR_U7F_DIV5_CPU_ARGUMENT);
    CHECK(pr_u7f_div5_0185_cpu(&bus, RAM, NULL, &left) == PR_U7F_DIV5_CPU_ARGUMENT);
    CHECK(pr_u7f_div5_0130_cpu(&bus, RAM, &c, NULL) == PR_U7F_DIV5_CPU_ARGUMENT);
    free(m); printf("OVL_007F_0130/0185 synthetic, flag, budget and guard cases passed (%u checks)\n", checks); return 0;
}
