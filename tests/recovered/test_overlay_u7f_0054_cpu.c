#include "poolrad/recovered/overlay_u7f_0054_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W_CS 0x6000u
#define RES_CS 0x7000u
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }
static uint16_t get16(const uint8_t *r, uint16_t s, uint16_t o) { size_t p = at(s, o); return (uint16_t)(r[p] | (r[(p + 1u) & 0xFFFFFu] << 8u)); }

/* Entry frame: out pointer, extra, record pointer. The record's +10h byte is 0Ch, so +0000 returns 0064h + 05DCh = 0640h; W (record +0102h) is given. */
static pr_res00ba_cpu setup(uint8_t *r, uint16_t out_seg, uint16_t out_off, uint16_t extra, uint16_t rec_seg, uint16_t rec_off, uint16_t w)
{
    pr_res00ba_cpu c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_BUS_RAM_SIZE);
    c.cs = W_CS; c.ip = 0x0054u; c.ds = 0x1000; c.ss = SS_; c.sp = S0; c.bp = 0x7777; c.ax = 0xABCD; c.flags = 0x0202;
    put16(r, c.ss, c.sp, 0x9000); put16(r, c.ss, (uint16_t)(c.sp + 2u), 0x8000); put16(r, c.ss, (uint16_t)(c.sp + 4u), out_off); put16(r, c.ss, (uint16_t)(c.sp + 6u), out_seg);
    put16(r, c.ss, (uint16_t)(c.sp + 8u), extra); put16(r, c.ss, (uint16_t)(c.sp + 10u), rec_off); put16(r, c.ss, (uint16_t)(c.sp + 12u), rec_seg);
    r[at(rec_seg, (uint16_t)(rec_off + 0x10u))] = 0x0C; put16(r, rec_seg, (uint16_t)(rec_off + 0x102u), w);
    r[at(W_CS, 0x61)] = 0xE8; r[at(W_CS, 0x62)] = 0x9C; r[at(W_CS, 0x63)] = 0xFF; r[at(W_CS, 0x80)] = 0xE8; r[at(W_CS, 0x81)] = 0x7D; r[at(W_CS, 0x82)] = 0xFF;
    r[at(W_CS, 0x0C)] = 0x9A; put16(r, W_CS, 0x0D, 0x13ABu); put16(r, W_CS, 0x0F, RES_CS);
    return c;
}
static pr_bus flat_bus(uint8_t *r) { pr_bus b; memset(&b, 0, sizeof b); b.ram = r; return b; }

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE);
    pr_res00ba_cpu c, saved; size_t budget; pr_u7f_0054_diagnostic d; pr_bus bus; pr_ega *ega = malloc(sizeof *ega);
    assert(r && before && ega);
    /* above: W + extra = 0800h > 0640h: stores 0640h - 0100h = 0540h through the out pointer, returns AL = 1, flags of that SUB (all clear) */
    c = setup(r, 0x4000u, 0x0200u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_RETURNED);
    assert(get16(r, 0x4000u, 0x0200u) == 0x0540u && (uint8_t)c.ax == 1u && (c.flags & 0x08D5u) == 0u && c.cs == 0x8000u && c.ip == 0x9000u && c.sp == S0 + 14u && c.bp == 0x7777u && c.dx == 0x0640u);
    /* not above: W + extra = 0100h <= 0640h: stores 0, AL = 0, flags of XOR AX,AX (ZF and PF) */
    c = setup(r, 0x4000u, 0x0200u, 0x0000u, 0x3000u, 0x0100u, 0x0100u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_RETURNED);
    assert(get16(r, 0x4000u, 0x0200u) == 0u && (uint8_t)c.ax == 0u && (c.flags & 0x08D5u) == 0x0044u);
    /* equality is not above (JBE): W + extra = 0640h */
    c = setup(r, 0x4000u, 0x0200u, 0x0540u, 0x3000u, 0x0100u, 0x0100u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_RETURNED); assert((uint8_t)c.ax == 0u && get16(r, 0x4000u, 0x0200u) == 0u);
    /* the out store goes through the bus: with an EGA attached, a store at A000:0100 writes all four planes (write mode 0, map mask 0Fh) and leaves the RAM bytes alone */
    c = setup(r, 0xA000u, 0x0100u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); pr_ega_reset(ega); memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_RETURNED);
    { unsigned p; for (p = 0; p < 4; ++p) assert(ega->plane[p][0x100] == 0x40u && ega->plane[p][0x101] == 0x05u); }
    assert(r[0xA0100] == 0xA5u && r[0xA0101] == 0xA5u);
    /* the same state without an EGA is flat RAM: the store lands in the RAM bytes */
    c = setup(r, 0xA000u, 0x0100u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_RETURNED); assert(r[0xA0100] == 0x40u && r[0xA0101] == 0x05u);
    /* the record's word is read through the bus: the record placed so +0102h is A000:1000, planes hold W = 0100h while the RAM bytes there hold 1234h; the +10h byte (read flat by the resident) is 0Ch in RAM */
    c = setup(r, 0x4000u, 0x0200u, 0x0700u, 0xA000u, (uint16_t)(0x1000u - 0x0102u), 0x1234u); pr_ega_reset(ega); ega->plane[0][0x1000] = 0x00u; ega->plane[0][0x1001] = 0x01u;
    memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_RETURNED); assert(get16(r, 0x4000u, 0x0200u) == 0x0540u);
    /* an out pointer on the body's own frame: the store at BP+2 (the return IP slot) replaces the return address: TRANSFER with the CPU left at the popped CS:IP */
    c = setup(r, SS_, (uint16_t)(S0 - 2u + 2u), 0x0700u, 0x3000u, 0x0100u, 0x0100u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_TRANSFER); assert(c.ip == 0x0540u && c.cs == 0x8000u && c.sp == S0 + 14u);
    /* the same store at BP-1 clobbers the returned flag byte: AL becomes the stored low byte */
    c = setup(r, SS_, (uint16_t)(S0 - 2u - 1u), 0x0700u, 0x3000u, 0x0100u, 0x0100u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_RETURNED); assert((uint8_t)c.ax == 0x40u);
    /* budget: 0 changes nothing; 6 stops at the first call before its return frame; a budget inside +0000 reports LIMIT with that body's CS:IP */
    c = setup(r, 0x4000u, 0x0200u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); saved = c; memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 0;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_LIMIT); assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE));
    c = setup(r, 0x4000u, 0x0200u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); bus = flat_bus(r); budget = 6;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_LIMIT); assert(c.ip == 0x0061u && c.cs == W_CS && c.sp == S0 - 10u && budget == 0);
    c = setup(r, 0x4000u, 0x0200u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); bus = flat_bus(r); budget = 10;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_LIMIT); assert(c.cs == W_CS && c.ip < 0x20u && budget == 0);
    /* a corrupted call site stops CODE before the return frame; so does a corrupted far call inside +0000 (reported as CALLEE with the callee's status) */
    c = setup(r, 0x4000u, 0x0200u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); r[at(W_CS, 0x81)] = 0x7Eu; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_CODE); assert(c.ip == 0x0080u && d.site == 0x0080u);
    c = setup(r, 0x4000u, 0x0200u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); r[at(W_CS, 0x62)] = 0x9Du; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_CODE); assert(c.ip == 0x0061u && c.sp == S0 - 10u);
    c = setup(r, 0x4000u, 0x0200u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); r[at(W_CS, 0x0C)] = 0xE8; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &d) == PR_U7F_0054_CALLEE); assert(d.callee_status == (int)PR_U7F_0000_CODE);
    /* argument rejection is atomic */
    c = setup(r, 0x4000u, 0x0200u, 0x0700u, 0x3000u, 0x0100u, 0x0100u); memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 100000;
    c.ip = 0x55; saved = c; assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0054_ARGUMENT);
    c.ip = 0x54; c.sp = 0x004F; saved = c; assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0054_ARGUMENT);
    c.sp = 0xFFF3; saved = c; assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0054_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE) && budget == 100000u); c.sp = S0;
    assert(pr_u7f_0054_cpu(NULL, PR_BUS_RAM_SIZE, &c, &budget, NULL) == PR_U7F_0054_ARGUMENT);
    assert(pr_u7f_0054_cpu(&bus, 0, &c, &budget, NULL) == PR_U7F_0054_ARGUMENT);
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, NULL, &budget, NULL) == PR_U7F_0054_ARGUMENT);
    assert(pr_u7f_0054_cpu(&bus, PR_BUS_RAM_SIZE, &c, NULL, NULL) == PR_U7F_0054_ARGUMENT);
    free(ega); free(before); free(r); puts("U7F0054 synthetic composition, bus and guard cases passed"); return 0;
}
