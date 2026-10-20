/* Focused synthetic test for the raw-CPU resident 0464:248E bitmap allocator (docs/re/RES_0464_ALLOC_248E_CPU.md).
 * No game bytes: the heap, frame and far-call segment words are synthetic; expectations are hand-derived. */
#include "poolrad/recovered/resident_0464_alloc_248e_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "alloc 248E line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { DS = 0x5000, SS = 0x2000, SYS = 0x6000, TOPSEG = 0x3300, ENDSEG = 0x3FF0 };
static const unsigned segwords[] = {0x24C3, 0x24E4, 0x2534, 0x255E, 0x2568, 0x258D};

/* Heap with nothing free: FreeList = HeapPtr = TOPSEG:0000, HeapEnd = endseg:0000, default HeapError. */
static pr_r0464_alloc_cpu setup(uint8_t *ram, unsigned mode, unsigned bit, unsigned mult, unsigned w0, unsigned w1, unsigned endseg)
{
    pr_r0464_alloc_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x8800, 0x248E, DS, 0x1111, SS, 0x0002};
    unsigned i;
    memset(ram, 0x5A, PR_BUS_RAM_SIZE);
    for (i = 0; i < 6u; ++i) put(ram, 0x8800, segwords[i], SYS);
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001); put(ram, SS, 0x8004, 0x9000); put(ram, SS, 0x8006, DS);
    put(ram, SS, 0x8008, bit); put(ram, SS, 0x800A, mult); put(ram, SS, 0x800C, w1); put(ram, SS, 0x800E, w0);
    ram[p(DS, 0x4A1E)] = (uint8_t)mode;
    put(ram, DS, 0x4400, 0); put(ram, DS, 0x4402, 0x3000); put(ram, DS, 0x4404, 0); put(ram, DS, 0x4406, TOPSEG);
    put(ram, DS, 0x4408, 0); put(ram, DS, 0x440A, endseg); put(ram, DS, 0x440C, 0); put(ram, DS, 0x440E, TOPSEG);
    put(ram, DS, 0x4414, 0x00A9); put(ram, DS, 0x4416, SYS);
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_BUS_RAM_SIZE); pr_ega *ega = malloc(sizeof *ega);
    pr_port_env env = {NULL, NULL, NULL}; pr_bus bus = {ram, ega, &env, NULL, NULL};
    pr_r0464_alloc_cpu c, before; pr_r0464_alloc_cpu_diagnostic d; unsigned k, zero;
    CHECK(ram != NULL && ega != NULL); pr_ega_reset(ega);

    /* Mode 0, no plane buffer: size 2*4*5*3 + 17h = 143 bytes -> 9 paragraphs at the heap top. */
    c = setup(ram, 0, 0, 3, 4, 5, ENDSEG);
    CHECK(pr_r0464_alloc_248e_cpu(&bus, PR_BUS_RAM_SIZE, &c, 10, 64, &d) == PR_R0464_ALLOC_CPU_OK);
    CHECK(get(ram, DS, 0x9000) == 0 && get(ram, DS, 0x9002) == TOPSEG);
    CHECK(get(ram, DS, 0x4404) == 0 && get(ram, DS, 0x4406) == TOPSEG + 9u);
    CHECK(get(ram, TOPSEG, 0) == 4 && get(ram, TOPSEG, 2) == 5 && get(ram, TOPSEG, 0x11) == 40 && ram[p(TOPSEG, 8)] == 3);
    for (k = 0, zero = 1; k < 143u; ++k) {
        if (k < 4u || k == 8u || k == 0x11u || k == 0x12u) continue;
        if (ram[p(TOPSEG, k)] != 0) zero = 0;
    }
    CHECK(zero && ram[p(TOPSEG, 143)] == 0x5A);
    CHECK(d.getmem_calls == 1 && d.getmem_status == 0 && d.fill_calls == 1 && d.fill_status == 0);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8010 && c.bp == 0x7777);

    /* Mode 0 with the plane buffer: a second GetMem of 120 bytes at TOPSEG+9, its pointer at header +13h, cleared. */
    c = setup(ram, 0, 1, 3, 4, 5, ENDSEG);
    CHECK(pr_r0464_alloc_248e_cpu(&bus, PR_BUS_RAM_SIZE, &c, 10, 64, &d) == PR_R0464_ALLOC_CPU_OK);
    CHECK(get(ram, TOPSEG, 0x13) == 0 && get(ram, TOPSEG, 0x15) == TOPSEG + 9u && d.getmem_calls == 2 && d.fill_calls == 2);
    CHECK(ram[p(TOPSEG + 9u, 0)] == 0 && ram[p(TOPSEG + 9u, 119)] == 0 && ram[p(TOPSEG + 9u, 120)] == 0x5A);

    /* Mode 1 with the plane buffer: sizes doubled (2*2*4*5*3 = 240, block 263 bytes: (263+7) = 10Eh -> 16 paragraphs
     * and granule 8, so the heap top becomes TOPSEG+16:0008); the second buffer is a quarter (60 bytes) there.
     * SHR +2555's AF reaches the RETURN through FillChar: set (DOSBox-X rule). */
    c = setup(ram, 1, 1, 3, 4, 5, ENDSEG);
    CHECK(pr_r0464_alloc_248e_cpu(&bus, PR_BUS_RAM_SIZE, &c, 10, 64, &d) == PR_R0464_ALLOC_CPU_OK);
    CHECK(get(ram, TOPSEG, 0x11) == 80 && get(ram, TOPSEG, 0x13) == 8 && get(ram, TOPSEG, 0x15) == TOPSEG + 16u);
    CHECK((c.flags & 0x10u) != 0);
    CHECK(ram[p(TOPSEG + 16u, 8u + 59u)] == 0 && ram[p(TOPSEG + 16u, 8u + 60u)] == 0x5A);

    /* No room (HeapEnd = HeapPtr): GetMem's default HeapError returns 0, so GetMem enters RunError: CALLEE. */
    c = setup(ram, 0, 0, 3, 4, 5, TOPSEG);
    CHECK(pr_r0464_alloc_248e_cpu(&bus, PR_BUS_RAM_SIZE, &c, 10, 64, &d) == PR_R0464_ALLOC_CPU_CALLEE);
    CHECK(d.getmem_calls == 1 && d.getmem_status == 2 && d.fill_calls == 0 && c.ip == 0x00E2 && c.ax == 203);

    /* ARGUMENT: rejected before any change, including a DS window inside the EGA aperture. */
    c = setup(ram, 0, 0, 3, 4, 5, ENDSEG); before = c;
    CHECK(pr_r0464_alloc_248e_cpu(NULL, PR_BUS_RAM_SIZE, &c, 10, 64, &d) == PR_R0464_ALLOC_CPU_ARGUMENT);
    CHECK(pr_r0464_alloc_248e_cpu(&bus, PR_BUS_RAM_SIZE - 1u, &c, 10, 64, &d) == PR_R0464_ALLOC_CPU_ARGUMENT);
    c.ip = 0x248F; CHECK(pr_r0464_alloc_248e_cpu(&bus, PR_BUS_RAM_SIZE, &c, 10, 64, &d) == PR_R0464_ALLOC_CPU_ARGUMENT);
    c.ip = 0x248E; c.ds = 0xA000; CHECK(pr_r0464_alloc_248e_cpu(&bus, PR_BUS_RAM_SIZE, &c, 10, 64, &d) == PR_R0464_ALLOC_CPU_ARGUMENT);
    c.ds = DS; CHECK(memcmp(&c, &before, sizeof c) == 0 && get(ram, DS, 0x9002) == 0x5A5A);

    free(ega); free(ram);
    printf("alloc 248E: %u semantic checks passed\n", checks);
    return 0;
}
