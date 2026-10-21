#include "poolrad/recovered/resident_0802_0105_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define RAM PR_R0802_0105_RAM_SIZE
#define CS 0x5800u
#define DS 0x5000u
#define SS 0x2800u
#define SP 0x8000u
#define INTR_SEG 0x6000u
#define HSEG 0x7800u
#define SYS_SEG 0x6100u
#define MSG_SEG 0x4800u
static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }

static struct { unsigned ax, bx, cx, dx; } calls[2200];
static unsigned ncalls;
static int refuse_at;
static int video(void *ctx, uint8_t *ram, const pr_dos_intr_cpu *before, pr_dos_intr_cpu *after)
{
    (void)ctx;
    if (ncalls < 2200) { calls[ncalls].ax = before->ax; calls[ncalls].bx = before->bx; calls[ncalls].cx = before->cx; calls[ncalls].dx = before->dx; }
    ++ncalls;
    if (refuse_at && (int)ncalls == refuse_at) return 1;
    *after = *before;
    after->ip = (uint16_t)rd(ram, before->ss, before->sp); after->cs = (uint16_t)rd(ram, before->ss, before->sp + 2u);
    after->flags = (uint16_t)rd(ram, before->ss, before->sp + 4u); after->sp = (uint16_t)(before->sp + 6u);
    return 0;
}
/* A message "text" of length n at MSG_SEG:0100; args attribute, row, first column. DS:4A1B page 3, 4A1A page 7, the map 25D5 = A0h + index. */
static pr_r0802_0105_cpu setup(uint8_t *r, unsigned ega, const char *text, unsigned n, unsigned attr, unsigned row, unsigned col)
{
    pr_r0802_0105_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, 0x0105, DS, 0x8888, SS, 0x0202};
    unsigned k;
    memset(r, 0x5A, RAM);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001);
    wr(r, SS, SP + 4, 0x0100); wr(r, SS, SP + 6, MSG_SEG);
    wr(r, SS, SP + 8, 0xAB00 | attr); wr(r, SS, SP + 10, 0xCD00 | row); wr(r, SS, SP + 12, 0xEF00 | col);
    r[at(MSG_SEG, 0x0100)] = (uint8_t)n;
    for (k = 0; k < n; ++k) r[at(MSG_SEG, 0x0101 + k)] = (uint8_t)(text ? text[k % strlen(text)] : 'x');
    r[at(DS, 0x4A1A)] = 7; r[at(DS, 0x4A1B)] = 3; r[at(DS, 0x4A1E)] = (uint8_t)ega;
    for (k = 0; k < 256; ++k) r[at(DS, 0x25D5 + k)] = (uint8_t)(0xA0 + k);
    wr(r, 0, 0x40, 0x0000); wr(r, 0, 0x42, HSEG);
    wr(r, CS, 0x65, INTR_SEG); wr(r, CS, 0xA8, INTR_SEG); wr(r, CS, 0xD3, INTR_SEG); wr(r, CS, 0xFF, INTR_SEG);   /* 0040's relocated Intr segments */
    wr(r, CS, 0x11C, 0x0B25); wr(r, CS, 0x11E, SYS_SEG);                                                         /* 0105's StrStore far call */
    wr(r, CS, 0x160, (unsigned)(0x0040 - 0x0162) & 0xFFFFu);                                                     /* 0105's near call to 0040 */
    ncalls = 0; refuse_at = 0;
    return c;
}
static pr_r0802_0105_status run(uint8_t *r, pr_r0802_0105_cpu *c, pr_r0802_0105_diagnostic *d)
{
    pr_r0802_0040_env env = {video, NULL};
    return pr_r0802_0105(r, RAM, c, &env, d);
}

int main(void)
{
    uint8_t *r = malloc(RAM), *saved = malloc(RAM);
    pr_r0802_0105_cpu c, before;
    pr_r0802_0105_diagnostic d;
    unsigned k;
    CHECK(r && saved);

    /* "Hi" with DS:4A1E = 0: two characters, each one 0040 call = two INT 10h calls; the column byte advances in place. */
    c = setup(r, 0, "Hi", 2, 0x12, 24, 3);
    CHECK(run(r, &c, &d) == PR_R0802_0105_RETURNED && d.chars == 2 && ncalls == 4);
    CHECK((calls[0].ax >> 8) == 2 && (calls[0].bx >> 8) == 3 && calls[0].dx == ((24u << 8) | 3u));
    CHECK((calls[1].ax >> 8) == 9 && (calls[1].ax & 0xFF) == 'H' && (calls[1].bx & 0xFF) == 0xB2 && calls[1].cx == 1);
    CHECK(calls[2].dx == ((24u << 8) | 4u) && (calls[3].ax & 0xFF) == 'i' && calls[3].cx == 1);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP + 14 && c.bp == 0x789A);
    CHECK((rd(r, SS, SP + 12) & 0xFF) == 5);                                                                    /* the caller's column slot was incremented twice */
    CHECK(r[at(SS, SP - 2 - 0x100)] == 2 && r[at(SS, SP - 2 - 0x100 + 1)] == 'H');                               /* the Pascal copy sits at BP-100h */

    /* An empty message: no 0040 call at all (JA 016F), flags are those of CMP 1,0. */
    c = setup(r, 0, "", 0, 0x12, 24, 3);
    CHECK(run(r, &c, &d) == PR_R0802_0105_RETURNED && d.chars == 0 && ncalls == 0 && (c.flags & 0x08D5) == 0x0000);
    /* DS:4A1E = 1: four INT 10h calls per character. */
    c = setup(r, 1, "AB", 2, 0x12, 5, 9);
    CHECK(run(r, &c, &d) == PR_R0802_0105_RETURNED && d.chars == 2 && ncalls == 8);
    CHECK((calls[2].bx >> 8) == 7 && (calls[3].bx >> 8) == 7 && (calls[4].dx & 0xFF) == 10 && (calls[5].ax & 0xFF) == 'B');
    /* DS:4A1E = 2: untranslated, one page: two INT 10h calls per character. */
    c = setup(r, 2, "Z", 1, 0x12, 5, 9);
    CHECK(run(r, &c, &d) == PR_R0802_0105_RETURNED && ncalls == 2 && (calls[1].bx & 0xFF) == 0x12);

    /* The column byte wraps at 256; a 255-character message runs 255 iterations (the index is a byte compared after the increment). */
    c = setup(r, 0, "ab", 2, 0x12, 1, 0xFF);
    CHECK(run(r, &c, &d) == PR_R0802_0105_RETURNED && (calls[0].dx & 0xFF) == 0xFF && (calls[2].dx & 0xFF) == 0x00 && (rd(r, SS, SP + 12) & 0xFF) == 1);
    c = setup(r, 2, "0123456789", 255, 0x12, 1, 0);
    CHECK(run(r, &c, &d) == PR_R0802_0105_RETURNED && d.chars == 255 && ncalls == 510 && (calls[508].dx & 0xFF) == 254 && (calls[509].ax & 0xFF) == (unsigned)"0123456789"[254 % 10]);

    /* A refusing INT 10h handler: DISPLAY, naming how many characters completed and Intr's SERVICE status. */
    c = setup(r, 0, "ABC", 3, 0x12, 1, 0); refuse_at = 3;
    CHECK(run(r, &c, &d) == PR_R0802_0105_DISPLAY && d.chars == 1 && d.display.intr_status == PR_DOS_INTR_SERVICE);

    /* ARGUMENT: wrong IP, entry SP outside [0140,FFF0], bad host objects; nothing changes. */
    for (k = 0; k < 7; ++k) {
        pr_r0802_0040_env env = {video, NULL}, bad = {NULL, NULL};
        c = setup(r, 0, "Hi", 2, 0x12, 24, 3);
        if (k == 0) c.ip = 0x0106;
        if (k == 1) c.sp = 0x013F;
        if (k == 2) c.sp = 0xFFF1;
        before = c; memcpy(saved, r, RAM);
        CHECK(pr_r0802_0105(k == 3 ? NULL : r, k == 4 ? 1 : RAM, k == 5 ? NULL : &c, k == 6 ? &bad : &env, NULL) == PR_R0802_0105_ARGUMENT);
        if (k != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, RAM) == 0 && ncalls == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
