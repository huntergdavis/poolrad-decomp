/* Focused synthetic test for the raw-CPU resident 0802:0000, the FreeMem wrapper (docs/re/RES_0802_0000_CPU.md). No game
 * bytes: the far-call operand is written at its site and the heap is synthetic. */
#include "poolrad/recovered/resident_0802_0000_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "0000 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { CS = 0x1802, SYS = 0x1AF8, DS = 0x1C7C, SS = 0x6000, PV = 0x5800, HEAP = 0x7000 };
static pr_r0802_0000_cpu setup(uint8_t *m, unsigned size, unsigned ptr_seg, unsigned ptr_off, unsigned used)
{
    pr_r0802_0000_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, CS, 0x0000, DS, 0x1111, SS, 0x0002};
    unsigned k, frame[] = {0x7000, 0x9001, 0, 0x0010, PV};
    memset(m, 0x5A, PR_R0802_0000_RAM_SIZE);
    m[p(CS, 0x0017)] = 0x9A; put(m, CS, 0x0018, 0x0254); put(m, CS, 0x001A, SYS);
    put(m, DS, 0x4400, 0); put(m, DS, 0x4402, HEAP);                                                 /* HeapOrg */
    put(m, DS, 0x4404, 0); put(m, DS, 0x4406, HEAP + used);                                          /* HeapPtr */
    put(m, DS, 0x4408, 0); put(m, DS, 0x440A, HEAP + 0x2000);                                        /* HeapEnd */
    put(m, DS, 0x440C, 0); put(m, DS, 0x440E, HEAP + used);                                          /* FreeList = HeapPtr */
    put(m, DS, 0x4414, 0x00A9); put(m, DS, 0x4416, SYS);
    put(m, PV, 0x10, ptr_off); put(m, PV, 0x12, ptr_seg);
    frame[2] = size;
    for (k = 0; k < 5; ++k) put(m, SS, 0xF000 + 2u * k, frame[k]);
    return c;
}

int main(void)
{
    uint8_t *m = malloc(PR_R0802_0000_RAM_SIZE);
    pr_r0802_0000_cpu c, before;
    pr_r0802_0000_diagnostic d;
    CHECK(m != NULL);

    /* the top block (256 bytes at the heap's start, HeapPtr just above it): released, HeapPtr lowered; RETF 6 */
    c = setup(m, 0x100, HEAP, 0, 0x10);
    CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_RETURNED);
    CHECK(get(m, DS, 0x4406) == HEAP && get(m, DS, 0x4404) == 0);
    CHECK(c.ip == 0x7000 && c.cs == 0x9001 && c.sp == 0xF00A && c.bp == 0x7777);
    CHECK(get(m, PV, 0x10) == 0 && get(m, PV, 0x12) == HEAP);           /* the variable is not cleared */
    /* size 9 rounds to 16: a 16-byte top block at HEAP:0 with HeapPtr HEAP+1 */
    c = setup(m, 9, HEAP, 0, 1);
    CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_RETURNED && get(m, DS, 0x4406) == HEAP);
    /* size 0: FreeMem's no-op, no pointer check even for a bogus pointer */
    c = setup(m, 0, 0x1234, 3, 0x10);
    CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_RETURNED && get(m, DS, 0x4406) == HEAP + 0x10);
    /* the narrow fact: FFF9h..FFFFh wrap to 0, so the release is SILENTLY skipped (a leak), even for a bogus pointer */
    c = setup(m, 0xFFF9, 0x1234, 3, 0x10);
    CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_RETURNED && get(m, DS, 0x4406) == HEAP + 0x10);
    c = setup(m, 0xFFFF, HEAP, 0, 0x10);
    CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_RETURNED && get(m, DS, 0x4406) == HEAP + 0x10);
    /* refusals: RunError 204 at 0AF8:00E2 */
    c = setup(m, 0x10, HEAP, 4, 0x10);
    CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_RUNERROR && c.cs == SYS && c.ip == 0x00E2 && c.ax == 204);
    c = setup(m, 0x10, HEAP - 1, 0, 0x10);
    CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_RUNERROR && c.ax == 204);
    c = setup(m, 0x10, HEAP + 0x10, 0, 0x10);                            /* at HeapPtr: outside [HeapOrg, HeapPtr) */
    CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_RUNERROR);
    /* ARGUMENT: rejected before any change */
    c = setup(m, 0x100, HEAP, 0, 0x10); before = c;
    CHECK(pr_r0802_0000(NULL, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_ARGUMENT);
    CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE - 1u, &c, 100, &d) == PR_R0802_0000_ARGUMENT);
    c.sp = 0x002B; CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_ARGUMENT);
    c.sp = 0xFFF7; CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x0001; CHECK(pr_r0802_0000(m, PR_R0802_0000_RAM_SIZE, &c, 100, &d) == PR_R0802_0000_ARGUMENT);
    c.ip = 0x0000; CHECK(memcmp(&c, &before, sizeof c) == 0 && get(m, DS, 0x4406) == HEAP + 0x10);

    free(m);
    printf("0802:0000: %u semantic checks passed\n", checks);
    return 0;
}
