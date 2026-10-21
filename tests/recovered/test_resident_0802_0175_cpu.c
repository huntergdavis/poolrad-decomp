#include "poolrad/recovered/resident_0802_0175_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define RAM PR_R0802_0175_RAM_SIZE
#define CS 0x5800u
#define DS 0x5000u
#define SS 0x2800u
#define SP 0x8000u
#define INTR_SEG 0x6000u
#define HSEG 0x7800u
static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }
static struct { unsigned ax, bx, cx, dx; } calls[8];
static unsigned ncalls;
static int refuse_at;
static int video(void *ctx, uint8_t *ram, const pr_dos_intr_cpu *before, pr_dos_intr_cpu *after)
{
    (void)ctx;
    if (ncalls < 8) { calls[ncalls].ax = before->ax; calls[ncalls].bx = before->bx; calls[ncalls].cx = before->cx; calls[ncalls].dx = before->dx; }
    ++ncalls;
    if (refuse_at && (int)ncalls == refuse_at) return 1;
    *after = *before;
    after->ip = (uint16_t)rd(ram, before->ss, before->sp); after->cs = (uint16_t)rd(ram, before->ss, before->sp + 2u);
    after->flags = (uint16_t)rd(ram, before->ss, before->sp + 4u); after->sp = (uint16_t)(before->sp + 6u);
    return 0;
}
/* Arguments: column, row, attribute, count. DS:4A1B page 3, 4A1A page 7, the attribute map 25D5 = A0h + index. */
static pr_r0802_0175_cpu setup(uint8_t *r, unsigned ega, unsigned col, unsigned row, unsigned attr, unsigned count)
{
    pr_r0802_0175_cpu c = {0x1234, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, 0x0175, DS, 0x8888, SS, 0x0202};
    unsigned k;
    memset(r, 0x5A, RAM);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001);
    wr(r, SS, SP + 4, 0xAB00 | count); wr(r, SS, SP + 6, 0xCD00 | attr); wr(r, SS, SP + 8, 0xEF00 | row); wr(r, SS, SP + 10, 0x1200 | col);
    r[at(DS, 0x4A1A)] = 7; r[at(DS, 0x4A1B)] = 3; r[at(DS, 0x4A1E)] = (uint8_t)ega;
    for (k = 0; k < 256; ++k) r[at(DS, 0x25D5 + k)] = (uint8_t)(0xA0 + k);
    wr(r, 0, 0x40, 0x0000); wr(r, 0, 0x42, HSEG);
    wr(r, CS, 0x65, INTR_SEG); wr(r, CS, 0xA8, INTR_SEG); wr(r, CS, 0xD3, INTR_SEG); wr(r, CS, 0xFF, INTR_SEG);
    wr(r, CS, 0x18D, (unsigned)(0x0040 - 0x018F) & 0xFFFFu);                                                     /* 0175's near call to 0040 */
    ncalls = 0; refuse_at = 0;
    return c;
}
static pr_r0802_0175_status run(uint8_t *r, pr_r0802_0175_cpu *c, pr_r0802_0175_diagnostic *d)
{
    pr_r0802_0040_env env = {video, NULL};
    return pr_r0802_0175(r, RAM, c, &env, d);
}
int main(void)
{
    uint8_t *r = malloc(RAM), *saved = malloc(RAM);
    pr_r0802_0175_cpu c, before;
    pr_r0802_0175_diagnostic d;
    unsigned k;
    CHECK(r && saved);

    /* The 0193 call: column 0, row 24, attribute 0, 40 blocks. One 0040 call = two INT 10h calls with DS:4A1E = 0. */
    c = setup(r, 0, 0, 24, 0, 40);
    CHECK(run(r, &c, &d) == PR_R0802_0175_RETURNED && d.callee_status == PR_R0802_0040_RETURNED && ncalls == 2);
    CHECK((calls[0].ax >> 8) == 2 && (calls[0].bx >> 8) == 3 && calls[0].dx == ((24u << 8) | 0u));
    CHECK((calls[1].ax >> 8) == 9 && (calls[1].ax & 0xFF) == 0xDB && (calls[1].bx & 0xFF) == 0xA0 && calls[1].cx == 40);       /* the full block, attribute 0 -> map[0] */
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP + 12 && c.bp == 0x789A);
    /* The arguments are forwarded in the order column, row, attribute, count; the character is the constant DBh. */
    c = setup(r, 1, 7, 3, 0x12, 5);
    CHECK(run(r, &c, &d) == PR_R0802_0175_RETURNED && ncalls == 4);
    CHECK(calls[0].dx == ((3u << 8) | 7u) && (calls[1].bx & 0xFF) == 0x12 && calls[1].cx == 5 && (calls[3].ax & 0xFF) == 0xDB && (calls[3].bx >> 8) == 7);
    /* The slots are bytes: the high byte of each is ignored; a count of 0 writes nothing visible but still calls. */
    c = setup(r, 2, 0xFF, 0xFF, 0xFF, 0);
    CHECK(run(r, &c, &d) == PR_R0802_0175_RETURNED && ncalls == 2 && calls[0].dx == 0xFFFF && calls[1].cx == 0 && (calls[1].bx & 0xFF) == 0xFF);
    /* A refusing handler: DISPLAY. */
    c = setup(r, 0, 0, 24, 0, 40); refuse_at = 2;
    CHECK(run(r, &c, &d) == PR_R0802_0175_DISPLAY && d.display.intr_status == PR_DOS_INTR_SERVICE && d.display.calls == 1);
    /* ARGUMENT: wrong IP, entry SP outside [0040,FFF4], bad host objects; nothing changes. */
    for (k = 0; k < 7; ++k) {
        pr_r0802_0040_env env = {video, NULL}, bad = {NULL, NULL};
        c = setup(r, 0, 0, 24, 0, 40);
        if (k == 0) c.ip = 0x0176;
        if (k == 1) c.sp = 0x003F;
        if (k == 2) c.sp = 0xFFF5;
        before = c; memcpy(saved, r, RAM);
        CHECK(pr_r0802_0175(k == 3 ? NULL : r, k == 4 ? 1 : RAM, k == 5 ? NULL : &c, k == 6 ? &bad : &env, NULL) == PR_R0802_0175_ARGUMENT);
        if (k != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, RAM) == 0 && ncalls == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
