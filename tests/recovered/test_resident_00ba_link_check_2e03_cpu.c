#include "poolrad/recovered/resident_00ba_link_check_2e03_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic state only: the model reads no code bytes. A character at 3000:0100 whose link (+CC/+CE) is 4000:0200; the link's byte +2E is the
 * index; the table byte is DS:[558E + index*16]. */
#define DS_ 0x1000u
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }

static pr_res_2e03_cpu setup(uint8_t *r, uint16_t loff, uint16_t lseg, uint8_t idx, uint8_t tval)
{
    pr_res_2e03_cpu c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_RES_2E03_RAM_SIZE);
    c.cs = 0x6000u; c.ip = 0x2E03u; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777u; c.ax = 0xABCDu; c.cx = 0x1234u; c.flags = 0x0202u;
    put16(r, SS_, S0, 0x9000u); put16(r, SS_, (uint16_t)(S0 + 2u), 0x8000u); put16(r, SS_, (uint16_t)(S0 + 4u), 0x0100u); put16(r, SS_, (uint16_t)(S0 + 6u), 0x3000u);
    put16(r, 0x3000u, 0x01CCu, loff); put16(r, 0x3000u, 0x01CEu, lseg);
    r[at(lseg, (uint16_t)(loff + 0x2Eu))] = idx; r[at(DS_, (uint16_t)(0x558Eu + ((unsigned)idx << 4u)))] = tval;
    return c;
}

int main(void)
{
    uint8_t *r = malloc(PR_RES_2E03_RAM_SIZE), *before = malloc(PR_RES_2E03_RAM_SIZE); pr_res_2e03_cpu c, saved;
    assert(r && before);
    /* no link: result 0, the flags are OR's (ZF set, CF/OF/AF clear), SP+8, BP and CS:IP back */
    c = setup(r, 0, 0, 0, 0);
    assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_RETURNED);
    assert((uint8_t)c.ax == 0u && c.sp == S0 + 8u && c.bp == 0x7777u && c.cs == 0x8000u && c.ip == 0x9000u);
    assert((c.flags & 0x08D5u) == 0x0044u && r[at(SS_, (uint16_t)(S0 - 3u))] == 0u);
    /* a link and a table byte of 2 (above 1): result 1; index 5 -> DI = 50h; CL = 4; the flags are CMP 2,1: result 1 (odd parity), so none of 08D5h */
    c = setup(r, 0x0200u, 0x4000u, 5, 2);
    assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_RETURNED);
    assert((uint8_t)c.ax == 1u && (c.ax >> 8) == 0u && c.di == 0x0050u && (uint8_t)c.cx == 4u && (c.cx >> 8) == 0x12u && c.es == 0x4000u);
    assert((c.flags & 0x08D5u) == 0x0000u);
    /* table bytes 1 and 0 give 0 (JBE: unsigned), 80h gives 1 */
    c = setup(r, 0x0200u, 0x4000u, 5, 1); assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_RETURNED); assert((uint8_t)c.ax == 0u);
    c = setup(r, 0x0200u, 0x4000u, 5, 0); assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_RETURNED); assert((uint8_t)c.ax == 0u);
    c = setup(r, 0x0200u, 0x4000u, 5, 0x80u); assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_RETURNED); assert((uint8_t)c.ax == 1u);
    /* only the segment word of the link is non-zero: the OR is non-zero, so the link 0001:0000 is followed */
    c = setup(r, 0x0000u, 0x0001u, 0xFFu, 7); assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_RETURNED);
    assert((uint8_t)c.ax == 1u && c.di == 0x0FF0u);
    /* LES with the link at offset FFFF (pinned DOSBox-X, measured natively): the offset word straddles physically, the segment word is read at
     * offset 0001 (wrapped), not at the next paragraph; a decoy segment at physical +2 must not be taken */
    c = setup(r, 0x0200u, 0x4000u, 5, 2); put16(r, SS_, (uint16_t)(S0 + 4u), 0xFF33u);       /* +CC = FFFF */
    put16(r, 0x3000u, 0xFFFFu, 0x0200u); put16(r, 0x3000u, 0x0001u, 0x4000u);               /* offset FFFF/10000h contiguous; segment at 0001 */
    r[at(0x3000u, 0xFFFFu) + 2u] = 0x00u; r[at(0x3000u, 0xFFFFu) + 3u] = 0x50u;              /* decoy 5000 at physical +2 */
    assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_RETURNED); assert(c.es == 0x4000u && (uint8_t)c.ax == 1u);
    /* argument rejection is atomic */
    c = setup(r, 0x0200u, 0x4000u, 5, 2); memcpy(before, r, PR_RES_2E03_RAM_SIZE);
    c.ip = 0x2E04u; saved = c; assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_ARGUMENT);
    c.ip = 0x2E03u; c.sp = 0x0003u; saved = c; assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_ARGUMENT);
    c.sp = 0xFFF9u; saved = c; assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_RES_2E03_RAM_SIZE)); c.sp = S0;
    assert(pr_res_00ba_link_check_2e03_cpu(NULL, PR_RES_2E03_RAM_SIZE, &c) == PR_RES_2E03_ARGUMENT);
    assert(pr_res_00ba_link_check_2e03_cpu(r, 0, &c) == PR_RES_2E03_ARGUMENT);
    assert(pr_res_00ba_link_check_2e03_cpu(r, PR_RES_2E03_RAM_SIZE, NULL) == PR_RES_2E03_ARGUMENT);
    free(before); free(r); puts("RES 00BA 2E03 link, table, wrap and guard cases passed"); return 0;
}
