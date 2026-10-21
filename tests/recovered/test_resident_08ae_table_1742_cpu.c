#include "poolrad/recovered/resident_08ae_table_1742_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define CS 0x5800u
#define DATA 0x5000u
#define SS 0x2800u
#define SP 0x8000u

static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void wr(uint8_t *r, unsigned s, unsigned o, unsigned v) { unsigned p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFF] = (uint8_t)(v >> 8); }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o) { unsigned p = at(s, o); return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8); }
static unsigned rb(const uint8_t *r, unsigned s, unsigned o) { return r[at(s, o)]; }

/* Entry frame: return IP 7000 / CS 9001 and the word argument; the relocated DS immediate at CS:174B names DATA.
 * Records 022D..02EC are pre-filled with 7777h so a clear is visible. */
static pr_r08ae_1742_cpu setup(uint8_t *r, unsigned arg)
{
    pr_r08ae_1742_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP, CS, 0x1742, 0x1234, 0x8888, SS, 0x0202};
    unsigned o;
    memset(r, 0xA5, PR_R08AE_1742_RAM_SIZE);
    wr(r, SS, SP, 0x7000); wr(r, SS, SP + 2, 0x9001); wr(r, SS, SP + 4, arg);
    wr(r, CS, 0x174B, DATA);
    for (o = 0x022D; o < 0x02EDu; o += 2) wr(r, DATA, o, 0x7777);
    return c;
}
static void table(uint8_t *r, unsigned arg, unsigned w0, unsigned w1, unsigned w2, unsigned w3)
{
    unsigned base = (0x055Fu + 8u * arg) & 0xFFFFu;
    wr(r, DATA, base, w0); wr(r, DATA, base + 2, w1); wr(r, DATA, base + 4, w2); wr(r, DATA, base + 6, w3);
}
static void returned(const pr_r08ae_1742_cpu *c)
{
    CHECK(c->cs == 0x9001 && c->ip == 0x7000 && c->sp == SP + 6);                          /* RETF 2 */
    CHECK(c->ax == 0x1111 && c->bx == 0x2222 && c->di == 0x6666 && c->si == 0x5555 && c->bp == 0x789A && c->ds == 0x1234);
    CHECK(c->cx == 0x3333 && c->dx == 0x4444 && c->es == 0x8888 && c->ss == SS);           /* untouched or restored */
}
static void record_hit(const uint8_t *r, unsigned row, unsigned source_word)
{
    unsigned base = 0x022Du + 0x30u * row, k;
    CHECK(rd(r, DATA, base) == 1 && rd(r, DATA, base + 2) == source_word);
    for (k = 4; k <= 0x2E; k += 2) CHECK(rd(r, DATA, base + k) == 0);                    /* the other 22 words are cleared */
}
static void record_miss(const uint8_t *r, unsigned row)
{
    unsigned base = 0x022Du + 0x30u * row, k;
    for (k = 0; k <= 0x2E; k += 2) CHECK(rd(r, DATA, base + k) == 0x7777);
}

int main(void)
{
    uint8_t *r = malloc(PR_R08AE_1742_RAM_SIZE), *saved = malloc(PR_R08AE_1742_RAM_SIZE);
    pr_r08ae_1742_cpu c, before;
    CHECK(r && saved);

    /* A=2: the four source words are at 055F + 16 + 2*row. Rows 0, 2, 3 hit, row 1 misses. */
    c = setup(r, 2); table(r, 2, 0x1111, 0, 0x3333, 0x4444);
    CHECK(pr_r08ae_table_1742_cpu(r, PR_R08AE_1742_RAM_SIZE, &c) == PR_R08AE_1742_OK);
    returned(&c);
    record_hit(r, 0, 0x1111); record_miss(r, 1); record_hit(r, 2, 0x3333); record_hit(r, 3, 0x4444);
    CHECK(rb(r, DATA, 0x0209) == 2 && rb(r, DATA, 0x020A) == 0xA5 && rb(r, DATA, 0x020B) == 2 && rb(r, DATA, 0x020C) == 2);
    CHECK(rd(r, DATA, 0x01FF) == 2 && rb(r, DATA, 0x01D8) == 0 && rb(r, DATA, 0x01D9) == 0);
    CHECK((c.flags & 0x08D5) == 0x0044);        /* the last flag writer is DEC BYTE [01D9] giving 0; CF kept from ADD SI,2 (clear) */
    CHECK(rd(r, SS, SP - 2) == 0x789A);         /* the saved registers stay in RAM below the stack */

    /* All four words zero: no record changes, no selector byte is written, the control bytes still end at 0. */
    c = setup(r, 5); table(r, 5, 0, 0, 0, 0);
    CHECK(pr_r08ae_table_1742_cpu(r, PR_R08AE_1742_RAM_SIZE, &c) == PR_R08AE_1742_OK);
    returned(&c);
    for (unsigned row = 0; row < 4; ++row) record_miss(r, row);
    CHECK(rb(r, DATA, 0x0209) == 0xA5 && rd(r, DATA, 0x01FF) == 5 && rb(r, DATA, 0x01D8) == 0);

    /* The whole WORD is stored at 01FF and selects the table (8*A); only its low byte lands in the selector bytes. */
    c = setup(r, 0x0A02); table(r, 0x0A02, 7, 7, 7, 7);
    CHECK(pr_r08ae_table_1742_cpu(r, PR_R08AE_1742_RAM_SIZE, &c) == PR_R08AE_1742_OK);
    CHECK(rd(r, DATA, 0x01FF) == 0x0A02 && rb(r, DATA, 0x0209) == 0x02 && rb(r, DATA, 0x020C) == 0x02);
    record_hit(r, 0, 7); record_hit(r, 3, 7);

    /* The effective source offset wraps at 16 bits: A=1FA0h gives 8*A = FD00h, so row 0 reads DS:025F. */
    c = setup(r, 0x1FA0); wr(r, DATA, 0x025F, 0x2468); wr(r, DATA, 0x0261, 0); wr(r, DATA, 0x0263, 0); wr(r, DATA, 0x0265, 0);
    CHECK(pr_r08ae_table_1742_cpu(r, PR_R08AE_1742_RAM_SIZE, &c) == PR_R08AE_1742_OK);
    CHECK(rd(r, DATA, 0x022F) == 0x2468 && rd(r, DATA, 0x022D) == 1);

    /* Aliasing: for A=1F99h the row-3 source word is DS:022D, the base of record 0. Row 0 hits and stores 1 there, so
     * row 3 (zero before the call) now reads a nonzero word and also hits. */
    c = setup(r, 0x1F99); wr(r, DATA, 0x0227, 0x5555); wr(r, DATA, 0x0229, 0); wr(r, DATA, 0x022B, 0); wr(r, DATA, 0x022D, 0);
    CHECK(pr_r08ae_table_1742_cpu(r, PR_R08AE_1742_RAM_SIZE, &c) == PR_R08AE_1742_OK);
    CHECK(rd(r, DATA, 0x022D) == 1 && rd(r, DATA, 0x022F) == 0x5555);
    CHECK(rd(r, DATA, 0x02BF) == 1 && rd(r, DATA, 0x02BD) == 1);

    /* ARGUMENT: wrong IP, SP outside [000C,FFFA], bad host objects; nothing changes. */
    for (unsigned k = 0; k < 6; ++k) {
        c = setup(r, 2);
        if (k == 0) c.ip = 0x1743;
        if (k == 1) c.sp = 0x000B;
        if (k == 2) c.sp = 0xFFFB;
        before = c; memcpy(saved, r, PR_R08AE_1742_RAM_SIZE);
        CHECK(pr_r08ae_table_1742_cpu(k == 3 ? NULL : r, k == 4 ? 1 : PR_R08AE_1742_RAM_SIZE, k == 5 ? NULL : &c) == PR_R08AE_1742_ARGUMENT);
        if (k != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, PR_R08AE_1742_RAM_SIZE) == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}
