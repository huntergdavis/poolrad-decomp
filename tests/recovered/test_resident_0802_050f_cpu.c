/* Focused synthetic test for the raw-CPU resident 0802:050F (docs/re/RES_0802_050F_CPU.md). No game bytes: the two far-call
 * operands are written at their sites and every stream is synthetic. */
#include "poolrad/recovered/resident_0802_050f_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "050f line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { CS = 0x1802, SYS = 0x1AF8, SS = 0x6000, PK = 0x5000, VS = 0x6800, OUT = 0x7000 };
static void lcall(uint8_t *m, unsigned site, unsigned off) { m[p(CS, site)] = 0x9A; put(m, CS, site + 1u, off); put(m, CS, site + 3u, SYS); }
static pr_r0802_050f_cpu setup(uint8_t *m, const uint8_t *packed, unsigned n, unsigned size)
{
    pr_r0802_050f_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, CS, 0x050F, 0x2222, 0x1111, SS, 0x0002};
    unsigned k, frame[] = {0x7000, 0x9001, 10, VS, 8, VS, 4, VS, 0, VS};
    memset(m, 0x5A, PR_R0802_050F_RAM_SIZE);
    lcall(m, 0x055C, 0x1685); lcall(m, 0x05A7, 0x16A8);
    memcpy(m + p(PK, 0), packed, n);
    put(m, VS, 0, 0); put(m, VS, 2, PK); put(m, VS, 4, 0); put(m, VS, 6, OUT); put(m, VS, 8, size); put(m, VS, 10, 0x1234);
    for (k = 0; k < 10; ++k) put(m, SS, 0xF000 + 2u * k, frame[k]);
    return c;
}

int main(void)
{
    uint8_t *m = malloc(PR_R0802_050F_RAM_SIZE);
    pr_r0802_050f_cpu c, before;
    pr_r0802_050f_diagnostic d;
    CHECK(m != NULL);

    {   /* literal 3 bytes, repeat 4 x 'z', literal 1 byte: 8 output bytes; RETF 10h; (in, out) = (8, 8) at [BP-2]/[BP-4] */
        static const uint8_t s[] = {0x02, 'a', 'b', 'c', 0xFC, 'z', 0x00, '!'};
        c = setup(m, s, sizeof s, sizeof s);
        CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 100, &d) == PR_R0802_050F_RETURNED);
        CHECK(memcmp(m + p(OUT, 0), "abczzzz!", 8) == 0 && m[p(OUT, 8)] == 0x5A);
        CHECK(c.ip == 0x7000 && c.cs == 0x9001 && c.sp == 0xF014 && c.bp == 0x7777 && d.arrivals == 3);
        CHECK(get(m, SS, 0xEFFC) == 8 && get(m, SS, 0xEFFA) == 8);
        CHECK(get(m, VS, 10) == 0x1234);                              /* the output-size word is never read or written */
    }
    {   /* 0x7E literal (127 bytes) and 0x81 repeat (127 bytes): the edges of both ranges */
        static uint8_t s[1 + 127 + 2];
        unsigned k;
        s[0] = 0x7E; for (k = 0; k < 127; ++k) s[1 + k] = (uint8_t)k; s[128] = 0x81; s[129] = 0xEE;
        c = setup(m, s, sizeof s, sizeof s);
        CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 100, &d) == PR_R0802_050F_RETURNED);
        CHECK(m[p(OUT, 126)] == 126 && m[p(OUT, 127)] == 0xEE && m[p(OUT, 253)] == 0xEE && m[p(OUT, 254)] == 0x5A);
    }
    {   /* size 0: one control still runs before the size is checked */
        static const uint8_t s[] = {0x01, 'x', 'y'};
        c = setup(m, s, sizeof s, 0);
        CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 100, &d) == PR_R0802_050F_RETURNED && memcmp(m + p(OUT, 0), "xy", 2) == 0);
    }
    {   /* 7Fh with size 0 returns; 7Fh and 80h below the size stall at 051Fh with nothing written */
        static const uint8_t s7f[] = {0x7F, 0x00}, s80[] = {0x01, 'q', 'r', 0x80};
        c = setup(m, s7f, sizeof s7f, 0);
        CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 100, &d) == PR_R0802_050F_RETURNED && m[p(OUT, 0)] == 0x5A);
        c = setup(m, s7f, sizeof s7f, sizeof s7f);
        CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 100, &d) == PR_R0802_050F_STALLED && c.ip == 0x051F && m[p(OUT, 0)] == 0x5A);
        CHECK(d.arrivals == 3 && c.ax == 0 && c.es == VS && c.di == 8);  /* entry arrival, the changing [BP-5], then the fixed point */
        c = setup(m, s80, sizeof s80, sizeof s80);
        CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 100, &d) == PR_R0802_050F_STALLED && memcmp(m + p(OUT, 0), "qr", 2) == 0);
        CHECK(c.ax == 3 && get(m, SS, 0xEFFC) == 3 && get(m, SS, 0xEFFA) == 2);
    }
    {   /* the budget: a size of FFFFh lets in wrap below it; the model stops at arrival max + 1 */
        static const uint8_t s[] = {0x81, 0x66};
        c = setup(m, s, sizeof s, 0xFFFF);
        CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 5, &d) == PR_R0802_050F_BUDGET && d.arrivals == 6 && c.ip == 0x051F);
    }
    {   /* the output index wraps at 16 bits: 517 repeats of 127 then 'Z' land at offset 517*127 - 65536 = 123 */
        static uint8_t s[517 * 2 + 2];
        unsigned k;
        for (k = 0; k < 517; ++k) { s[2 * k] = 0x81; s[2 * k + 1] = 0x11; }
        s[1034] = 0x00; s[1035] = 'Z';
        c = setup(m, s, sizeof s, sizeof s);
        CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 1000, &d) == PR_R0802_050F_RETURNED);
        CHECK(m[p(OUT, 123)] == 'Z' && m[p(OUT, 122)] == 0x11 && m[p(OUT, 124)] == 0x11 && get(m, SS, 0xEFFA) == 124);
    }
    /* ARGUMENT: rejected before any change */
    {   static const uint8_t s[] = {0x00, 'a'};
        c = setup(m, s, sizeof s, sizeof s); before = c;
        CHECK(pr_r0802_050f(NULL, PR_R0802_050F_RAM_SIZE, &c, 10, &d) == PR_R0802_050F_ARGUMENT);
        CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE - 1u, &c, 10, &d) == PR_R0802_050F_ARGUMENT);
        c.sp = 0x0015; CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 10, &d) == PR_R0802_050F_ARGUMENT);
        c.sp = 0xFFED; CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 10, &d) == PR_R0802_050F_ARGUMENT);
        c.sp = 0xF000; c.ip = 0x051F; CHECK(pr_r0802_050f(m, PR_R0802_050F_RAM_SIZE, &c, 10, &d) == PR_R0802_050F_ARGUMENT);
        c.ip = 0x050F; CHECK(memcmp(&c, &before, sizeof c) == 0 && m[p(OUT, 0)] == 0x5A);
    }
    free(m);
    printf("0802:050F: %u semantic checks passed\n", checks);
    return 0;
}
