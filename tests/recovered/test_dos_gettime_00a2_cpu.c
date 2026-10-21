/* Focused synthetic test for the raw-CPU Dos.GetTime 0A32:00A2 (docs/re/DOS_GETTIME_00A2_CPU.md). No game bytes. */
#include "poolrad/recovered/dos_gettime_00a2_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "gettime line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, DATA = 0x3000 };
static int mode;  /* 0 answer 13:42:07.55; 1 refuse; 2 return to the wrong IP */
static int service(void *ctx, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after)
{
    (void)ctx;
    if (mode == 1) return 1;
    *after = *before;
    after->cx = (13u << 8) | 42u; after->dx = (7u << 8) | 55u;
    after->ip = mode == 2 ? 0x00A8u : 0x00A9u;
    return 0;
}
static pr_dos_gettime_cpu setup(uint8_t *ram, uint16_t sp, uint16_t flags)
{
    pr_dos_gettime_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0, 0x6000, 0x00A2, 0x5000, 0x1111, SS, 0};
    const unsigned w[] = {0x7000, 0x9001, 0x0130, DATA, 0x0120, DATA, 0x0110, DATA, 0x0100, DATA};
    unsigned i;
    memset(ram, 0x5A, PR_DOS_GETTIME_RAM_SIZE);
    for (i = 0; i < 10u; ++i) put(ram, SS, (uint16_t)(sp + 2u * i), w[i]);
    c.sp = sp; c.flags = flags; mode = 0; return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_DOS_GETTIME_RAM_SIZE);
    pr_dos_gettime_cpu c, before;
    CHECK(ram != NULL);

    /* 13:42:07.55: Hour <- CH, Minute <- CL, Second <- DH, Sec100 <- DL, each a word with high byte 0. */
    c = setup(ram, 0x8000, 0x08D7);
    CHECK(pr_dos_gettime_00a2(ram, PR_DOS_GETTIME_RAM_SIZE, &c, service, NULL) == PR_DOS_GETTIME_RETURNED);
    CHECK(get(ram, DATA, 0x100) == 13 && get(ram, DATA, 0x110) == 42 && get(ram, DATA, 0x120) == 7 && get(ram, DATA, 0x130) == 55);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8014 && c.bp == 0x7777);
    CHECK(c.ax == 13 && c.cx == 0x0D2A && c.dx == 0x0737 && c.es == DATA && c.di == 0x0102);
    /* XOR AH,AH: CF OF AF SF clear, ZF PF set; TF IF DF and the reserved bits as they were. */
    CHECK(c.flags == ((0x08D7u & (uint16_t)~0x08D5u) | 0x0044u));
    CHECK(get(ram, SS, 0x7FFE) == 0x7777);  /* pushed BP left below the popped frame */

    /* DF set: each STOSW steps DI down; the stored words are the same. */
    c = setup(ram, 0x8000, 0x0402);
    CHECK(pr_dos_gettime_00a2(ram, PR_DOS_GETTIME_RAM_SIZE, &c, service, NULL) == PR_DOS_GETTIME_RETURNED);
    CHECK(c.di == 0x00FE && get(ram, DATA, 0x100) == 13 && (c.flags & 0x0400u) != 0);

    /* The service refuses, or answers with a non-continuation: stop at the INT, AH = 2Ch, BP pushed, nothing stored. */
    c = setup(ram, 0x8000, 0x0002); mode = 1;
    CHECK(pr_dos_gettime_00a2(ram, PR_DOS_GETTIME_RAM_SIZE, &c, service, NULL) == PR_DOS_GETTIME_SERVICE);
    CHECK(c.ip == 0x00A7 && c.ax == 0x2C34 && c.sp == 0x7FFE && c.bp == 0x7FFE && get(ram, DATA, 0x100) == 0x5A5A);
    c = setup(ram, 0x8000, 0x0002); mode = 2;
    CHECK(pr_dos_gettime_00a2(ram, PR_DOS_GETTIME_RAM_SIZE, &c, service, NULL) == PR_DOS_GETTIME_SERVICE && c.ip == 0x00A7);

    /* ARGUMENT: rejected before any change, including SP outside [0002,FFEC]. */
    c = setup(ram, 0x8000, 0x0002); before = c;
    CHECK(pr_dos_gettime_00a2(NULL, PR_DOS_GETTIME_RAM_SIZE, &c, service, NULL) == PR_DOS_GETTIME_ARGUMENT);
    CHECK(pr_dos_gettime_00a2(ram, PR_DOS_GETTIME_RAM_SIZE - 1u, &c, service, NULL) == PR_DOS_GETTIME_ARGUMENT);
    CHECK(pr_dos_gettime_00a2(ram, PR_DOS_GETTIME_RAM_SIZE, &c, NULL, NULL) == PR_DOS_GETTIME_ARGUMENT);
    c.sp = 0x0001; CHECK(pr_dos_gettime_00a2(ram, PR_DOS_GETTIME_RAM_SIZE, &c, service, NULL) == PR_DOS_GETTIME_ARGUMENT);
    c.sp = 0xFFED; CHECK(pr_dos_gettime_00a2(ram, PR_DOS_GETTIME_RAM_SIZE, &c, service, NULL) == PR_DOS_GETTIME_ARGUMENT);
    c.sp = 0x8000; c.ip = 0x00A3; CHECK(pr_dos_gettime_00a2(ram, PR_DOS_GETTIME_RAM_SIZE, &c, service, NULL) == PR_DOS_GETTIME_ARGUMENT);
    c.ip = 0x00A2; CHECK(memcmp(&c, &before, sizeof c) == 0 && get(ram, DATA, 0x100) == 0x5A5A);

    free(ram);
    printf("gettime 00A2: %u semantic checks passed\n", checks);
    return 0;
}
