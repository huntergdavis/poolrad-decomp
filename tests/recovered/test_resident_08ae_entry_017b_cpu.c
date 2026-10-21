#include "poolrad/recovered/resident_08ae_entry_017b_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define CS 0x5800u
#define DS 0x5000u
#define SS 0x2800u
#define SP 0x8000u

static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }
static unsigned rb(const uint8_t *r, unsigned s, unsigned o) { return r[at(s, o)]; }

/* A far-called entry: SS:[SP] return IP 7000, [SP+2] return CS 9001. DS:5372 = flag byte. */
static pr_r08ae_017b_cpu setup(uint8_t *r, unsigned ip, unsigned flag)
{
    pr_r08ae_017b_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, ip, DS, 0x8888, SS, 0x0202};
    memset(r, 0xA5, PR_R08AE_017B_RAM_SIZE);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001);
    r[at(DS, 0x5372)] = (uint8_t)flag;
    return c;
}

int main(void)
{
    uint8_t *r = malloc(PR_R08AE_017B_RAM_SIZE), *saved = malloc(PR_R08AE_017B_RAM_SIZE);
    pr_r08ae_017b_cpu c, before;
    unsigned k;
    CHECK(r && saved);

    /* [5372] == 2: AL and [5373] are loaded and stored, then the body returns without a call (BP restored, CS:IP popped). */
    c = setup(r, 0x017B, 2);
    CHECK(pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c) == PR_R08AE_017B_RETURN);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP + 4 && c.bp == 0x789A);
    CHECK((c.ax & 0xFF) == 2 && (c.ax & 0xFF00) == 0x1100 && rb(r, DS, 0x5373) == 2);
    CHECK((c.flags & 0x08D5) == 0x0044 && (c.flags & ~0x08D5) == 0x0202);       /* CMP 2,2: ZF and PF only */
    CHECK(c.bx == 0x2222 && c.cx == 0x3333 && c.dx == 0x4444 && c.si == 0x5555 && c.di == 0x6666 && c.ds == DS && c.es == 0x8888 && c.ss == SS);

    /* [5372] != 2: the function stops at CALL 137C with the far-return frame and the saved BP on the stack. */
    for (k = 0; k < 4; ++k) {
        static const unsigned values[4] = {0, 1, 3, 0xFF};
        c = setup(r, 0x017B, values[k]);
        CHECK(pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c) == PR_R08AE_017B_CALL_137C);
        CHECK(c.ip == 0x137C && c.cs == CS && c.sp == SP - 6 && c.bp == SP - 2);
        CHECK(rd(r, SS, SP - 6) == 0x0191 && rd(r, SS, SP - 4) == CS && rd(r, SS, SP - 2) == 0x789A);
        CHECK((c.ax & 0xFF) == values[k] && rb(r, DS, 0x5373) == values[k]);
    }
    c = setup(r, 0x017B, 0);                                                    /* CMP 0,2 = FE: CF, AF, SF */
    pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c);
    CHECK((c.flags & 0x08D5) == 0x0091);
    c = setup(r, 0x017B, 0x82);                                                 /* CMP 82,2 = 80: SF only */
    pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c);
    CHECK((c.flags & 0x08D5) == 0x0080);
    c = setup(r, 0x017B, 0x80);                                                 /* CMP 80,2 = 7E: OF, AF, PF */
    pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c);
    CHECK((c.flags & 0x08D5) == 0x0814);

    /* The post-return entries: each pushes its argument and frame and stops at the next callee. */
    c = setup(r, 0x0191, 5); c.sp = SP - 2; c.bp = 0x1234;
    CHECK(pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c) == PR_R08AE_017B_CALL_1742);
    CHECK(c.ip == 0x1742 && c.sp == SP - 8 && c.ax == 0 && (c.flags & 0x08D5) == 0x0044);
    CHECK(rd(r, SS, SP - 8) == 0x0198 && rd(r, SS, SP - 6) == CS && rd(r, SS, SP - 4) == 0);
    c = setup(r, 0x0198, 5); c.sp = SP - 2; c.ax = 0x7777;
    CHECK(pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c) == PR_R08AE_017B_CALL_17AE);
    CHECK(c.ip == 0x17AE && c.sp == SP - 8 && c.ax == 0 && rd(r, SS, SP - 8) == 0x019F && rd(r, SS, SP - 4) == 0);
    c = setup(r, 0x019F, 5); c.sp = SP - 2; c.flags = 0x0ED7;
    CHECK(pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c) == PR_R08AE_017B_CALL_13B1);
    CHECK(c.ip == 0x13B1 && c.sp == SP - 6 && c.flags == 0x0ED7 && rd(r, SS, SP - 6) == 0x01A3 && rd(r, SS, SP - 4) == CS);
    c = setup(r, 0x01A3, 5); c.sp = SP - 2; wr(r, SS, SP - 2, 0xBEEF);
    CHECK(pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c) == PR_R08AE_017B_RETURN);
    CHECK(c.bp == 0xBEEF && c.ip == 0x7000 && c.cs == 0x9001 && c.sp == SP + 4);

    /* Aliasing: with SS == DS and the saved-BP slot at DS:5373 (SP = 5375h), the byte store lands on the low byte of the pushed BP. */
    c = setup(r, 0x017B, 2); c.ss = DS; c.sp = 0x5375; wr(r, DS, 0x5375, 0x7000); wr(r, DS, 0x5377, 0x9001); r[at(DS, 0x5372)] = 2;
    CHECK(pr_r08ae_entry_017b_cpu(r, PR_R08AE_017B_RAM_SIZE, &c) == PR_R08AE_017B_RETURN);
    CHECK(c.bp == 0x7802 && c.sp == 0x5379 && c.ip == 0x7000 && c.cs == 0x9001);

    /* ARGUMENT: wrong IP, SP outside [0006,FFFA], bad host objects; nothing changes. */
    for (k = 0; k < 6; ++k) {
        c = setup(r, 0x017B, 0);
        if (k == 0) c.ip = 0x017C;
        if (k == 1) c.sp = 0x0005;
        if (k == 2) c.sp = 0xFFFB;
        before = c; memcpy(saved, r, PR_R08AE_017B_RAM_SIZE);
        CHECK(pr_r08ae_entry_017b_cpu(k == 3 ? NULL : r, k == 4 ? 1 : PR_R08AE_017B_RAM_SIZE, k == 5 ? NULL : &c) == PR_R08AE_017B_ARGUMENT);
        if (k != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, PR_R08AE_017B_RAM_SIZE) == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
