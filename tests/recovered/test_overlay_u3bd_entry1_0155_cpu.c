#include "poolrad/recovered/overlay_u3bd_entry1_0155_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CS_ 0x6000u
#define DS_ 0x1000u
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }

/* Entry frame: return IP 9000h, CS 8000h, then the four argument slots pushed first..fourth = a, b, c, d (so d is nearest). */
static pr_u3bd_0155_cpu setup(uint8_t *r, uint16_t ds, uint16_t sp, uint8_t a, uint8_t b, uint8_t cc, uint8_t d)
{
    pr_u3bd_0155_cpu c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_U3BD_0155_RAM_SIZE);
    c.cs = CS_; c.ip = 0x0155u; c.ds = ds; c.ss = SS_; c.sp = sp; c.bp = 0x7777u; c.ax = 0xABCDu; c.flags = 0x08D7u;
    put16(r, SS_, sp, 0x9000u); put16(r, SS_, (uint16_t)(sp + 2u), 0x8000u);
    put16(r, SS_, (uint16_t)(sp + 4u), (uint16_t)(0xCD00u | d)); put16(r, SS_, (uint16_t)(sp + 6u), (uint16_t)(0xCD00u | cc));
    put16(r, SS_, (uint16_t)(sp + 8u), (uint16_t)(0xCD00u | b)); put16(r, SS_, (uint16_t)(sp + 10u), (uint16_t)(0xCD00u | a));
    return c;
}

int main(void)
{
    uint8_t *r = malloc(PR_U3BD_0155_RAM_SIZE), *before = malloc(PR_U3BD_0155_RAM_SIZE);
    pr_u3bd_0155_cpu c, saved; size_t budget;
    assert(r && before);
    /* the program body's own call: pushes 0Bh, 00h, 07h, 06h; DS:6A60..6A63 receive them in push order, AL is the last, flags untouched */
    c = setup(r, DS_, S0, 0x0Bu, 0x00u, 0x07u, 0x06u); budget = 100;
    assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_RETURNED);
    assert(r[at(DS_, 0x6A60u)] == 0x0Bu && r[at(DS_, 0x6A61u)] == 0x00u && r[at(DS_, 0x6A62u)] == 0x07u && r[at(DS_, 0x6A63u)] == 0x06u);
    assert(c.ax == 0xAB06u && c.flags == 0x08D7u && c.bp == 0x7777u && c.sp == S0 + 12u && c.cs == 0x8000u && c.ip == 0x9000u && budget == 88u);
    /* the high byte of each slot is ignored */
    assert(r[at(DS_, 0x6A64u)] == 0xA5u);
    /* DS = SS with 6A60 on the first argument slot's low byte (BP+0C = SP+0A): the first store rewrites it with itself, the rest go above */
    c = setup(r, SS_, (uint16_t)(0x6A60u - 0x0Au), 0x11u, 0x22u, 0x33u, 0x44u); budget = 100;
    assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_RETURNED);
    assert(r[at(SS_, 0x6A60u)] == 0x11u && r[at(SS_, 0x6A61u)] == 0x22u && r[at(SS_, 0x6A62u)] == 0x33u && r[at(SS_, 0x6A63u)] == 0x44u);
    /* DS = SS with 6A60 on the second slot (BP+0A = SP+8): store 1 overwrites the byte the second load reads, so the reread is seen */
    c = setup(r, SS_, (uint16_t)(0x6A60u - 0x08u), 0x11u, 0x22u, 0x33u, 0x44u); budget = 100;
    assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_RETURNED);
    assert(r[at(SS_, 0x6A60u)] == 0x11u && r[at(SS_, 0x6A61u)] == 0x11u);   /* the second load rereads 6A60, which store 1 just set to 11h */
    /* DS = SS with 6A60 on the return IP (SP): the stores replace the return address: TRANSFER at the popped CS:IP */
    c = setup(r, SS_, 0x6A60u, 0x11u, 0x22u, 0x33u, 0x44u); budget = 100;
    assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_TRANSFER);
    assert(c.ip == 0x2211u && c.cs == 0x4433u && c.sp == 0x6A60u + 12u);
    /* budget: 0 changes nothing; 3 stops before the first store */
    c = setup(r, DS_, S0, 1, 2, 3, 4); saved = c; memcpy(before, r, PR_U3BD_0155_RAM_SIZE); budget = 0;
    assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_LIMIT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_U3BD_0155_RAM_SIZE));
    c = setup(r, DS_, S0, 1, 2, 3, 4); budget = 3;
    assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_LIMIT);
    assert(c.ip == 0x015Bu && budget == 0u && (uint8_t)c.ax == 1u && r[at(DS_, 0x6A60u)] == 0xA5u);
    /* argument rejection is atomic */
    c = setup(r, DS_, S0, 1, 2, 3, 4); memcpy(before, r, PR_U3BD_0155_RAM_SIZE); budget = 100;
    c.ip = 0x0156u; saved = c; assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_ARGUMENT);
    c.ip = 0x0155u; c.sp = 0x0001u; saved = c; assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_ARGUMENT);
    c.sp = 0xFFF5u; saved = c; assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_U3BD_0155_RAM_SIZE) && budget == 100u); c.sp = S0;
    assert(pr_u3bd_entry1_0155_cpu(NULL, PR_U3BD_0155_RAM_SIZE, &c, &budget) == PR_U3BD_0155_ARGUMENT);
    assert(pr_u3bd_entry1_0155_cpu(r, 0, &c, &budget) == PR_U3BD_0155_ARGUMENT);
    assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, NULL, &budget) == PR_U3BD_0155_ARGUMENT);
    assert(pr_u3bd_entry1_0155_cpu(r, PR_U3BD_0155_RAM_SIZE, &c, NULL) == PR_U3BD_0155_ARGUMENT);
    free(before); free(r); puts("U3BD 0155 synthetic, alias and guard cases passed"); return 0;
}
