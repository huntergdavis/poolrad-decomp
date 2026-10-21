#include "poolrad/recovered/resident_0802_0040_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define RAM PR_R0802_0040_RAM_SIZE
#define CS 0x5800u
#define DS 0x5000u
#define SS 0x2800u
#define SP 0x8000u
#define INTR_SEG 0x6000u
#define HSEG 0x7800u
static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }

/* The INT 10h handler: logs AX BX CX DX at entry and returns as an IRET (it changes AX so the Regs write-back is visible). */
static struct { unsigned ax, bx, cx, dx; } calls[8];
static unsigned ncalls;
static int refuse_at;      /* 1-based call number the handler refuses, 0 = never */
static int video(void *ctx, uint8_t *ram, const pr_dos_intr_cpu *before, pr_dos_intr_cpu *after)
{
    (void)ctx;
    if (ncalls < 8) { calls[ncalls].ax = before->ax; calls[ncalls].bx = before->bx; calls[ncalls].cx = before->cx; calls[ncalls].dx = before->dx; }
    ++ncalls;
    if (refuse_at && (int)ncalls == refuse_at) return 1;
    *after = *before;
    after->ax = 0xAA00u | (before->ax & 0xFFu);
    after->ip = (uint16_t)rd(ram, before->ss, before->sp); after->cs = (uint16_t)rd(ram, before->ss, before->sp + 2u);
    after->flags = (uint16_t)rd(ram, before->ss, before->sp + 4u); after->sp = (uint16_t)(before->sp + 6u);
    return 0;
}
/* Arguments: char, count, attribute index, row, column. DS:4A1A page 7, DS:4A1B page 3; the attribute map 25D5 holds 0A0h + index. */
static pr_r0802_0040_cpu setup(uint8_t *r, unsigned ega, unsigned ch, unsigned count, unsigned attr, unsigned row, unsigned col)
{
    pr_r0802_0040_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, 0x0040, DS, 0x8888, SS, 0x0202};
    unsigned k;
    memset(r, 0x5A, RAM);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001);
    wr(r, SS, SP + 4, 0xAB00 | ch); wr(r, SS, SP + 6, 0xAB00 | count); wr(r, SS, SP + 8, 0xAB00 | attr);
    wr(r, SS, SP + 10, 0xAB00 | row); wr(r, SS, SP + 12, 0xAB00 | col);
    r[at(DS, 0x4A1A)] = 7; r[at(DS, 0x4A1B)] = 3; r[at(DS, 0x4A1E)] = (uint8_t)ega;
    for (k = 0; k < 256; ++k) r[at(DS, 0x25D5 + k)] = (uint8_t)(0xA0 + k);
    for (k = 0; k < 0x14; ++k) r[at(DS, 0x4A02 + k)] = (uint8_t)(0xC0 + k);                       /* stale Regs */
    wr(r, 0, 0x40, 0x0000); wr(r, 0, 0x42, HSEG);                                                  /* IVT[10h] */
    wr(r, CS, 0x65, INTR_SEG); wr(r, CS, 0xA8, INTR_SEG); wr(r, CS, 0xD3, INTR_SEG); wr(r, CS, 0xFF, INTR_SEG);   /* the four relocated segment words */
    ncalls = 0; refuse_at = 0;
    return c;
}
static pr_r0802_0040_status run(uint8_t *r, pr_r0802_0040_cpu *c, pr_r0802_0040_diagnostic *d)
{
    pr_r0802_0040_env env = {video, NULL};
    return pr_r0802_0040(r, RAM, c, &env, d);
}

int main(void)
{
    uint8_t *r = malloc(RAM), *saved = malloc(RAM);
    pr_r0802_0040_cpu c, before;
    pr_r0802_0040_diagnostic d;
    unsigned k;
    CHECK(r && saved);

    /* DS:4A1E = 0: the attribute is translated through the map and there is ONE write, on page 3. Two INT 10h calls. */
    c = setup(r, 0, 'Q', 5, 0x12, 24, 9);
    CHECK(run(r, &c, &d) == PR_R0802_0040_RETURNED && d.calls == 2 && ncalls == 2);
    CHECK((calls[0].ax >> 8) == 0x02 && (calls[0].bx >> 8) == 3 && calls[0].dx == ((24u << 8) | 9u));     /* AH=02 set cursor: BH page, DH row, DL col */
    CHECK((calls[1].ax >> 8) == 0x09 && (calls[1].ax & 0xFF) == 'Q' && (calls[1].bx >> 8) == 3 && (calls[1].bx & 0xFF) == 0xB2 && calls[1].cx == 5);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP + 14 && c.bp == 0x789A);
    CHECK(rd(r, SS, SP - 2) == 0x789A);                                                                   /* the pushed BP stays below the frame */
    /* Regs (DS:4A02) after: the handler's AX written back by Intr; BL, CX, DX as set. */
    CHECK(r[at(DS, 0x4A03)] == 0xAA && r[at(DS, 0x4A04)] == 0xB2 && rd(r, DS, 0x4A06) == 5 && r[at(DS, 0x4A08)] == 9 && r[at(DS, 0x4A09)] == 24);

    /* DS:4A1E = 1: no translation (BL is the index itself) and a SECOND pair on page 7. Four calls. */
    c = setup(r, 1, 'Z', 1, 0x12, 3, 4);
    CHECK(run(r, &c, &d) == PR_R0802_0040_RETURNED && d.calls == 4 && ncalls == 4);
    CHECK((calls[1].bx & 0xFF) == 0x12 && (calls[1].bx >> 8) == 3);
    CHECK((calls[2].ax >> 8) == 0x02 && (calls[2].bx >> 8) == 7 && calls[2].dx == ((3u << 8) | 4u));
    CHECK((calls[3].ax >> 8) == 0x09 && (calls[3].ax & 0xFF) == 'Z' && (calls[3].bx >> 8) == 7 && (calls[3].bx & 0xFF) == 0x12 && calls[3].cx == 1);
    CHECK(c.sp == SP + 14 && c.ip == 0x7000);

    /* DS:4A1E = 2 (and 0FFh): NOT the same as 1. The attribute is untranslated like 1, but there is only ONE write, like 0. */
    for (k = 0; k < 2; ++k) {
        c = setup(r, k ? 0xFF : 2, 'Y', 2, 0x12, 1, 1);
        CHECK(run(r, &c, &d) == PR_R0802_0040_RETURNED && d.calls == 2 && ncalls == 2);
        CHECK((calls[1].bx & 0xFF) == 0x12 && (calls[1].bx >> 8) == 3 && calls[1].cx == 2);
    }

    /* The argument slots are bytes: the high byte of each word slot is ignored. The count is a zero-extended byte. */
    c = setup(r, 0, 'A', 0xFF, 0xFF, 0xFF, 0xFF);
    CHECK(run(r, &c, &d) == PR_R0802_0040_RETURNED && calls[1].cx == 0x00FF && (calls[1].bx & 0xFF) == 0x9F && calls[0].dx == 0xFFFF);
    c = setup(r, 0, 'A', 0, 0, 0, 0);
    CHECK(run(r, &c, &d) == PR_R0802_0040_RETURNED && calls[1].cx == 0 && (calls[1].bx & 0xFF) == 0xA0);

    /* A refusing INT 10h handler: INTR, with the diagnostic naming SERVICE and how many Intr calls completed. */
    c = setup(r, 1, 'Q', 1, 0x12, 1, 1); refuse_at = 3;
    CHECK(run(r, &c, &d) == PR_R0802_0040_INTR && d.intr_status == PR_DOS_INTR_SERVICE && d.calls == 2);
    c = setup(r, 0, 'Q', 1, 0x12, 1, 1); refuse_at = 1;
    CHECK(run(r, &c, &d) == PR_R0802_0040_INTR && d.calls == 0 && c.cs == HSEG);

    /* Intr must come back to the instruction after the call: a handler that returns to the wrong IP is a TRANSFER. */
    c = setup(r, 0, 'Q', 1, 0x12, 1, 1);
    wr(r, CS, 0x65, INTR_SEG);                                                                            /* unchanged: the transfer is Intr's own return IP */
    CHECK(run(r, &c, &d) == PR_R0802_0040_RETURNED);

    /* ARGUMENT: wrong IP, entry SP outside [0022,FFF0], bad host objects; nothing changes. */
    for (k = 0; k < 7; ++k) {
        pr_r0802_0040_env env = {video, NULL}, bad = {NULL, NULL};
        c = setup(r, 0, 'Q', 1, 0x12, 1, 1);
        if (k == 0) c.ip = 0x0041;
        if (k == 1) c.sp = 0x0021;
        if (k == 2) c.sp = 0xFFF1;
        before = c; memcpy(saved, r, RAM);
        CHECK(pr_r0802_0040(k == 3 ? NULL : r, k == 4 ? 1 : RAM, k == 5 ? NULL : &c, k == 6 ? &bad : &env, NULL) == PR_R0802_0040_ARGUMENT);
        if (k != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, RAM) == 0 && ncalls == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
