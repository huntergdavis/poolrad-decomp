#include "poolrad/recovered/overlay_u6a_0824_raw_cpu.h"
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

/* The 21 array bytes are at G+17h; DS:[5D92]/[5D94] hold G. The array is written first, then the entry frame. */
static pr_u6a_0824_raw_regs setup(uint8_t *r, uint16_t gseg, uint16_t goff, const uint8_t *arr)
{
    pr_u6a_0824_raw_regs c; unsigned k; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_BUS_RAM_SIZE);
    c.cs = CS_; c.ip = 0x0824u; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777; c.ax = 0xABCD; c.flags = 0x0202;
    for (k = 0; k < 21; ++k) r[at(gseg, (uint16_t)(goff + 0x17u + k))] = arr[k];
    put16(r, DS_, 0x5D92u, goff); put16(r, DS_, 0x5D94u, gseg);
    put16(r, c.ss, c.sp, 0x9000); put16(r, c.ss, (uint16_t)(c.sp + 2u), 0x8000);
    return c;
}
static pr_bus flat_bus(uint8_t *r) { pr_bus b; memset(&b, 0, sizeof b); b.ram = r; return b; }

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE); uint8_t arr[21]; unsigned k, p;
    pr_u6a_0824_raw_regs c, saved; size_t budget; pr_bus bus; pr_ega *ega = malloc(sizeof *ega);
    assert(r && before && ega);
    /* descending 21..1 sorts to 1..21, and the loop leaves SP, BP and CS:IP as the caller had them */
    for (k = 0; k < 21; ++k) arr[k] = (uint8_t)(21u - k);
    c = setup(r, 0x3000u, 0x0100u, arr); bus = flat_bus(r); budget = 100000;
    assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_RETURNED);
    for (k = 0; k < 21; ++k) assert(r[at(0x3000u, (uint16_t)(0x0100u + 0x17u + k))] == k + 1u);
    assert(c.cs == 0x8000u && c.ip == 0x9000u && c.sp == S0 + 4u && c.bp == 0x7777u);
    /* the high bit does not take part in the order but travels with its byte: 83 01 82 then 7Fh x 18 -> 01 82 83 7F... (hand-run of the exchange loop) */
    memset(arr, 0x7F, sizeof arr); arr[0] = 0x83; arr[1] = 0x01; arr[2] = 0x82;
    c = setup(r, 0x3000u, 0x0100u, arr); bus = flat_bus(r); budget = 100000;
    assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_RETURNED);
    assert(r[at(0x3000u, 0x0117u)] == 0x01u && r[at(0x3000u, 0x0118u)] == 0x82u && r[at(0x3000u, 0x0119u)] == 0x83u && r[at(0x3000u, 0x011Au)] == 0x7Fu);
    /* the array accesses go through the bus: with an EGA attached and the array at A000:0100.., plane 0 holds 21..1; reads see plane 0 and every write goes to all four planes */
    for (k = 0; k < 21; ++k) arr[k] = (uint8_t)(21u - k);
    c = setup(r, 0xA000u, (uint16_t)(0x0100u - 0x17u), arr); pr_ega_reset(ega); for (k = 0; k < 21; ++k) ega->plane[0][0x100 + k] = (uint8_t)(21u - k);
    memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 100000;
    assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_RETURNED);
    for (p = 0; p < 4; ++p) for (k = 0; k < 21; ++k) assert(ega->plane[p][0x100 + k] == k + 1u);
    for (k = 0; k < 21; ++k) assert(r[0xA0100 + k] == (uint8_t)(21u - k));            /* the RAM bytes under the aperture were never touched */
    /* the same state without an EGA is flat RAM */
    c = setup(r, 0xA000u, (uint16_t)(0x0100u - 0x17u), arr); bus = flat_bus(r); budget = 100000;
    assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_RETURNED); for (k = 0; k < 21; ++k) assert(r[0xA0100 + k] == k + 1u);
    /* an array over the return address: array = 00 90 00 80 then 7Fh x 17; the exchange loop leaves 00 00 80 90 there, so the return pops IP 0000h and CS 9080h: TRANSFER */
    memset(arr, 0x7F, sizeof arr); arr[0] = 0x00; arr[1] = 0x90; arr[2] = 0x00; arr[3] = 0x80;
    c = setup(r, SS_, (uint16_t)(S0 - 0x17u), arr); bus = flat_bus(r); budget = 100000;
    assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_TRANSFER); assert(c.ip == 0x0000u && c.cs == 0x9080u && c.sp == S0 + 4u);
    /* budget: 0 changes nothing; 3 stops after PUSH BP, MOV BP,SP and SUB SP,4 */
    for (k = 0; k < 21; ++k) arr[k] = (uint8_t)(21u - k);
    c = setup(r, 0x3000u, 0x0100u, arr); saved = c; memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 0;
    assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_LIMIT); assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE));
    c = setup(r, 0x3000u, 0x0100u, arr); bus = flat_bus(r); budget = 3;
    assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_LIMIT); assert(c.ip == 0x082Au && budget == 0 && c.sp == S0 - 6u && c.bp == S0 - 2u);
    /* argument rejection is atomic */
    c = setup(r, 0x3000u, 0x0100u, arr); memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 100000;
    c.ip = 0x0825u; saved = c; assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_ARGUMENT);
    c.ip = 0x0824u; c.sp = 0x0007u; saved = c; assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_ARGUMENT);
    c.sp = 0xFFFDu; saved = c; assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE) && budget == 100000u); c.sp = S0;
    assert(pr_u6a_0824_raw_cpu(NULL, PR_BUS_RAM_SIZE, &c, &budget) == PR_U6A_0824_RAW_ARGUMENT);
    assert(pr_u6a_0824_raw_cpu(&bus, 0, &c, &budget) == PR_U6A_0824_RAW_ARGUMENT);
    assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, NULL, &budget) == PR_U6A_0824_RAW_ARGUMENT);
    assert(pr_u6a_0824_raw_cpu(&bus, PR_BUS_RAM_SIZE, &c, NULL) == PR_U6A_0824_RAW_ARGUMENT);
    free(ega); free(before); free(r); puts("U6A 0824 synthetic, bus and guard cases passed"); return 0;
}
