#include "poolrad/recovered/system_iocheck_04a9_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define RAM PR_SYSTEM_IOCHECK_RAM_SIZE
#define CS 0x5800u
#define DS 0x5000u
#define SS 0x2800u
#define SP 0x8000u
static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static pr_system_iocheck_cpu setup(uint8_t *r, unsigned in_out_res)
{
    pr_system_iocheck_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, 0x04A9, DS, 0x8888, SS, 0x0202};
    memset(r, 0xA5, RAM);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001);
    wr(r, DS, 0x4426, in_out_res);
    return c;
}
int main(void)
{
    uint8_t *r = malloc(RAM), *saved = malloc(RAM);
    pr_system_iocheck_cpu c, before;
    unsigned v, k;
    CHECK(r && saved);

    /* InOutRes == 0: RETF with the frame popped; only the flags and IP/CS/SP change. */
    c = setup(r, 0);
    CHECK(pr_system_iocheck_04a9(r, RAM, &c) == PR_SYSTEM_IOCHECK_RETURNED);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP + 4);
    CHECK((c.flags & 0x08D5) == 0x0044 && (c.flags & ~0x08D5) == 0x0202);       /* CMP 0,0: ZF and PF */
    CHECK(c.ax == 0x1111 && c.bx == 0x2222 && c.bp == 0x789A && c.ds == DS && c.ss == SS);

    /* Every non-zero InOutRes is the terminal stop, never a return: asserted for all 65535 values. */
    for (v = 1; v < 65536u; ++v) {
        c = setup(r, v);
        CHECK(pr_system_iocheck_04a9(r, RAM, &c) != PR_SYSTEM_IOCHECK_RETURNED);
        if (v < 0x100u || v == 0x8000u || v == 0xFFFFu || (v & 0x0FFFu) == 0x0ABCu) {
            CHECK(c.ip == 0x00E2u && c.ax == v && c.sp == SP && c.cs == CS && c.bp == 0x789A);
            CHECK((c.flags & 0x0801) == 0 && ((c.flags & 0x80) != 0) == ((v & 0x8000u) != 0) && (c.flags & 0x40) == 0);
        }
    }
    c = setup(r, 0x0067);                                                         /* 103: file not open */
    CHECK(pr_system_iocheck_04a9(r, RAM, &c) == PR_SYSTEM_IOCHECK_RUNERROR);
    CHECK(c.ip == 0x00E2 && c.ax == 0x0067 && (c.flags & 0x08D5) == 0x0000);     /* 67h has five set bits: PF clear */
    CHECK(c.sp == SP && r[at(SS, SP)] == 0x00 && r[at(SS, SP + 1)] == 0x70);   /* the return frame is left for 00E2 to pop */
    c = setup(r, 0x0100);                                                         /* the WORD is tested, not its low byte */
    CHECK(pr_system_iocheck_04a9(r, RAM, &c) == PR_SYSTEM_IOCHECK_RUNERROR && c.ax == 0x0100 && (c.flags & 0x08D5) == 0x0004);    /* low byte 00: PF; the word is nonzero: ZF clear */

    /* Aliasing: SS == DS with SP at 4426 reads the frame's own words: InOutRes is the return IP. */
    c = setup(r, 0); c.ss = DS; c.sp = 0x4426; wr(r, DS, 0x4426, 0x7000); wr(r, DS, 0x4428, 0x9001);
    CHECK(pr_system_iocheck_04a9(r, RAM, &c) == PR_SYSTEM_IOCHECK_RUNERROR && c.ax == 0x7000);
    c = setup(r, 0); c.ss = DS; c.sp = 0x4426; wr(r, DS, 0x4426, 0);
    CHECK(pr_system_iocheck_04a9(r, RAM, &c) == PR_SYSTEM_IOCHECK_RETURNED && c.ip == 0 && c.cs == 0xA5A5);

    /* ARGUMENT: wrong IP, SP above FFFC, bad host objects; nothing changes. */
    for (k = 0; k < 5; ++k) {
        c = setup(r, 5);
        if (k == 0) c.ip = 0x04AA;
        if (k == 1) c.sp = 0xFFFD;
        before = c; memcpy(saved, r, RAM);
        CHECK(pr_system_iocheck_04a9(k == 2 ? NULL : r, k == 3 ? 1 : RAM, k == 4 ? NULL : &c) == PR_SYSTEM_IOCHECK_ARGUMENT);
        if (k != 4) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, RAM) == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
