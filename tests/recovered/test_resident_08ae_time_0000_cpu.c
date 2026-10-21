/* Focused synthetic test for the raw-CPU resident 08AE:0000 time in hundredths (docs/re/RES_08AE_TIME_0000_CPU.md).
 * No game bytes: the two far-call segment words and the frame are synthetic; expectations are hand-derived. */
#include "poolrad/recovered/resident_08ae_time_0000_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "time 0000 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, CS = 0x8800 };
static unsigned hh, mm, ss, cc; static int refuse;
static int service(void *ctx, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after)
{
    (void)ctx;
    if (refuse) return 1;
    *after = *before; after->cx = (uint16_t)((hh << 8) | mm); after->dx = (uint16_t)((ss << 8) | cc); after->ip = 0x00A9u;
    return 0;
}
static pr_r08ae_time_cpu setup(uint8_t *ram, unsigned h, unsigned m, unsigned s, unsigned c)
{
    pr_r08ae_time_cpu cpu = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, CS, 0x0000, 0x5000, 0x1111, SS, 0x0002};
    memset(ram, 0x5A, PR_BUS_RAM_SIZE);
    put(ram, CS, 0x001D, 0x6000); put(ram, CS, 0x002D, 0x6800);          /* GetTime and LongMul segments */
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001);          /* caller's far return */
    hh = h; mm = m; ss = s; cc = c; refuse = 0;
    return cpu;
}
static unsigned long run(uint8_t *ram, pr_bus *bus, unsigned h, unsigned m, unsigned s, unsigned c, pr_r08ae_time_cpu *out)
{
    pr_r08ae_time_cpu_diagnostic d;
    *out = setup(ram, h, m, s, c);
    CHECK(pr_r08ae_time_0000_cpu(bus, PR_BUS_RAM_SIZE, out, service, NULL, &d) == PR_R08AE_TIME_CPU_OK);
    CHECK(d.gettime_status == 0 && d.longmul_status == 0);
    CHECK(out->cs == 0x9001 && out->ip == 0x7000 && out->sp == 0x8004 && out->bp == 0x7777);
    CHECK(get(ram, SS, 0x7FFA) == out->ax && get(ram, SS, 0x7FFC) == out->dx);  /* [BP-4]/[BP-2] hold the result */
    return ((unsigned long)out->dx << 16) | out->ax;
}

int main(void)
{
    uint8_t *ram = malloc(PR_BUS_RAM_SIZE);
    pr_bus bus = {ram, NULL, NULL, NULL, NULL};
    pr_r08ae_time_cpu c, before; pr_r08ae_time_cpu_diagnostic d;
    CHECK(ram != NULL);

    /* 13:42:07.55 = 13*360000 + (42*6000 mod 65536 = 55392) + 700 + 55 = 4736147 (0048:4493), not 4932755:
     * the original keeps only the low word of minute*6000 (MUL then XOR DX,DX). */
    CHECK(run(ram, &bus, 13, 42, 7, 55, &c) == 4736147ul && c.dx == 0x0048 && c.ax == 0x4493);
    /* 00:10:59.99: no wrap yet (10*6000 = 60000) -> 65999. */
    CHECK(run(ram, &bus, 0, 10, 59, 99, &c) == 65999ul);
    /* 00:11:00.00: 66000 wraps to 464 in the minute term. */
    CHECK(run(ram, &bus, 0, 11, 0, 0, &c) == 464ul);
    /* 23:59:59.99 -> 8312319 (untruncated would be 8639999). */
    CHECK(run(ram, &bus, 23, 59, 59, 99, &c) == 8312319ul);
    /* GetTime wrote the four locals: hour [BP-6], minute [BP-8], second [BP-0Ah], sec100 [BP-0Ch] (BP = 7FFEh). */
    CHECK(get(ram, SS, 0x7FF8) == 23 && get(ram, SS, 0x7FF6) == 59 && get(ram, SS, 0x7FF4) == 59 && get(ram, SS, 0x7FF2) == 99);

    /* The DOS service refuses: CALLEE with GetTime's SERVICE status, stopped inside GetTime at its INT. */
    c = setup(ram, 13, 42, 7, 55); refuse = 1;
    CHECK(pr_r08ae_time_0000_cpu(&bus, PR_BUS_RAM_SIZE, &c, service, NULL, &d) == PR_R08AE_TIME_CPU_CALLEE);
    CHECK(d.gettime_status == PR_DOS_GETTIME_SERVICE && d.longmul_status == -1 && c.cs == 0x6000 && c.ip == 0x00A7);

    /* ARGUMENT: rejected before any change (NULL service, an attached EGA, wrong IP, wrong RAM size). */
    c = setup(ram, 1, 2, 3, 4); before = c;
    CHECK(pr_r08ae_time_0000_cpu(&bus, PR_BUS_RAM_SIZE, &c, NULL, NULL, &d) == PR_R08AE_TIME_CPU_ARGUMENT);
    bus.ega = (pr_ega *)ram; CHECK(pr_r08ae_time_0000_cpu(&bus, PR_BUS_RAM_SIZE, &c, service, NULL, &d) == PR_R08AE_TIME_CPU_ARGUMENT);
    bus.ega = NULL;
    CHECK(pr_r08ae_time_0000_cpu(&bus, PR_BUS_RAM_SIZE - 1u, &c, service, NULL, &d) == PR_R08AE_TIME_CPU_ARGUMENT);
    c.ip = 1; CHECK(pr_r08ae_time_0000_cpu(&bus, PR_BUS_RAM_SIZE, &c, service, NULL, &d) == PR_R08AE_TIME_CPU_ARGUMENT);
    c.ip = 0; CHECK(memcmp(&c, &before, sizeof c) == 0 && get(ram, SS, 0x7FFE) == 0x5A5A);

    free(ram);
    printf("time 0000: %u semantic checks passed\n", checks);
    return 0;
}
