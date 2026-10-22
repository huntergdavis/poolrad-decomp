#include "poolrad/recovered/overlay_u7f_entry0_0000_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W_CS 0x6000u
#define SS_ 0x2000u
#define RECSEG 0x3000u
#define RECOFF 0x0100u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }

/* An entry frame whose record has the given +10h kind byte; the far call's bytes and relocated segment word are as the loader leaves them. */
static pr_res00ba_cpu setup(uint8_t *r, uint8_t kind, uint16_t resident_cs)
{
    pr_res00ba_cpu c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_U9E_10E4_RAM_SIZE);
    c.cs = W_CS; c.ip = 0; c.ds = 0x1000; c.ss = SS_; c.sp = 0x8000; c.bp = 0x7777; c.ax = 0xABCD; c.flags = 0x0202;
    put16(r, c.ss, c.sp, 0x9000); put16(r, c.ss, (uint16_t)(c.sp + 2u), 0x8000); put16(r, c.ss, (uint16_t)(c.sp + 4u), RECOFF); put16(r, c.ss, (uint16_t)(c.sp + 6u), RECSEG);
    r[at(RECSEG, RECOFF + 0x10u)] = kind; r[at(RECSEG, RECOFF + 0x16u)] = 0;
    r[at(W_CS, 0x0Cu)] = 0x9A; put16(r, W_CS, 0x0Du, 0x13ABu); put16(r, W_CS, 0x0Fu, resident_cs);
    return c;
}
static void returned(const pr_res00ba_cpu *c, uint16_t ax, uint16_t arith)
{
    assert(c->cs == 0x8000 && c->ip == 0x9000 && c->sp == 0x8008 && c->bp == 0x7777);
    assert(c->ax == ax); assert((c->flags & 0x08D5u) == arith); assert((c->flags & ~0x08D5u) == (0x0202u & ~0x08D5u));
}
int main(void)
{
    uint8_t *r = malloc(PR_U9E_10E4_RAM_SIZE), *before = malloc(PR_U9E_10E4_RAM_SIZE);
    pr_res00ba_cpu c, saved; size_t budget; pr_u7f_0000_diagnostic d; unsigned resident;
    assert(r && before);
    /* kind 0Ch maps to 0064h; plus 05DCh = 0640h: no carry, parity odd, AF set */
    c = setup(r, 0x0C, 0x7000u); budget = 10000;
    assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, &d) == PR_U7F_0000_RETURNED); returned(&c, 0x0640u, 0x0010u);
    assert(d.callee_status == -1);
    /* kind 01h maps to FEA2h; plus 05DCh wraps to 047Eh with carry and even parity */
    c = setup(r, 0x01, 0x7000u); budget = 10000;
    assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0000_RETURNED); returned(&c, 0x047Eu, 0x0005u);
    /* the segment word is read live: the same result with the word relocated elsewhere, and the return frame the resident saw names this body's CS */
    for (resident = 0x7000u; resident < 0xF000u; resident += 0x1357u) {
        c = setup(r, 0x10, (uint16_t)resident); budget = 10000;
        assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0000_RETURNED);
        assert(c.ax == (uint16_t)(0x015Eu + 0x05DCu));
    }
    /* budget: 0 leaves everything; 5 stops at the far call before its return frame exists; the CODE stop is also before the frame */
    c = setup(r, 0x0C, 0x7000u); saved = c; memcpy(before, r, PR_U9E_10E4_RAM_SIZE); budget = 0;
    assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0000_LIMIT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_U9E_10E4_RAM_SIZE));
    c = setup(r, 0x0C, 0x7000u); budget = 5;
    assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0000_LIMIT); assert(c.cs == W_CS && c.ip == 0x000Cu && c.sp == 0x7FF8u && budget == 0);
    c = setup(r, 0x0C, 0x7000u); r[at(W_CS, 0x0Cu)] = 0xE8; budget = 10000;
    assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, &d) == PR_U7F_0000_CODE); assert(c.ip == 0x000Cu && c.sp == 0x7FF8u && d.site == 0x000Cu && budget == 10000u - 5u);
    c = setup(r, 0x0C, 0x7000u); put16(r, W_CS, 0x0Du, 0x13ACu); budget = 10000;
    assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0000_CODE);
    /* a budget that ends inside the resident closure reports LIMIT with the resident's CS:IP preserved */
    c = setup(r, 0x0C, 0x7345u); budget = 8;
    assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, &d) == PR_U7F_0000_LIMIT); assert(c.cs == 0x7345u && budget == 0);
    /* argument rejection is atomic */
    c = setup(r, 0x0C, 0x7000u); saved = c; memcpy(before, r, PR_U9E_10E4_RAM_SIZE); budget = 10000;
    c.ip = 1; saved = c; assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0000_ARGUMENT);
    c.ip = 0; c.sp = 0x003Fu; saved = c; assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0000_ARGUMENT);
    c.sp = 0xFFF1u; saved = c; assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0000_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_U9E_10E4_RAM_SIZE) && budget == 10000);
    c.sp = 0x8000;
    assert(pr_u7f_entry0_0000_cpu(NULL, PR_U9E_10E4_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0000_ARGUMENT);
    assert(pr_u7f_entry0_0000_cpu(r, 0, &c, &budget, NULL) == PR_U7F_0000_ARGUMENT);
    assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, NULL, &budget, NULL) == PR_U7F_0000_ARGUMENT);
    assert(pr_u7f_entry0_0000_cpu(r, PR_U9E_10E4_RAM_SIZE, &c, NULL, NULL) == PR_U7F_0000_ARGUMENT);
    free(before); free(r); puts("U7F0000 synthetic composition/guard cases passed"); return 0;
}
