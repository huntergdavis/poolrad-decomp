#include "poolrad/recovered/overlay_u33_0b97_cpu.h"
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

/* Entry frame: near pointer P, X, Y; the far pointer G in DS:[6716]. The counter byte at SS:[P-2] starts at 10h. */
static pr_u33_0b97_regs setup(uint8_t *r, uint16_t gseg, uint16_t goff, uint16_t p, uint8_t x, uint8_t y)
{
    pr_u33_0b97_regs c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_BUS_RAM_SIZE);
    c.cs = CS_; c.ip = 0x0B97u; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777; c.ax = 0xABCD; c.flags = 0x0202;
    put16(r, c.ss, c.sp, 0x9000); put16(r, c.ss, (uint16_t)(c.sp + 2u), 0x8000); put16(r, c.ss, (uint16_t)(c.sp + 4u), p);
    put16(r, c.ss, (uint16_t)(c.sp + 6u), (uint16_t)(0xCD00u | x)); put16(r, c.ss, (uint16_t)(c.sp + 8u), (uint16_t)(0xCD00u | y));
    put16(r, DS_, 0x6716u, goff); put16(r, DS_, 0x6718u, gseg); r[at(SS_, (uint16_t)(p - 2u))] = 0x10u;
    return c;
}
static pr_bus flat_bus(uint8_t *r) { pr_bus b; memset(&b, 0, sizeof b); b.ram = r; return b; }

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE);
    pr_u33_0b97_regs c, saved; size_t budget; pr_bus bus; pr_ega *ega = malloc(sizeof *ega);
    assert(r && before && ega);
    /* X=3, Y=2, G=3000:0100: the cells are at 0100+2*50+3+7 = 016Eh (34h) and 016Fh (35h); the counter at SS:[P-2] goes 10h -> 11h */
    c = setup(r, 0x3000u, 0x0100u, 0x8030u, 3, 2); bus = flat_bus(r); budget = 1000;
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_RETURNED);
    assert(r[at(0x3000u, 0x016Eu)] == 0x34u && r[at(0x3000u, 0x016Fu)] == 0x35u && r[at(SS_, 0x802Eu)] == 0x11u);
    assert(c.cs == 0x8000u && c.ip == 0x9000u && c.sp == S0 + 10u && c.bp == 0x7777u);
    /* signed X and Y: X = FFh (-1), Y = FEh (-2): 0100h + (-2*50) + (-1) + 7 = 0100h - 5Eh = 00A2h and 00A3h */
    c = setup(r, 0x3000u, 0x0100u, 0x8030u, 0xFFu, 0xFEu); bus = flat_bus(r); budget = 1000;
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_RETURNED);
    assert(r[at(0x3000u, 0x00A2u)] == 0x34u && r[at(0x3000u, 0x00A3u)] == 0x35u);
    /* the counter increment's flags: 7Fh -> 80h sets OF, SF and AF; CF is the one the ADD DI,CX left */
    c = setup(r, 0x3000u, 0x0100u, 0x8030u, 3, 2); r[at(SS_, 0x802Eu)] = 0x7Fu; bus = flat_bus(r); budget = 1000;
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_RETURNED); assert(r[at(SS_, 0x802Eu)] == 0x80u && (c.flags & 0x08D4u) == 0x0890u);
    /* the three stores go through the bus: with an EGA attached, a grid store at A000:0100 writes all four planes (write mode 0, map mask 0Fh) and leaves the RAM bytes alone;
     * X=0, Y=0, G=A000:00F9 puts the cells at A000:0100 (34h) and A000:0101 (35h) */
    c = setup(r, 0xA000u, 0x00F9u, 0x8030u, 0, 0); pr_ega_reset(ega); memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 1000;
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_RETURNED);
    { unsigned p; for (p = 0; p < 4; ++p) assert(ega->plane[p][0x100] == 0x34u && ega->plane[p][0x101] == 0x35u); }
    assert(r[0xA0100] == 0xA5u && r[0xA0101] == 0xA5u);
    /* the same state without an EGA is flat RAM */
    c = setup(r, 0xA000u, 0x00F9u, 0x8030u, 0, 0); bus = flat_bus(r); budget = 1000;
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_RETURNED); assert(r[0xA0100] == 0x34u && r[0xA0101] == 0x35u);
    /* the counter increment goes through the bus: the stack segment placed in the aperture (SS = A000, P-2 = 0200h) reads and writes the planes (plane 0 is read) */
    c = setup(r, 0x3000u, 0x0100u, 0x0202u, 3, 2); c.ss = 0xA000u; c.sp = 0x4000u; put16(r, c.ss, c.sp, 0x9000); put16(r, c.ss, (uint16_t)(c.sp + 2u), 0x8000); put16(r, c.ss, (uint16_t)(c.sp + 4u), 0x0202u);
    put16(r, c.ss, (uint16_t)(c.sp + 6u), 3u); put16(r, c.ss, (uint16_t)(c.sp + 8u), 2u);
    pr_ega_reset(ega); ega->plane[0][0x0200] = 0x20u; memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 1000;
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_RETURNED);
    { unsigned p; for (p = 0; p < 4; ++p) assert(ega->plane[p][0x0200] == 0x21u); }
    /* the counter pointer on the return IP slot (P = BP+4, so P-2 = BP+2): the increment changes the return address: TRANSFER at the popped CS:IP */
    c = setup(r, 0x3000u, 0x0100u, (uint16_t)(S0 - 2u + 4u), 3, 2); bus = flat_bus(r); budget = 1000;
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_TRANSFER); assert(c.ip == 0x9011u && c.cs == 0x8000u && c.sp == S0 + 10u);   /* setup seeded the counter byte (10h) on the IP low byte, so 9010h + 1 */
    /* budget: 0 changes nothing; 3 stops before the first CBW */
    c = setup(r, 0x3000u, 0x0100u, 0x8030u, 3, 2); saved = c; memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 0;
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_LIMIT); assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE));
    c = setup(r, 0x3000u, 0x0100u, 0x8030u, 3, 2); bus = flat_bus(r); budget = 3;
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_LIMIT); assert(c.ip == 0x0B9Du && budget == 0 && (uint8_t)c.ax == 3u);
    /* argument rejection is atomic */
    c = setup(r, 0x3000u, 0x0100u, 0x8030u, 3, 2); memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 1000;
    c.ip = 0x0B98u; saved = c; assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_ARGUMENT);
    c.ip = 0x0B97u; c.sp = 0x000Fu; saved = c; assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_ARGUMENT);
    c.sp = 0xFFF7u; saved = c; assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE) && budget == 1000u); c.sp = S0;
    assert(pr_u33_0b97_cpu(NULL, PR_BUS_RAM_SIZE, &c, &budget) == PR_U33_0B97_ARGUMENT);
    assert(pr_u33_0b97_cpu(&bus, 0, &c, &budget) == PR_U33_0B97_ARGUMENT);
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, NULL, &budget) == PR_U33_0B97_ARGUMENT);
    assert(pr_u33_0b97_cpu(&bus, PR_BUS_RAM_SIZE, &c, NULL) == PR_U33_0B97_ARGUMENT);
    free(ega); free(before); free(r); puts("U33 0B97 synthetic, bus and guard cases passed"); return 0;
}
