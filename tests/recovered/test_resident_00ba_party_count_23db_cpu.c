#include "poolrad/recovered/resident_00ba_party_count_23db_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic state only: the model reads no code bytes. The party head DS:[5D96] links characters through +104h/+106h; a non-zero byte +10Dh counts
 * the character in DS:[6814h + CBW(byte +10Eh)]. */
#define DS_ 0x1000u
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }
static void chr(uint8_t *r, uint16_t seg, uint8_t flag, uint8_t idx, uint16_t nseg)
{ put16(r, seg, 0x0204u, nseg ? 0x0100u : 0u); put16(r, seg, 0x0206u, nseg); r[at(seg, 0x020Du)] = flag; r[at(seg, 0x020Eu)] = idx; }

static pr_res_23db_cpu setup(uint8_t *r, uint16_t head)
{
    pr_res_23db_cpu c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_RES_2E03_RAM_SIZE);
    c.cs = 0x6000u; c.ip = 0x23DBu; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777u; c.ax = 0xABCDu; c.flags = 0x0202u;
    put16(r, SS_, S0, 0x9000u); put16(r, SS_, (uint16_t)(S0 + 2u), 0x8000u);
    put16(r, DS_, 0x5D96u, head ? 0x0100u : 0u); put16(r, DS_, 0x5D98u, head);
    return c;
}
static int run(uint8_t *r, pr_res_23db_cpu *c, size_t b) { size_t left = b; int s = (int)pr_res_00ba_party_count_23db_cpu(r, PR_RES_2E03_RAM_SIZE, c, &left); return s; }

int main(void)
{
    uint8_t *r = malloc(PR_RES_2E03_RAM_SIZE), *before = malloc(PR_RES_2E03_RAM_SIZE); pr_res_23db_cpu c, saved; size_t left;
    assert(r && before);
    /* an empty party: both counters are zeroed (from A5h), SP+4, BP and CS:IP back */
    c = setup(r, 0); assert(run(r, &c, 100000) == PR_RES_23DB_RETURNED);
    assert(r[at(DS_, 0x6814u)] == 0u && r[at(DS_, 0x6815u)] == 0u && c.sp == S0 + 4u && c.bp == 0x7777u && c.cs == 0x8000u && c.ip == 0x9000u);
    /* three characters: idx 0 counted, flag 0 not counted, idx 1 counted; and a negative index FFh counts DS:[6813] */
    c = setup(r, 0x3000u); chr(r, 0x3000u, 1, 0, 0x3040u); chr(r, 0x3040u, 0, 1, 0x3080u); chr(r, 0x3080u, 5, 1, 0x30C0u); chr(r, 0x30C0u, 1, 0xFFu, 0);
    r[at(DS_, 0x6813u)] = 0x10u; assert(run(r, &c, 100000) == PR_RES_23DB_RETURNED);
    assert(r[at(DS_, 0x6814u)] == 1u && r[at(DS_, 0x6815u)] == 1u && r[at(DS_, 0x6813u)] == 0x11u);
    /* the INC flags: counting 7Fh -> 80h sets OF and SF; the last flag writer before RETF is the final OR (CF=OF=AF=0, ZF=1) */
    c = setup(r, 0x3000u); chr(r, 0x3000u, 1, 0, 0); assert(run(r, &c, 100000) == PR_RES_23DB_RETURNED);
    assert((c.flags & 0x0841u) == 0x0040u);
    /* a cyclic party never ends: the budget stops it at the next unexecuted instruction with the budget spent */
    c = setup(r, 0x3000u); chr(r, 0x3000u, 1, 0, 0x3000u); left = 500;
    assert(pr_res_00ba_party_count_23db_cpu(r, PR_RES_2E03_RAM_SIZE, &c, &left) == PR_RES_23DB_LIMIT && left == 0u && c.ip >= 0x23F8u && c.ip < 0x242Fu);
    /* budget 0 changes nothing */
    c = setup(r, 0); saved = c; memcpy(before, r, PR_RES_2E03_RAM_SIZE); left = 0;
    assert(pr_res_00ba_party_count_23db_cpu(r, PR_RES_2E03_RAM_SIZE, &c, &left) == PR_RES_23DB_LIMIT && c.ip == 0x23DBu && c.sp == S0 && !memcmp(r, before, PR_RES_2E03_RAM_SIZE));
    /* argument rejection is atomic */
    c = setup(r, 0); memcpy(before, r, PR_RES_2E03_RAM_SIZE); left = 100;
    c.ip = 0x23DCu; saved = c; assert(pr_res_00ba_party_count_23db_cpu(r, PR_RES_2E03_RAM_SIZE, &c, &left) == PR_RES_23DB_ARGUMENT);
    c.ip = 0x23DBu; c.sp = 0x0005u; saved = c; assert(pr_res_00ba_party_count_23db_cpu(r, PR_RES_2E03_RAM_SIZE, &c, &left) == PR_RES_23DB_ARGUMENT);
    c.sp = 0xFFFDu; saved = c; assert(pr_res_00ba_party_count_23db_cpu(r, PR_RES_2E03_RAM_SIZE, &c, &left) == PR_RES_23DB_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_RES_2E03_RAM_SIZE) && left == 100u); c.sp = S0;
    assert(pr_res_00ba_party_count_23db_cpu(NULL, PR_RES_2E03_RAM_SIZE, &c, &left) == PR_RES_23DB_ARGUMENT);
    assert(pr_res_00ba_party_count_23db_cpu(r, 0, &c, &left) == PR_RES_23DB_ARGUMENT);
    assert(pr_res_00ba_party_count_23db_cpu(r, PR_RES_2E03_RAM_SIZE, NULL, &left) == PR_RES_23DB_ARGUMENT);
    assert(pr_res_00ba_party_count_23db_cpu(r, PR_RES_2E03_RAM_SIZE, &c, NULL) == PR_RES_23DB_ARGUMENT);
    (void)saved; free(before); free(r); puts("RES_00BA_23DB synthetic, budget and guard cases passed"); return 0;
}
