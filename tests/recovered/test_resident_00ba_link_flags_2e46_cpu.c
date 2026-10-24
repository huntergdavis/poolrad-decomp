#include "poolrad/recovered/resident_00ba_link_flags_2e46_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic state only: the models read no code bytes. A character at 3000:0100 whose link (+CC/+CE) is 4000:0200; the link's byte +2E is the
 * index; 2E03's table byte is DS:[558E + index*16], this body's flags byte DS:[5590 + index*16]. */
#define DS_ 0x1000u
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }

static pr_res_2e46_cpu setup(uint8_t *r, uint16_t loff, uint16_t lseg, uint8_t idx, uint8_t tval, uint8_t fval)
{
    pr_res_2e46_cpu c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_RES_2E03_RAM_SIZE);
    c.cs = 0x6000u; c.ip = 0x2E46u; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777u; c.ax = 0xABCDu; c.cx = 0x1234u; c.flags = 0x0202u;
    put16(r, SS_, S0, 0x9000u); put16(r, SS_, (uint16_t)(S0 + 2u), 0x8000u); put16(r, SS_, (uint16_t)(S0 + 4u), 0x0100u); put16(r, SS_, (uint16_t)(S0 + 6u), 0x3000u);
    put16(r, 0x3000u, 0x01CCu, loff); put16(r, 0x3000u, 0x01CEu, lseg);
    r[at(lseg, (uint16_t)(loff + 0x2Eu))] = idx;
    r[at(DS_, (uint16_t)(0x558Eu + ((unsigned)idx << 4u)))] = tval; r[at(DS_, (uint16_t)(0x5590u + ((unsigned)idx << 4u)))] = fval;
    return c;
}
static int run(pr_res_2e46_cpu *c, uint8_t *r) { return (int)pr_res_00ba_link_flags_2e46_cpu(r, PR_RES_2E03_RAM_SIZE, c); }

int main(void)
{
    uint8_t *r = malloc(PR_RES_2E03_RAM_SIZE), *before = malloc(PR_RES_2E03_RAM_SIZE); pr_res_2e46_cpu c, saved;
    assert(r && before);
    /* no link: 2E03 returns 0, so the result is 0; SP+8, BP and CS:IP back; the local at [BP-1] is 0 */
    c = setup(r, 0, 0, 0, 0, 0x14u); assert(run(&c, r) == PR_RES_2E46_RETURNED);
    assert((uint8_t)c.ax == 0u && c.sp == S0 + 8u && c.bp == 0x7777u && c.cs == 0x8000u && c.ip == 0x9000u && r[at(SS_, (uint16_t)(S0 - 3u))] == 0u);
    /* 2E03 returns 1 (table byte 2) and the flags byte has both bits 14h: result 1; CMP 14h,14h leaves ZF and PF set */
    c = setup(r, 0x0200u, 0x4000u, 5, 2, 0x14u); assert(run(&c, r) == PR_RES_2E46_RETURNED);
    assert((uint8_t)c.ax == 1u && c.di == 0x0050u && (uint8_t)c.cx == 4u && (c.flags & 0x08D5u) == 0x0044u);
    /* only one of the two bits, or neither: result 0; extra bits do not matter */
    c = setup(r, 0x0200u, 0x4000u, 5, 2, 0x04u); assert(run(&c, r) == PR_RES_2E46_RETURNED && (uint8_t)c.ax == 0u);
    c = setup(r, 0x0200u, 0x4000u, 5, 2, 0x10u); assert(run(&c, r) == PR_RES_2E46_RETURNED && (uint8_t)c.ax == 0u);
    c = setup(r, 0x0200u, 0x4000u, 5, 2, 0xFFu); assert(run(&c, r) == PR_RES_2E46_RETURNED && (uint8_t)c.ax == 1u);
    /* 2E03 returns 0 through its table (byte 1): the flags byte is never consulted, result 0 */
    c = setup(r, 0x0200u, 0x4000u, 5, 1, 0x14u); assert(run(&c, r) == PR_RES_2E46_RETURNED && (uint8_t)c.ax == 0u);
    /* index FFh: both tables at DI = 0FF0h */
    c = setup(r, 0x0200u, 0x4000u, 0xFFu, 7, 0x34u); assert(run(&c, r) == PR_RES_2E46_RETURNED && (uint8_t)c.ax == 1u && c.di == 0x0FF0u);
    /* the entry frame's return address is honoured as given (a TRANSFER needs the body's own writes to change it; the differential's alias cases) */
    c = setup(r, 0, 0, 0, 0, 0); put16(r, SS_, S0, 0x1234u);
    assert(run(&c, r) == PR_RES_2E46_RETURNED && c.ip == 0x1234u && c.cs == 0x8000u);
    /* argument rejection is atomic */
    c = setup(r, 0x0200u, 0x4000u, 5, 2, 0x14u); memcpy(before, r, PR_RES_2E03_RAM_SIZE);
    c.ip = 0x2E47u; saved = c; assert(run(&c, r) == PR_RES_2E46_ARGUMENT);
    c.ip = 0x2E46u; c.sp = 0x000Fu; saved = c; assert(run(&c, r) == PR_RES_2E46_ARGUMENT);
    c.sp = 0xFFF9u; saved = c; assert(run(&c, r) == PR_RES_2E46_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_RES_2E03_RAM_SIZE)); c.sp = S0;
    assert(pr_res_00ba_link_flags_2e46_cpu(NULL, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E46_ARGUMENT);
    assert(pr_res_00ba_link_flags_2e46_cpu(r, 0, &c) == PR_RES_2E46_ARGUMENT);
    assert(pr_res_00ba_link_flags_2e46_cpu(r, PR_RES_2E03_RAM_SIZE, NULL) == PR_RES_2E46_ARGUMENT);
    free(before); free(r); puts("RES_00BA_2E46 synthetic and guard cases passed"); return 0;
}
