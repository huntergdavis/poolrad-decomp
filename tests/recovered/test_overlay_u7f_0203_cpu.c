#include "poolrad/recovered/overlay_u7f_0203_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W_CS 0x6000u
#define RES_CS 0x7000u
#define DS_ 0x1000u
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }
static uint16_t get16(const uint8_t *r, uint16_t s, uint16_t o) { size_t p = at(s, o); return (uint16_t)(r[p] | (r[(p + 1u) & 0xFFFFFu] << 8u)); }

/* The record at G has class byte 0Ch at +10h (so +0000 returns 0064h + 05DCh = 0640h), the accumulator word at +90h and W at +102h. */
static pr_u7f_0203_regs setup(uint8_t *r, uint16_t gseg, uint16_t goff, uint16_t amount, uint16_t acc, uint16_t w)
{
    pr_u7f_0203_regs c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_BUS_RAM_SIZE);
    c.cs = W_CS; c.ip = 0x0203u; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777; c.ax = 0xABCD; c.flags = 0x0202;
    put16(r, c.ss, c.sp, 0x9000); put16(r, c.ss, (uint16_t)(c.sp + 2u), 0x8000); put16(r, c.ss, (uint16_t)(c.sp + 4u), amount);
    put16(r, DS_, 0x5D92u, goff); put16(r, DS_, 0x5D94u, gseg); put16(r, DS_, 0x6804u, 0x0010u); put16(r, DS_, 0x6806u, 0x0000u);
    r[at(gseg, (uint16_t)(goff + 0x10u))] = 0x0C; r[at(gseg, (uint16_t)(goff + 0x16u))] = 0x00; put16(r, gseg, (uint16_t)(goff + 0x90u), acc); put16(r, gseg, (uint16_t)(goff + 0x102u), w);
    r[at(W_CS, 0x61)] = 0xE8; r[at(W_CS, 0x62)] = 0x9C; r[at(W_CS, 0x63)] = 0xFF; r[at(W_CS, 0x80)] = 0xE8; r[at(W_CS, 0x81)] = 0x7D; r[at(W_CS, 0x82)] = 0xFF;
    r[at(W_CS, 0x0C)] = 0x9A; put16(r, W_CS, 0x0D, 0x13ABu); put16(r, W_CS, 0x0F, RES_CS);
    r[at(W_CS, 0x21A)] = 0xE8; put16(r, W_CS, 0x21Bu, 0xFE37u); r[at(W_CS, 0x22B)] = 0x9A; put16(r, W_CS, 0x22Cu, 0x0B0Bu); put16(r, W_CS, 0x22Eu, 0x6200u);
    r[at(W_CS, 0x230)] = 0x9A; put16(r, W_CS, 0x231u, 0x1693u); put16(r, W_CS, 0x233u, 0x8200u); r[at(W_CS, 0x24D)] = 0xE8; put16(r, W_CS, 0x24Eu, 0xFDEAu);
    r[at(W_CS, 0x28B)] = 0xE8; put16(r, W_CS, 0x28Cu, 0xFDACu);
    return c;
}
static pr_bus flat_bus(uint8_t *r) { pr_bus b; memset(&b, 0, sizeof b); b.ram = r; return b; }
static const pr_u7f_0203_limits LIM = {65536, 65536, 65536, 65536, 1000000};

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE);
    pr_u7f_0203_regs c, saved; size_t budget; pr_u7f_0203_diagnostic d; pr_bus bus; pr_ega *ega = malloc(sizeof *ega);
    assert(r && before && ega);
    /* not above: W + AMOUNT = 0200h <= 0640h. G+90h := 1234h + 0100h = 1334h; +003A adds AMOUNT to W (0100h + 0100h = 0200h); the counter is untouched; no display */
    c = setup(r, 0x3000u, 0x0100u, 0x0100u, 0x1234u, 0x0100u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_RETURNED);
    assert(get16(r, 0x3000u, 0x0190u) == 0x1334u && get16(r, 0x3000u, 0x0202u) == 0x0200u && get16(r, DS_, 0x6804u) == 0x0010u && get16(r, DS_, 0x6806u) == 0u);
    assert(c.cs == 0x8000u && c.ip == 0x9000u && c.sp == S0 + 6u && c.bp == 0x7777u && d.callee == 4);
    /* the accumulator RMW goes through the bus: G+90h in the aperture, planes 0Fh-mapped: plane 0 holds 34h/12h at +90h/+91h, W is read from the planes (0), so the branch is not-above;
     * the accumulator becomes 1334h in all four planes and the RAM bytes under the aperture are not written; +003A and +0000's reads are flat RAM by their contracts */
    c = setup(r, 0xA000u, 0x0100u, 0x0100u, 0x0000u, 0x0000u); pr_ega_reset(ega); ega->plane[0][0x190] = 0x34u; ega->plane[0][0x191] = 0x12u;
    memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 100000;
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_RETURNED);
    { unsigned p; for (p = 0; p < 4; ++p) assert(ega->plane[p][0x190] == 0x34u && ega->plane[p][0x191] == 0x13u); }
    assert(r[0xA0190] == 0x00u && r[0xA0191] == 0x00u);
    /* the same state without an EGA is flat RAM */
    c = setup(r, 0xA000u, 0x0100u, 0x0100u, 0x1234u, 0x0000u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_RETURNED); assert(get16(r, 0xA000u, 0x0190u) == 0x1334u);
    /* the budget: 0 changes nothing; 10 stops before the first call (10 instructions precede 021A) */
    c = setup(r, 0x3000u, 0x0100u, 0x0100u, 0x1234u, 0x0100u); saved = c; memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 0;
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_LIMIT); assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE));
    c = setup(r, 0x3000u, 0x0100u, 0x0100u, 0x1234u, 0x0100u); bus = flat_bus(r); budget = 10;
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_LIMIT); assert(c.cs == W_CS && c.ip == 0x021Au && budget == 0 && d.callee == -1);
    /* a corrupted call site stops CODE before the return frame; a corrupted far call inside +0000 is a CALLEE stop that names +0054 */
    c = setup(r, 0x3000u, 0x0100u, 0x0100u, 0x1234u, 0x0100u); r[at(W_CS, 0x21Au)] = 0x9Au; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_CODE); assert(c.ip == 0x021Au && d.site == 0x021Au);
    c = setup(r, 0x3000u, 0x0100u, 0x0100u, 0x1234u, 0x0100u); r[at(W_CS, 0x28Bu)] = 0x9Au; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_CODE); assert(d.site == 0x028Bu);
    c = setup(r, 0x3000u, 0x0100u, 0x0100u, 0x1234u, 0x0100u); r[at(W_CS, 0x0Cu)] = 0xE8u; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_CALLEE); assert(d.callee == 0);
    /* the above branch's display call is checked before its return frame: with the far call to StrLoad corrupted the body stops CODE at 022B */
    c = setup(r, 0x3000u, 0x0100u, 0x7000u, 0x1234u, 0x0100u); r[at(W_CS, 0x22Bu)] = 0xE8u; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_CODE); assert(d.site == 0x022Bu && c.ip == 0x022Bu);
    /* argument rejection is atomic */
    c = setup(r, 0x3000u, 0x0100u, 0x0100u, 0x1234u, 0x0100u); memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 100000;
    c.ip = 0x0204u; saved = c; assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_ARGUMENT);
    c.ip = 0x0203u; c.sp = 0x01FFu; saved = c; assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_ARGUMENT);
    c.sp = 0xFFF9u; saved = c; assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE) && budget == 100000u); c.sp = S0;
    assert(pr_u7f_0203_cpu(NULL, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0203_ARGUMENT);
    assert(pr_u7f_0203_cpu(&bus, 0, &c, &budget, &LIM, &d) == PR_U7F_0203_ARGUMENT);
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, NULL, &budget, &LIM, &d) == PR_U7F_0203_ARGUMENT);
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, NULL, &LIM, &d) == PR_U7F_0203_ARGUMENT);
    assert(pr_u7f_0203_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, NULL, &d) == PR_U7F_0203_ARGUMENT);
    free(ega); free(before); free(r); puts("U7F0203 synthetic, bus and guard cases passed"); return 0;
}
