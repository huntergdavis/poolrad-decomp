#include "poolrad/recovered/overlay_u25_entry20_1cc3_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define CS 0x5000u
#define SS 0x2800u
#define SP 0x8000u
#define SYSTEM 0x0AF8u
#define RSEG 0x3100u
#define ROFF 0x0010u

static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }

/* Entry frame: return IP 7000 / CS 9001, record pointer RSEG:ROFF, Real factor (three words). */
static pr_u25_1cc3_cpu_state setup(uint8_t *r, const unsigned words[7], unsigned f0, unsigned f1, unsigned f2)
{
    pr_u25_1cc3_cpu_state c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, 0x1CC3, 0x1800, 0x8888, SS, 0x0202};
    unsigned k, site;
    memset(r, 0xA5, PR_U25_1CC3_CPU_RAM_SIZE);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001); wr(r, SS, SP + 4, ROFF); wr(r, SS, SP + 6, RSEG);
    wr(r, SS, SP + 8, f0); wr(r, SS, SP + 10, f1); wr(r, SS, SP + 12, f2);
    for (k = 0; k < 7; ++k) {
        wr(r, RSEG, ROFF + 0x88 + 2 * k, words[k]);
        site = 0x1CD0 + 0x2A * k;
        r[at(CS, site)] = 0x9A; wr(r, CS, site + 1, 0x119A); wr(r, CS, site + 3, SYSTEM);
        r[at(CS, site + 0xE)] = 0x9A; wr(r, CS, site + 0xF, 0x1186); wr(r, CS, site + 0x11, SYSTEM);
        r[at(CS, site + 0x13)] = 0x9A; wr(r, CS, site + 0x14, 0x119E); wr(r, CS, site + 0x16, SYSTEM);
    }
    return c;
}
static void finished(const pr_u25_1cc3_cpu_state *c)
{
    CHECK(c->cs == 0x9001 && c->ip == 0x7000 && c->sp == SP + 4 + 0x0A && c->bp == 0x789A);
}

int main(void)
{
    uint8_t *r = malloc(PR_U25_1CC3_CPU_RAM_SIZE), *saved = malloc(PR_U25_1CC3_CPU_RAM_SIZE);
    pr_u25_1cc3_cpu_state c, before;
    pr_u25_1cc3_cpu_diagnostic d;
    unsigned k;
    static const unsigned words[7] = {3, 10, 0, 1, 255, 1000, 40000};
    CHECK(r && saved);
#define RUN(cpu) pr_u25_entry20_1cc3_cpu(r, PR_U25_1CC3_CPU_RAM_SIZE, &(cpu), &d)

    /* Factor 2.0 (82 00 00 00 00 00) doubles every word; factor 1.0 (81 ...) leaves them; factor 0 zeroes them. */
    c = setup(r, words, 0x0082, 0x0000, 0x0000);
    CHECK(RUN(c) == PR_U25_1CC3_CPU_OK); finished(&c);
    for (k = 0; k < 7; ++k) CHECK(rd(r, RSEG, ROFF + 0x88 + 2 * k) == (unsigned)(words[k] * 2) % 65536u);
    CHECK(d.calls == 21 && d.callee_status == 0 && d.call_site == 0x1CD0 + 0x2A * 6 + 0x13);
    CHECK(rd(r, RSEG, ROFF + 0x86) == 0xA5A5 && rd(r, RSEG, ROFF + 0x96) == 0xA5A5);   /* neighbours untouched */
    c = setup(r, words, 0x0081, 0x0000, 0x0000);
    CHECK(RUN(c) == PR_U25_1CC3_CPU_OK); finished(&c);
    for (k = 0; k < 7; ++k) CHECK(rd(r, RSEG, ROFF + 0x88 + 2 * k) == words[k]);
    c = setup(r, words, 0x0000, 0x0000, 0x0000);
    CHECK(RUN(c) == PR_U25_1CC3_CPU_OK); finished(&c);
    for (k = 0; k < 7; ++k) CHECK(rd(r, RSEG, ROFF + 0x88 + 2 * k) == 0);
    /* Factor 0.5 (80 ...) halves and truncates: 3 -> 1, 255 -> 127, 40000 -> 20000. */
    c = setup(r, words, 0x0080, 0x0000, 0x0000);
    CHECK(RUN(c) == PR_U25_1CC3_CPU_OK);
    CHECK(rd(r, RSEG, ROFF + 0x88) == 1 && rd(r, RSEG, ROFF + 0x90) == 127 && rd(r, RSEG, ROFF + 0x94) == 20000);

    /* The pushes of the original SYSTEM routines stay in RAM below the stack: the caller's saved BP and the first far
     * call frame sit just under the entry SP. */
    c = setup(r, words, 0x0082, 0x0000, 0x0000);
    CHECK(RUN(c) == PR_U25_1CC3_CPU_OK);
    CHECK(rd(r, SS, SP - 2) == 0x789A);

    /* A large factor overflows at the first nonzero word's multiply: RunError 205 at the LCALL SYSTEM 1186 site. */
    c = setup(r, words, 0x00FF, 0x0000, 0x0000);
    CHECK(RUN(c) == PR_U25_1CC3_CPU_CALLEE);
    CHECK(d.call_site == 0x1CDE && d.callee_status == (int)PR_SYSTEM_REAL_RUNERROR && d.calls == 2);
    CHECK(c.ax == 0x00CD && c.cs == SYSTEM && c.ip == 0x00E2);
    CHECK(rd(r, RSEG, ROFF + 0x88) == 3);                                          /* nothing was stored */
    /* A zero first word survives the huge factor (0 * x = 0), and the stop moves to the next nonzero word. */
    {
        static const unsigned zero_first[7] = {0, 7, 0, 0, 0, 0, 0};
        c = setup(r, zero_first, 0x00FF, 0x0000, 0x0000);
        CHECK(RUN(c) == PR_U25_1CC3_CPU_CALLEE);
        CHECK(d.call_site == 0x1CDE + 0x2A && d.calls == 5);
        CHECK(rd(r, RSEG, ROFF + 0x88) == 0);
    }

    /* Malformed LCALL (opcode or offset): stopped at the site, nothing pushed by it. */
    c = setup(r, words, 0x0082, 0, 0); r[at(CS, 0x1CD0)] = 0x90;
    CHECK(RUN(c) == PR_U25_1CC3_CPU_CODE);
    CHECK(c.ip == 0x1CD0 && c.sp == SP - 2 && d.calls == 0 && d.call_site == 0x1CD0);
    c = setup(r, words, 0x0082, 0, 0); wr(r, CS, 0x1CDF, 0x1187);
    CHECK(RUN(c) == PR_U25_1CC3_CPU_CODE);
    CHECK(c.ip == 0x1CDE && d.calls == 1);

    /* ARGUMENT: wrong IP, SP outside [0046,FFF2], bad host objects; nothing changes. */
    for (k = 0; k < 7; ++k) {
        c = setup(r, words, 0x0082, 0, 0);
        if (k == 0) c.ip = 0x1CC4;
        if (k == 1) c.sp = 0x0045;
        if (k == 2) c.sp = 0xFFF3;
        before = c; memcpy(saved, r, PR_U25_1CC3_CPU_RAM_SIZE);
        CHECK(pr_u25_entry20_1cc3_cpu(k == 3 ? NULL : r, k == 4 ? 1 : PR_U25_1CC3_CPU_RAM_SIZE, k == 5 ? NULL : &c, &d) == PR_U25_1CC3_CPU_ARGUMENT);
        if (k != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, PR_U25_1CC3_CPU_RAM_SIZE) == 0);
        if (k == 6) break;
    }
    c = setup(r, words, 0x0082, 0, 0);
    CHECK(pr_u25_entry20_1cc3_cpu(r, PR_U25_1CC3_CPU_RAM_SIZE, &c, NULL) == PR_U25_1CC3_CPU_OK);
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
