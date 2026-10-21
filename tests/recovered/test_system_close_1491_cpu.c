#include "poolrad/recovered/system_close_1491_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define RAM PR_SYSTEM_CLOSE_RAM_SIZE
#define CS 0x5800u
#define DS 0x5000u
#define SS 0x2800u
#define DATA 0x3000u
#define SP 0x8000u
static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }

static int mode, interrupts;     /* 0 success; 1 DOS error 6 (CF); 2 refuse; 3 wrong IP; 4 changed BX; 5 changed ZF; 6 changed ES */
static pr_system_close_cpu seen;
static int service(void *ctx, const pr_system_close_cpu *before, pr_system_close_cpu *after)
{
    (void)ctx; ++interrupts; seen = *before;
    if (mode == 2) return 1;
    *after = *before; after->ip = mode == 3 ? 0x14A7u : 0x14A8u;
    if (mode == 0) { after->ax = 0x3E11; after->flags &= (uint16_t)~1u; }
    else if (mode == 1) { after->ax = 0x0006; after->flags |= 1u; }
    else { after->ax = 0; }
    if (mode == 4) after->bx ^= 1u;
    if (mode == 5) after->flags ^= 0x40u;
    if (mode == 6) after->es ^= 1u;
    return 0;
}
/* The file variable lives at DATA:0100 as [handle][magic]; InOutRes is DS:4426. */
static pr_system_close_cpu setup(uint8_t *r, unsigned handle, unsigned magic)
{
    pr_system_close_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, 0x1491, DS, 0x8888, SS, 0x0202};
    memset(r, 0xA5, RAM);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001); wr(r, SS, SP + 4, 0x0100); wr(r, SS, SP + 6, DATA);
    wr(r, DATA, 0x0100, handle); wr(r, DATA, 0x0102, magic);
    wr(r, DS, 0x4426, 0); mode = 0; interrupts = 0;
    return c;
}
static pr_system_close_status run(uint8_t *r, pr_system_close_cpu *c) { return pr_system_close_1491(r, RAM, c, service, NULL); }

int main(void)
{
    uint8_t *r = malloc(RAM), *saved = malloc(RAM);
    pr_system_close_cpu c, before;
    unsigned k, h;
    CHECK(r && saved);

    /* Open file, handle > 4, DOS succeeds: one INT 21h AH=3Eh with BX = the handle; magic := D7B0; InOutRes untouched. */
    c = setup(r, 5, 0xD7B3);
    CHECK(run(r, &c) == PR_SYSTEM_CLOSE_RETURNED);
    CHECK(interrupts == 1 && seen.ip == 0x14A6 && (seen.ax >> 8) == 0x3E && seen.bx == 5);
    CHECK(rd(r, DATA, 0x0102) == 0xD7B0 && rd(r, DATA, 0x0100) == 5 && rd(r, DS, 0x4426) == 0);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP + 8 && c.bx == 5 && c.ax == 0x3E11 && c.di == 0x0100 && c.es == DATA);
    CHECK(rd(r, SS, SP - 2) == 0x149A);                                       /* the helper's return slot is left on the stack */

    /* THE GOTCHA: DOS fails (CF, AX = 6): InOutRes := AX AND the magic is STILL D7B0. */
    c = setup(r, 7, 0xD7B3); mode = 1;
    CHECK(run(r, &c) == PR_SYSTEM_CLOSE_RETURNED);
    CHECK(rd(r, DS, 0x4426) == 6 && rd(r, DATA, 0x0102) == 0xD7B0 && interrupts == 1);

    /* Standard handles 0..4 are never closed (no INT), but the magic is still marked closed; 5 and above call DOS; the compare is unsigned. */
    for (h = 0; h < 8u; ++h) {
        c = setup(r, h, 0xD7B3);
        CHECK(run(r, &c) == PR_SYSTEM_CLOSE_RETURNED);
        CHECK(interrupts == (h > 4 ? 1 : 0) && rd(r, DATA, 0x0102) == 0xD7B0 && rd(r, DS, 0x4426) == 0);
        CHECK(c.bx == h);
    }
    for (k = 0; k < 3; ++k) {
        static const unsigned big[3] = {0x8000, 0xFFFF, 0x00FF};
        c = setup(r, big[k], 0xD7B3); CHECK(run(r, &c) == PR_SYSTEM_CLOSE_RETURNED); CHECK(interrupts == 1 && seen.bx == big[k]);
    }
    c = setup(r, 3, 0xD7B3); c.flags = 0x0ED7; CHECK(run(r, &c) == PR_SYSTEM_CLOSE_RETURNED);     /* CMP BX,4 with BX = 3: CF, AF, SF, PF image */
    CHECK((c.flags & 0x08D5) == 0x0095);

    /* Magic != D7B3 (closed, input, output, garbage): InOutRes := 0067h, no DOS call, magic unchanged, RETF 4. */
    for (k = 0; k < 4; ++k) {
        static const unsigned magics[4] = {0xD7B0, 0xD7B1, 0xD7B2, 0x0000};
        c = setup(r, 9, magics[k]);
        CHECK(run(r, &c) == PR_SYSTEM_CLOSE_RETURNED);
        CHECK(interrupts == 0 && rd(r, DS, 0x4426) == 0x67 && rd(r, DATA, 0x0102) == magics[k] && rd(r, DATA, 0x0100) == 9);
        CHECK(c.bx == SP && c.sp == SP + 8 && (c.flags & 0x0040) == 0 && c.ip == 0x7000);
    }

    /* The service refuses or answers with a non-continuation: stop at the INT, nothing after it applied (magic and InOutRes untouched). */
    for (k = 2; k <= 6; ++k) {
        c = setup(r, 6, 0xD7B3); mode = (int)k;
        CHECK(run(r, &c) == PR_SYSTEM_CLOSE_SERVICE);
        CHECK(c.ip == 0x14A6 && (c.ax >> 8) == 0x3E && c.bx == 6 && c.sp == SP);
        CHECK(rd(r, DATA, 0x0102) == 0xD7B3 && rd(r, DS, 0x4426) == 0);
    }

    /* The helper's return slot overwritten: SS == DS and entry SP 4428h put the slot on InOutRes, which `MOV [4426],67h` rewrites. */
    c = setup(r, 5, 0x1234); c.ss = DS; c.sp = 0x4428; wr(r, DS, 0x4428, 0x7000); wr(r, DS, 0x442A, 0x9001); wr(r, DS, 0x442C, 0x0100); wr(r, DS, 0x442E, DATA);
    CHECK(run(r, &c) == PR_SYSTEM_CLOSE_TRANSFER);
    CHECK(c.ip == 0x0067 && c.sp == 0x4428 && interrupts == 0);

    /* Aliasing: the variable's magic word IS InOutRes (ES:DI = DS:4424). Handle 5 closes; the magic store is the InOutRes store. */
    c = setup(r, 0, 0); wr(r, SS, SP + 4, 0x4424); wr(r, SS, SP + 6, DS); wr(r, DS, 0x4424, 5); wr(r, DS, 0x4426, 0xD7B3);
    CHECK(run(r, &c) == PR_SYSTEM_CLOSE_RETURNED);
    CHECK(rd(r, DS, 0x4426) == 0xD7B0 && interrupts == 1);
    c = setup(r, 0, 0); mode = 1; wr(r, SS, SP + 4, 0x4424); wr(r, SS, SP + 6, DS); wr(r, DS, 0x4424, 5); wr(r, DS, 0x4426, 0xD7B3);
    CHECK(run(r, &c) == PR_SYSTEM_CLOSE_RETURNED);                           /* InOutRes := 6, then the magic store overwrites the same word */
    CHECK(rd(r, DS, 0x4426) == 0xD7B0);

    /* ARGUMENT: wrong IP, SP outside [0002,FFF8], bad host objects; nothing changes. */
    for (k = 0; k < 7; ++k) {
        c = setup(r, 5, 0xD7B3);
        if (k == 0) c.ip = 0x1492;
        if (k == 1) c.sp = 0x0001;
        if (k == 2) c.sp = 0xFFF9;
        before = c; memcpy(saved, r, RAM);
        CHECK(pr_system_close_1491(k == 3 ? NULL : r, k == 4 ? 1 : RAM, k == 5 ? NULL : &c, k == 6 ? NULL : service, NULL) == PR_SYSTEM_CLOSE_ARGUMENT);
        if (k != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, RAM) == 0 && interrupts == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
