#include "poolrad/recovered/resident_08ae_state_01a5_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define CS 0x5800u
#define DS 0x5000u
#define SS 0x2800u
#define SP 0x8000u
#define RAM PR_R08AE_01A5_RAM_SIZE

static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }

/* A far-called entry: SS:[SP] return IP 7000, [SP+2] return CS 9001, [SP+4] the argument word. DS:5372 and DS:5374 are inputs. */
static pr_r08ae_01a5_cpu setup(uint8_t *r, unsigned ip, unsigned flag, unsigned w5374, unsigned argument)
{
    pr_r08ae_01a5_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, ip, DS, 0x8888, SS, 0x0202};
    memset(r, 0xA5, RAM);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001); wr(r, SS, SP + 4, argument);
    r[at(DS, 0x5372)] = (uint8_t)flag; wr(r, DS, 0x5374, w5374);
    return c;
}
/* A resumed state: the saved BP at SP-2 and BP pointing at it, so [BP+6] is the argument. */
static pr_r08ae_01a5_cpu resumed(uint8_t *r, unsigned ip, unsigned flag, unsigned w5374, unsigned argument)
{
    pr_r08ae_01a5_cpu c = setup(r, ip, flag, w5374, argument);
    c.sp = SP - 2; c.bp = SP - 2; wr(r, SS, SP - 2, 0x789A);
    return c;
}
static pr_r08ae_01a5_status run(uint8_t *r, pr_r08ae_01a5_cpu *c) { return pr_r08ae_state_01a5_cpu(r, RAM, c); }

int main(void)
{
    uint8_t *r = malloc(RAM), *saved = malloc(RAM);
    pr_r08ae_01a5_cpu c, before;
    unsigned k;
    CHECK(r && saved);

    /* DS:5372 == 2: nothing is called or stored; BP restored, CS:IP popped, the argument word dropped. */
    c = setup(r, 0x01A5, 2, 0x1234, 9);
    CHECK(run(r, &c) == PR_R08AE_01A5_RETURN);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP + 6 && c.bp == 0x789A && rd(r, DS, 0x5374) == 0x1234);
    CHECK((c.flags & 0x08D5) == 0x0044 && c.ax == 0x1111);                      /* CMP 2,2: ZF and PF; nothing else is touched */

    /* A == 0 calls 137C (return IP 01C8, no argument), whatever DS:5374 holds. */
    for (k = 0; k < 2; ++k) {
        c = setup(r, 0x01A5, 0, k ? 0xFFFF : 0, 0);
        CHECK(run(r, &c) == PR_R08AE_01A5_CALL_137C);
        CHECK(c.ip == 0x137C && c.cs == CS && c.bp == SP - 2 && c.sp == SP - 6);
        CHECK(rd(r, SS, c.sp) == 0x01C8 && rd(r, SS, c.sp + 2u) == CS && rd(r, SS, SP - 2) == 0x789A);
        CHECK((c.flags & 0x08D5) == 0x0044 && rd(r, DS, 0x5374) == (k ? 0xFFFF : 0));
    }

    /* A != 0 and DS:5374 == 0 calls 13B1 (return IP 01C2); the whole WORD is tested, so 0100 is nonzero. */
    c = setup(r, 0x01A5, 0, 0, 5);
    CHECK(run(r, &c) == PR_R08AE_01A5_CALL_13B1);
    CHECK(c.ip == 0x13B1 && c.sp == SP - 6 && rd(r, SS, c.sp) == 0x01C2 && rd(r, SS, c.sp + 2u) == CS);
    c = setup(r, 0x01A5, 0, 0x0100, 5);
    CHECK(run(r, &c) == PR_R08AE_01A5_RETURN);

    /* A != 0 and DS:5374 != 0 skips the call and stores A at DS:5374. */
    c = setup(r, 0x01A5, 3, 7, 0x1234);
    CHECK(run(r, &c) == PR_R08AE_01A5_RETURN);
    CHECK(rd(r, DS, 0x5374) == 0x1234 && c.ax == 0x1234 && c.sp == SP + 6 && c.bp == 0x789A && c.ip == 0x7000 && c.cs == 0x9001);
    CHECK((c.flags & 0x08D5) == 0x0000);                                       /* CMP 7,0: nothing set; PF is even parity of 07 = odd: clear */

    /* The post-return entries store A and return; 01C2 and 01C8 are the same continuation. */
    for (k = 0; k < 2; ++k) {
        c = resumed(r, k ? 0x01C8 : 0x01C2, 0, 0x4444, 0xBEEF);
        CHECK(run(r, &c) == PR_R08AE_01A5_RETURN);
        CHECK(rd(r, DS, 0x5374) == 0xBEEF && c.ax == 0xBEEF && c.bp == 0x789A && c.ip == 0x7000 && c.cs == 0x9001 && c.sp == SP + 6);
    }

    /* Aliasing: SS == DS and SP = 5370 put BP at 536E, so [BP+6] is DS:5374 itself. A nonzero word there takes the no-call path and
     * rewrites the same value; a zero word calls 137C. */
    c = setup(r, 0x01A5, 0, 0, 0); c.ss = DS; c.sp = 0x5370; wr(r, DS, 0x5370, 0x7000); wr(r, DS, 0x5372, 0x9001); r[at(DS, 0x5372)] = 0; wr(r, DS, 0x5374, 5);
    CHECK(run(r, &c) == PR_R08AE_01A5_RETURN);
    CHECK(rd(r, DS, 0x5374) == 5 && c.ip == 0x7000 && c.sp == 0x5376);
    c = setup(r, 0x01A5, 0, 0, 0); c.ss = DS; c.sp = 0x5370; wr(r, DS, 0x5370, 0x7000); r[at(DS, 0x5372)] = 0; wr(r, DS, 0x5374, 0);
    CHECK(run(r, &c) == PR_R08AE_01A5_CALL_137C);

    /* ARGUMENT: wrong IP, SP outside [0006,FFFA], bad host objects; nothing changes. */
    for (k = 0; k < 7; ++k) {
        c = setup(r, 0x01A5, 0, 0, 0);
        if (k == 0) c.ip = 0x01A6;
        if (k == 1) c.sp = 0x0005;
        if (k == 2) c.sp = 0xFFFB;
        if (k == 3) c.ip = 0x01CE;
        before = c; memcpy(saved, r, RAM);
        CHECK(pr_r08ae_state_01a5_cpu(k == 4 ? NULL : r, k == 5 ? 1 : RAM, k == 6 ? NULL : &c) == PR_R08AE_01A5_ARGUMENT);
        if (k != 6) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, RAM) == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
