#include "poolrad/recovered/resident_00ba_zero_test_23b7_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic state only: the model reads no code bytes. A character at 3000:0100; the tested byte is at character+10Eh. */
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }

static pr_res_23b7_cpu setup(uint8_t *r, uint8_t field)
{
    pr_res_23b7_cpu c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_RES_23B7_RAM_SIZE);
    c.cs = 0x6000u; c.ip = 0x23B7u; c.ds = 0x1000u; c.ss = SS_; c.sp = S0; c.bp = 0x7777u; c.ax = 0xABCDu; c.flags = 0x0202u;
    put16(r, SS_, S0, 0x9000u); put16(r, SS_, (uint16_t)(S0 + 2u), 0x8000u); put16(r, SS_, (uint16_t)(S0 + 4u), 0x0100u); put16(r, SS_, (uint16_t)(S0 + 6u), 0x3000u);
    r[at(0x3000u, 0x020Eu)] = field;
    return c;
}

int main(void)
{
    uint8_t *r = malloc(PR_RES_23B7_RAM_SIZE), *before = malloc(PR_RES_23B7_RAM_SIZE); pr_res_23b7_cpu c, saved;
    assert(r && before);
    /* a zero byte: result 1; flags are CMP 0,0's (ZF, PF set; CF/AF/SF/OF clear); SP+8, BP, CS:IP back; ES:DI = the character */
    c = setup(r, 0);
    assert(pr_res_00ba_zero_test_23b7_cpu(r, PR_RES_23B7_RAM_SIZE, &c) == PR_RES_23B7_RETURNED);
    assert((uint8_t)c.ax == 1u && (c.ax >> 8) == 0xABu && c.sp == S0 + 8u && c.bp == 0x7777u && c.cs == 0x8000u && c.ip == 0x9000u);
    assert(c.es == 0x3000u && c.di == 0x0100u && (c.flags & 0x08D5u) == 0x0044u && r[at(SS_, (uint16_t)(S0 - 3u))] == 1u);
    /* a non-zero byte: result 0; CMP 80h,0 sets SF only (80h has one bit set: odd parity, no PF) */
    c = setup(r, 0x80u); assert(pr_res_00ba_zero_test_23b7_cpu(r, PR_RES_23B7_RAM_SIZE, &c) == PR_RES_23B7_RETURNED);
    assert((uint8_t)c.ax == 0u && (c.flags & 0x08D5u) == 0x0080u);
    c = setup(r, 0x03u); assert(pr_res_00ba_zero_test_23b7_cpu(r, PR_RES_23B7_RAM_SIZE, &c) == PR_RES_23B7_RETURNED);
    assert((uint8_t)c.ax == 0u && (c.flags & 0x08D5u) == 0x0004u);                     /* 03h: two bits, even parity */
    /* the highest entry SP (FFF8): BP+6 = FFFC, the top edge the guard allows; LES's segment word is at FFFE. The wrapped segment read of
     * pinned DOSBox-X (offset above FFFC) is unreachable here because of the SP guard */
    c = setup(r, 0); c.sp = 0xFFF8u; c.ss = SS_;
    put16(r, SS_, 0xFFF8u, 0x9000u); put16(r, SS_, 0xFFFAu, 0x8000u);                   /* the return frame */
    r[at(SS_, 0xFFFCu)] = 0x00u; r[at(SS_, 0xFFFDu)] = 0x01u;                            /* character offset 0100 at FFFC.. */
    put16(r, SS_, 0xFFFEu, 0x3000u);                                                    /* segment at FFFE (BP+8) */
    assert(pr_res_00ba_zero_test_23b7_cpu(r, PR_RES_23B7_RAM_SIZE, &c) == PR_RES_23B7_RETURNED); assert(c.es == 0x3000u && (uint8_t)c.ax == 1u);
    /* argument rejection is atomic */
    c = setup(r, 0); memcpy(before, r, PR_RES_23B7_RAM_SIZE);
    c.ip = 0x23B8u; saved = c; assert(pr_res_00ba_zero_test_23b7_cpu(r, PR_RES_23B7_RAM_SIZE, &c) == PR_RES_23B7_ARGUMENT);
    c.ip = 0x23B7u; c.sp = 0x0003u; saved = c; assert(pr_res_00ba_zero_test_23b7_cpu(r, PR_RES_23B7_RAM_SIZE, &c) == PR_RES_23B7_ARGUMENT);
    c.sp = 0xFFF9u; saved = c; assert(pr_res_00ba_zero_test_23b7_cpu(r, PR_RES_23B7_RAM_SIZE, &c) == PR_RES_23B7_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_RES_23B7_RAM_SIZE)); c.sp = S0;
    assert(pr_res_00ba_zero_test_23b7_cpu(NULL, PR_RES_23B7_RAM_SIZE, &c) == PR_RES_23B7_ARGUMENT);
    assert(pr_res_00ba_zero_test_23b7_cpu(r, 0, &c) == PR_RES_23B7_ARGUMENT);
    assert(pr_res_00ba_zero_test_23b7_cpu(r, PR_RES_23B7_RAM_SIZE, NULL) == PR_RES_23B7_ARGUMENT);
    free(before); free(r); puts("RES 00BA 23B7 zero test, flags, LES edge and guard cases passed"); return 0;
}
