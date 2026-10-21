#include "poolrad/recovered/system_filecheck_14b6_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define RAM PR_SYSTEM_FILECHECK_RAM_SIZE
#define CS 0x5800u
#define DS 0x5000u
#define SS 0x2800u
#define DATA 0x3000u
#define SP 0x8000u
static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }
/* A near-called state: the return IP 149A at SS:SP; the file variable at DATA:0100 = [handle][magic]; InOutRes is DS:4426. */
static pr_system_filecheck_cpu setup(uint8_t *r, unsigned magic)
{
    pr_system_filecheck_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x0100, 0x789A, SP, CS, 0x14B6, DS, DATA, SS, 0x0202};
    memset(r, 0xA5, RAM);
    wr(r, SS, SP, 0x149A); wr(r, DATA, 0x0100, 5); wr(r, DATA, 0x0102, magic); wr(r, DS, 0x4426, 0x1234);
    return c;
}
int main(void)
{
    uint8_t *r = malloc(RAM), *saved = malloc(RAM);
    pr_system_filecheck_cpu c, before;
    unsigned k;
    CHECK(r && saved);

    /* Magic D7B3: equal. InOutRes is untouched; flags are those of CMP D7B3,D7B3 (ZF, PF); RET to the slot. */
    c = setup(r, 0xD7B3);
    CHECK(pr_system_filecheck_14b6(r, RAM, &c) == PR_SYSTEM_FILECHECK_RETURNED);
    CHECK(c.ip == 0x149A && c.sp == SP + 2 && c.cs == CS && rd(r, DS, 0x4426) == 0x1234 && (c.flags & 0x08D5) == 0x0044 && (c.flags & ~0x08D5) == 0x0202);
    CHECK(c.ax == 0x1111 && c.bx == 0x2222 && c.di == 0x0100 && c.es == DATA && c.bp == 0x789A);

    /* Any other magic: InOutRes := 67h and the flags of the CMP (so the caller's JNE is taken). */
    for (k = 0; k < 6; ++k) {
        static const unsigned magics[6] = {0xD7B0, 0xD7B1, 0xD7B2, 0xD7B4, 0x0000, 0xFFFF};
        c = setup(r, magics[k]);
        CHECK(pr_system_filecheck_14b6(r, RAM, &c) == PR_SYSTEM_FILECHECK_RETURNED);
        CHECK(rd(r, DS, 0x4426) == 0x0067 && c.ip == 0x149A && (c.flags & 0x40) == 0);
    }
    c = setup(r, 0xD7B2);                                                         /* D7B2 - D7B3 = FFFF: CF, AF, SF, PF */
    pr_system_filecheck_14b6(r, RAM, &c);
    CHECK((c.flags & 0x08D5) == 0x0095);
    c = setup(r, 0xD7B4);                                                         /* D7B4 - D7B3 = 1: none of CF/ZF/SF/OF; odd parity: PF clear */
    pr_system_filecheck_14b6(r, RAM, &c);
    CHECK((c.flags & 0x08D5) == 0x0000);
    c = setup(r, 0x7FFF);                                                         /* 7FFF - D7B3: +32767 - (-10317) overflows */
    pr_system_filecheck_14b6(r, RAM, &c);
    CHECK((c.flags & 0x0800) == 0x0800 && (c.flags & 0x01) == 0x01);

    /* The offset DI+2 wraps at 16 bits inside ES; the word at FFFF straddles linearly. */
    c = setup(r, 0xD7B3); c.di = 0xFFFE; wr(r, DATA, 0x0000, 0xD7B3);
    CHECK(pr_system_filecheck_14b6(r, RAM, &c) == PR_SYSTEM_FILECHECK_RETURNED && rd(r, DS, 0x4426) == 0x1234);
    c = setup(r, 0xD7B3); c.di = 0xFFFE; wr(r, DATA, 0x0000, 0x0000);
    CHECK(pr_system_filecheck_14b6(r, RAM, &c) == PR_SYSTEM_FILECHECK_RETURNED && rd(r, DS, 0x4426) == 0x0067);

    /* The RET rereads its slot AFTER the store: SS == DS and SP = 4426h put InOutRes on the return slot, so the helper returns to 0067h. */
    c = setup(r, 0xD7B1); c.ss = DS; c.sp = 0x4426; wr(r, DS, 0x4426, 0x149A);
    CHECK(pr_system_filecheck_14b6(r, RAM, &c) == PR_SYSTEM_FILECHECK_RETURNED && c.ip == 0x0067 && c.sp == 0x4428);
    c = setup(r, 0xD7B3); c.ss = DS; c.sp = 0x4426; wr(r, DS, 0x4426, 0x149A);
    CHECK(pr_system_filecheck_14b6(r, RAM, &c) == PR_SYSTEM_FILECHECK_RETURNED && c.ip == 0x149A);

    /* ARGUMENT: wrong IP, SP above FFFC, bad host objects; nothing changes. */
    for (k = 0; k < 5; ++k) {
        c = setup(r, 0xD7B1);
        if (k == 0) c.ip = 0x14B7;
        if (k == 1) c.sp = 0xFFFD;
        before = c; memcpy(saved, r, RAM);
        CHECK(pr_system_filecheck_14b6(k == 2 ? NULL : r, k == 3 ? 1 : RAM, k == 4 ? NULL : &c) == PR_SYSTEM_FILECHECK_ARGUMENT);
        if (k != 4) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, RAM) == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
