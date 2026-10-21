#include "poolrad/recovered/resident_08ae_dispatch_00dc_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define CS 0x5800u
#define DS 0x5000u
#define SS 0x2800u
#define SP 0x8000u
#define RAM PR_R08AE_00DC_RAM_SIZE

static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }

/* A far-called entry: SS:[SP] return IP 7000, [SP+2] return CS 9001, [SP+4] the argument word. DS:5372 and DS:5374 are inputs. */
static pr_r08ae_00dc_cpu setup(uint8_t *r, unsigned ip, unsigned flag, unsigned w5374, unsigned argument)
{
    pr_r08ae_00dc_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, ip, DS, 0x8888, SS, 0x0202};
    memset(r, 0xA5, RAM);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001); wr(r, SS, SP + 4, argument);
    r[at(DS, 0x5372)] = (uint8_t)flag; wr(r, DS, 0x5374, w5374);
    return c;
}
/* A resumed state inside the function: the saved BP at SP-2 and BP pointing at it, so [BP+6] is the argument. */
static pr_r08ae_00dc_cpu resumed(uint8_t *r, unsigned ip, unsigned flag, unsigned w5374, unsigned argument)
{
    pr_r08ae_00dc_cpu c = setup(r, ip, flag, w5374, argument);
    c.sp = SP - 2; c.bp = SP - 2; wr(r, SS, SP - 2, 0x789A);
    return c;
}
/* The stop frame: the return IP and CS, then the pushed argument word when the callee takes one. */
static void frame(const uint8_t *r, const pr_r08ae_00dc_cpu *c, unsigned depth_words, unsigned return_ip, int has_arg, unsigned value)
{
    CHECK(c->sp == SP - 2 * depth_words);
    CHECK(rd(r, SS, c->sp) == return_ip && rd(r, SS, c->sp + 2u) == CS);
    if (has_arg) CHECK(rd(r, SS, c->sp + 4u) == value);
}
static pr_r08ae_00dc_status run(uint8_t *r, pr_r08ae_00dc_cpu *c) { return pr_r08ae_dispatch_00dc_cpu(r, RAM, c); }

int main(void)
{
    uint8_t *r = malloc(RAM), *saved = malloc(RAM);
    pr_r08ae_00dc_cpu c, before;
    unsigned k;
    CHECK(r && saved);

    /* DS:5372 == 2: no call; BP restored, CS:IP popped and the argument word dropped (RETF 2). */
    c = setup(r, 0x00DC, 2, 0xFFFF, 0);
    CHECK(run(r, &c) == PR_R08AE_00DC_RETURN);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP + 6 && c.bp == 0x789A);
    CHECK((c.flags & 0x08D5) == 0x0044 && c.ax == 0x1111);                      /* CMP 2,2: ZF and PF; nothing else is touched */

    /* DS:5374 != 0 first calls 137C (no argument), return IP 00F4. */
    c = setup(r, 0x00DC, 0, 1, 0);
    CHECK(run(r, &c) == PR_R08AE_00DC_CALL_137C);
    CHECK(c.ip == 0x137C && c.cs == CS && c.bp == SP - 2); frame(r, &c, 3, 0x00F4, 0, 0);
    CHECK(rd(r, SS, SP - 2) == 0x789A);
    c = setup(r, 0x00DC, 0, 0x0100, 0);                                         /* the whole WORD is tested, not its low byte */
    CHECK(run(r, &c) == PR_R08AE_00DC_CALL_137C);

    /* DS:5374 == 0 skips 137C: the argument selects the first callee. */
    c = setup(r, 0x00DC, 0, 0, 0); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_17AE); CHECK(c.ip == 0x17AE); frame(r, &c, 4, 0x0103, 1, 0);
    c = setup(r, 0x00DC, 0, 0, 1); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_181A); CHECK(c.ip == 0x181A); frame(r, &c, 4, 0x0120, 1, 1);
    c = setup(r, 0x00DC, 0, 0, 0xFF); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_17AE); frame(r, &c, 4, 0x012E, 1, 0);
    c = setup(r, 0x00DC, 1, 0, 7); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_1742); frame(r, &c, 4, 0x0147, 1, 6);     /* [5372]==1: 1742(A-1) */
    c = setup(r, 0x00DC, 0, 0, 7); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_17AE); frame(r, &c, 4, 0x0156, 1, 6);     /* [5372]==0: 17AE(A-1) */
    c = setup(r, 0x00DC, 0, 0, 0x100); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_17AE);                                /* A != 0, 1, FF: falls to [5372] */
    CHECK(rd(r, SS, c.sp + 4u) == 0xFF && (c.flags & 0x08D5) == 0x0014);                                          /* DEC 100 -> FF: AF and PF, CF kept clear */
    c = setup(r, 0x00DC, 3, 0, 7); CHECK(run(r, &c) == PR_R08AE_00DC_RETURN); CHECK(c.sp == SP + 6);               /* [5372]==3: no callee, 5374==0 */
    c = setup(r, 0x00DC, 3, 0, 0xFFFF);                                         /* A = FFFF is not 00FF (a word compare): falls to [5372] == 3, nothing to call */
    CHECK(run(r, &c) == PR_R08AE_00DC_RETURN);

    /* The post-return entries: each pushes its argument and frame and stops at the next callee, or returns. */
    c = resumed(r, 0x0103, 0, 0, 0); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_1742); CHECK(c.ip == 0x1742 && c.sp == SP - 8); CHECK(rd(r, SS, c.sp) == 0x010A);
    c = resumed(r, 0x010A, 0, 0, 0); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_181A); CHECK(rd(r, SS, c.sp) == 0x0111);
    c = resumed(r, 0x012E, 0, 0, 0); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_1742); CHECK(rd(r, SS, c.sp) == 0x0135);
    c = resumed(r, 0x0165, 0, 0, 0); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_17AE); CHECK(rd(r, SS, c.sp) == 0x016C);
    c = resumed(r, 0x016C, 0, 0, 0); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_1742); CHECK(rd(r, SS, c.sp) == 0x0173);
    c = resumed(r, 0x0173, 0, 0, 0); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_13B1); CHECK(c.ip == 0x13B1 && c.sp == SP - 6 && rd(r, SS, c.sp) == 0x0177);
    for (k = 0; k < 5; ++k) {                                                  /* the join at 0156: DS:5374 != 0 calls 006C(0Ah) then 17AE/1742/13B1 */
        static const unsigned entries[5] = {0x0111, 0x0120, 0x0135, 0x0147, 0x0156};
        c = resumed(r, entries[k], 0, 1, 0); CHECK(run(r, &c) == PR_R08AE_00DC_CALL_006C); CHECK(c.ip == 0x006C && c.ax == 0x000A);
        CHECK(rd(r, SS, c.sp) == 0x0165 && rd(r, SS, c.sp + 4u) == 0x000A);
        c = resumed(r, entries[k], 0, 0, 0); CHECK(run(r, &c) == PR_R08AE_00DC_RETURN);
        CHECK(c.bp == 0x789A && c.ip == 0x7000 && c.cs == 0x9001 && c.sp == SP + 6);
    }
    c = resumed(r, 0x0177, 0, 0, 0); CHECK(run(r, &c) == PR_R08AE_00DC_RETURN); CHECK(c.sp == SP + 6);

    /* A write by an earlier callee is seen at the original read sites: the join rereads DS:5374 after the callee changed it. */
    c = resumed(r, 0x0156, 0, 0, 0); wr(r, DS, 0x5374, 0x0100);
    CHECK(run(r, &c) == PR_R08AE_00DC_CALL_006C);

    /* Aliasing: SS == DS and SP = 536E, so BP = 536C and the argument word [BP+6] overlaps the flag byte DS:5372. */
    c = setup(r, 0x00DC, 1, 0, 0); c.ss = DS; c.sp = 0x536E; wr(r, DS, 0x536E, 0x7000); wr(r, DS, 0x5370, 0x9001); r[at(DS, 0x5372)] = 1; wr(r, DS, 0x5374, 0);
    CHECK(run(r, &c) == PR_R08AE_00DC_CALL_1742);                               /* BP = 536C, so [BP+6] is the word at DS:5372 (flag byte 01, then A5) */
    CHECK(c.sp == 0x5366 && rd(r, DS, c.sp + 4u) == 0xA500);                  /* [5372] == 1: 1742(A-1) with A = A501 */

    /* ARGUMENT: wrong IP, SP outside [0008,FFFA], bad host objects; nothing changes. */
    for (k = 0; k < 7; ++k) {
        c = setup(r, 0x00DC, 0, 0, 0);
        if (k == 0) c.ip = 0x00DD;
        if (k == 1) c.sp = 0x0007;
        if (k == 2) c.sp = 0xFFFB;
        if (k == 3) c.ip = 0x0178;
        before = c; memcpy(saved, r, RAM);
        CHECK(pr_r08ae_dispatch_00dc_cpu(k == 4 ? NULL : r, k == 5 ? 1 : RAM, k == 6 ? NULL : &c) == PR_R08AE_00DC_ARGUMENT);
        if (k != 6) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, RAM) == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
