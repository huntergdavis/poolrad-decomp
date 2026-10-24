#include "poolrad/recovered/overlay_u25_compare_words_18eb_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic state: the model reads only the LCALL bytes at CS:18FA (9A A8 16, then the FillChar segment) and composes the C FillChar. */
#define CS_ 0x6000u
#define DS_ 0x5000u
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }

static pr_u25_18eb_cpu setup(uint8_t *r, uint16_t ds, uint16_t ss, uint16_t sp, uint16_t a, uint16_t b)
{
    pr_u25_18eb_cpu c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_U25_18EB_RAM_SIZE);
    r[at(CS_, 0x18FAu)] = 0x9Au; put16(r, CS_, 0x18FBu, 0x16A8u); put16(r, CS_, 0x18FDu, 0x0AF8u);
    c.cs = CS_; c.ip = 0x18EBu; c.ds = ds; c.es = 0x1111u; c.ss = ss; c.sp = sp; c.bp = 0x7777u; c.ax = 0xABCDu; c.cx = 0x4321u; c.flags = 0x0602u;
    put16(r, ss, sp, 0x9000u); put16(r, ss, (uint16_t)(sp + 2u), 0x8000u); put16(r, ss, (uint16_t)(sp + 4u), b); put16(r, ss, (uint16_t)(sp + 6u), a);
    return c;
}

static void bytes(const uint8_t *r, uint16_t ds, const uint8_t want[6])
{ for (unsigned k = 0; k < 6u; ++k) assert(r[at(ds, (uint16_t)(0x6F78u + k))] == want[k]); }

int main(void)
{
    uint8_t *r = malloc(PR_U25_18EB_RAM_SIZE), *before = malloc(PR_U25_18EB_RAM_SIZE); pr_u25_18eb_cpu c, saved;
    static const uint8_t below[6] = {0, 1, 1, 0, 1, 0}, equal[6] = {1, 0, 0, 0, 1, 1}, above[6] = {0, 1, 0, 1, 0, 1}, alias[6] = {1, 1, 1, 0, 1, 0};
    assert(r && before);
    /* 3 vs 9: below; the last CMP (3-9 = FFFA) sets CF, PF, AF, SF; FillChar leaves CX 0, ES:DI past the fill, AL 0 then AX = a, DF clear */
    c = setup(r, DS_, SS_, S0, 3u, 9u);
    assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_RETURNED); bytes(r, DS_, below);
    assert(c.ax == 3u && c.cx == 0u && c.di == 0x6F7Eu && c.es == DS_ && c.bx == (uint16_t)(S0 - 14u) && !(c.flags & 0x0400u));
    assert((c.flags & 0x08D5u) == 0x0095u && c.sp == S0 + 8u && c.bp == 0x7777u && c.cs == 0x8000u && c.ip == 0x9000u);
    /* equal (8000h, 8000h): ZF and PF */
    c = setup(r, DS_, SS_, S0, 0x8000u, 0x8000u);
    assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_RETURNED); bytes(r, DS_, equal); assert((c.flags & 0x08D5u) == 0x0044u);
    /* unsigned above (8000h, 7FFFh), signed below: 8000h-7FFFh = 1 sets AF and OF only */
    c = setup(r, DS_, SS_, S0, 0x8000u, 0x7FFFu);
    assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_RETURNED); bytes(r, DS_, above); assert((c.flags & 0x08D5u) == 0x0810u);
    /* DS = SS, entry SP 6F74: b and a ARE flag bytes 0-1 and 2-3. The clear zeroes both; every compare rereads them, so each store changes
     * the next compare: 0=0 (b := 1), 0<>1 (b := 101h), 0<101h (a := 1), 1>101h no, 1<=101h, 1>=101h no */
    c = setup(r, SS_, SS_, 0x6F74u, 0x5555u, 0x5555u);
    assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_RETURNED); bytes(r, SS_, alias);
    assert(c.ax == 1u && c.sp == 0x6F7Cu && c.cs == 0x8000u && c.ip == 0x9000u);
    /* DS = SS, entry SP 6F78: the clear zeroes the return frame and b; a = 5 > 0 stores into the return frame: RETF pops 0100:0100 */
    c = setup(r, SS_, SS_, 0x6F78u, 5u, 7u);
    assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_TRANSFER); assert(c.cs == 0x0100u && c.ip == 0x0100u && c.sp == 0x6F80u);
    /* the LCALL bytes are checked: a different opcode stops at +18FA */
    c = setup(r, DS_, SS_, S0, 1u, 2u); r[at(CS_, 0x18FAu)] = 0xE8u;
    assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_CODE); assert(c.ip == 0x18FAu && c.cs == CS_);
    /* the SP edges the guard allows */
    c = setup(r, DS_, SS_, 0x000Eu, 2u, 1u); assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_RETURNED); bytes(r, DS_, above);
    c = setup(r, DS_, SS_, 0xFFF8u, 2u, 1u); assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_RETURNED); bytes(r, DS_, above);
    /* argument rejection is atomic */
    c = setup(r, DS_, SS_, S0, 1u, 2u); memcpy(before, r, PR_U25_18EB_RAM_SIZE);
    c.ip = 0x18ECu; saved = c; assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_ARGUMENT); assert(!memcmp(&c, &saved, sizeof c));
    c.ip = 0x18EBu; c.sp = 0x000Du; saved = c; assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_ARGUMENT); assert(!memcmp(&c, &saved, sizeof c));
    c.sp = 0xFFF9u; saved = c; assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_U25_18EB_RAM_SIZE)); c.sp = S0;
    assert(pr_u25_compare_words_18eb_cpu(NULL, PR_U25_18EB_RAM_SIZE, &c) == PR_U25_18EB_ARGUMENT);
    assert(pr_u25_compare_words_18eb_cpu(r, 0, &c) == PR_U25_18EB_ARGUMENT);
    assert(pr_u25_compare_words_18eb_cpu(r, PR_U25_18EB_RAM_SIZE, NULL) == PR_U25_18EB_ARGUMENT);
    free(before); free(r); puts("U25 18EB word compare: relations, flags, frame aliasing, transfer, code and guard cases passed"); return 0;
}
