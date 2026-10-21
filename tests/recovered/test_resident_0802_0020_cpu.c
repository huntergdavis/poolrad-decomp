/* Focused synthetic test for the raw-CPU resident 0802:0020, the GetMem wrapper (docs/re/RES_0802_0020_CPU.md). No game
 * bytes: the far-call operand is written at its site and the heap is synthetic. */
#include "poolrad/recovered/resident_0802_0020_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "0020 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { CS = 0x1802, SYS = 0x1AF8, DS = 0x1C7C, SS = 0x6000, PV = 0x5800, HEAP = 0x7000 };
static pr_r0802_0020_cpu setup(uint8_t *m, unsigned size, unsigned paras)
{
    pr_r0802_0020_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, CS, 0x0020, DS, 0x1111, SS, 0x0002};
    unsigned k, frame[] = {0x7000, 0x9001, 0, 0x0010, PV};
    memset(m, 0x5A, PR_R0802_0020_RAM_SIZE);
    m[p(CS, 0x002D)] = 0x9A; put(m, CS, 0x002E, 0x023F); put(m, CS, 0x0030, SYS);
    put(m, DS, 0x4400, 0); put(m, DS, 0x4402, HEAP); put(m, DS, 0x4404, 0); put(m, DS, 0x4406, HEAP);   /* HeapOrg, HeapPtr */
    put(m, DS, 0x4408, 0); put(m, DS, 0x440A, HEAP + paras);                                         /* HeapEnd */
    put(m, DS, 0x440C, 0); put(m, DS, 0x440E, HEAP);                                                 /* FreeList = HeapPtr */
    put(m, DS, 0x4414, 0x00A9); put(m, DS, 0x4416, SYS);                                             /* default HeapError */
    put(m, PV, 0x10, 0xEEEE); put(m, PV, 0x12, 0xEEEE);
    frame[2] = size;
    for (k = 0; k < 5; ++k) put(m, SS, 0xF000 + 2u * k, frame[k]);
    return c;
}

int main(void)
{
    uint8_t *m = malloc(PR_R0802_0020_RAM_SIZE);
    pr_r0802_0020_cpu c, before;
    pr_r0802_0020_diagnostic d;
    CHECK(m != NULL);

    /* size 9 -> 16 bytes from the top of the heap: P := 7000:0000, HeapPtr := 7001:0000; RETF 6 */
    c = setup(m, 9, 0x100);
    CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_RETURNED);
    CHECK(get(m, PV, 0x10) == 0 && get(m, PV, 0x12) == HEAP && get(m, DS, 0x4406) == HEAP + 1 && get(m, DS, 0x4404) == 0);
    CHECK(c.ip == 0x7000 && c.cs == 0x9001 && c.sp == 0xF00A && c.bp == 0x7777);
    /* size 8 is already a granule: 8 bytes -> HeapPtr := 7000:0008 */
    c = setup(m, 8, 0x100);
    CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_RETURNED && get(m, DS, 0x4404) == 8);
    /* size 0 -> nil, heap unchanged */
    c = setup(m, 0, 0x100);
    CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_RETURNED);
    CHECK(get(m, PV, 0x10) == 0 && get(m, PV, 0x12) == 0 && get(m, DS, 0x4406) == HEAP && get(m, DS, 0x4404) == 0);
    /* the narrow fact: FFF9h..FFFFh + 7 wraps, AND FFF8h gives 0, so a near-64K request answers nil with no RunError */
    c = setup(m, 0xFFFF, 0x2);
    CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_RETURNED);
    CHECK(get(m, PV, 0x10) == 0 && get(m, PV, 0x12) == 0 && get(m, DS, 0x4406) == HEAP);
    c = setup(m, 0xFFF9, 0x2);
    CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_RETURNED && get(m, PV, 0x12) == 0);
    /* FFF8h does not wrap: it needs FFF8h bytes, more than 2 paragraphs -> RunError 203 at 0AF8:00E2; P untouched */
    c = setup(m, 0xFFF8, 0x2);
    CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_RUNERROR);
    CHECK(c.cs == SYS && c.ip == 0x00E2 && c.ax == 0x00CB && get(m, PV, 0x10) == 0xEEEE);
    /* a non-default HeapError on failure: stopped at its indirect call */
    c = setup(m, 0x100, 0x2); put(m, DS, 0x4414, 0x1234); put(m, DS, 0x4416, 0x5678);
    CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_HEAP_ERROR && c.cs == SYS);
    /* ARGUMENT: rejected before any change */
    c = setup(m, 9, 0x100); before = c;
    CHECK(pr_r0802_0020(NULL, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_ARGUMENT);
    CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE - 1u, &c, 100, &d) == PR_R0802_0020_ARGUMENT);
    c.sp = 0x001F; CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_ARGUMENT);
    c.sp = 0xFFF7; CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x0021; CHECK(pr_r0802_0020(m, PR_R0802_0020_RAM_SIZE, &c, 100, &d) == PR_R0802_0020_ARGUMENT);
    c.ip = 0x0020; CHECK(memcmp(&c, &before, sizeof c) == 0 && get(m, PV, 0x10) == 0xEEEE);

    free(m);
    printf("0802:0020: %u semantic checks passed\n", checks);
    return 0;
}
