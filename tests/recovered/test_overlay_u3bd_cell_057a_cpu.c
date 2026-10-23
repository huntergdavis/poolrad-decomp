#include "poolrad/recovered/overlay_u3bd_cell_057a_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic state only: the model composes the raw +031F leaf model, which works from the frame in RAM, so no code bytes are placed. */
#define CS_ 0x6000u
#define DS_ 0x1000u
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }

static pr_u3bd_057a_cpu setup(uint8_t *r, uint8_t x, uint8_t y, uint8_t mapid)
{
    pr_u3bd_057a_cpu c; unsigned k; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_BUS_RAM_SIZE);
    c.cs = CS_; c.ip = 0x057Au; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777u; c.ax = 0xABCDu; c.flags = 0x0202u;
    put16(r, SS_, S0, 0x9000u); put16(r, SS_, (uint16_t)(S0 + 2u), 0x8000u);
    put16(r, SS_, (uint16_t)(S0 + 4u), (uint16_t)(0xCD00u | y)); put16(r, SS_, (uint16_t)(S0 + 6u), (uint16_t)(0xCD00u | x));
    r[at(DS_, 0x84DCu)] = mapid; put16(r, DS_, 0x6A5Cu, 0x0100u); put16(r, DS_, 0x6A5Eu, 0x3000u);
    for (k = 0; k < 256u; ++k) r[at(0x3000u, (uint16_t)(0x0300u + k))] = (uint8_t)(k ^ 0x5Au);   /* the cell table at buffer + 200h */
    return c;
}
static pr_bus flat_bus(uint8_t *r) { pr_bus b; memset(&b, 0, sizeof b); b.ram = r; return b; }

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE);
    pr_u3bd_057a_cpu c, saved; pr_bus bus; pr_ega *ega = malloc(sizeof *ega);
    assert(r && before && ega);
    /* in the grid (x=5, y=3): the cell 3*16+5 = 53 = 35h, value 35h ^ 5Ah = 6Fh; [BP-1] keeps it; SP+8, CS:IP back */
    c = setup(r, 5, 3, 1); bus = flat_bus(r);
    assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_RETURNED);
    assert((uint8_t)c.ax == 0x6Fu && r[at(SS_, (uint16_t)(S0 - 3u))] == 0x6Fu && c.sp == S0 + 8u && c.cs == 0x8000u && c.ip == 0x9000u && c.bp == 0x7777u);
    /* out of the grid on map 0 and map 10: the result is 0 and nothing is wrapped */
    c = setup(r, 0x20u, 3, 0); bus = flat_bus(r);
    assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_RETURNED); assert((uint8_t)c.ax == 0u && r[at(SS_, (uint16_t)(S0 + 6u))] == 0x20u);
    c = setup(r, 0x20u, 3, 10); bus = flat_bus(r);
    assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_RETURNED); assert((uint8_t)c.ax == 0u);
    /* out of the grid on another map: x = 20h (> 15) wraps to 0 IN PLACE, y = F0h (< 0) wraps to 15: the cell 15*16+0 = F0h, value F0h ^ 5Ah = AAh */
    c = setup(r, 0x20u, 0xF0u, 1); bus = flat_bus(r);
    assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_RETURNED);
    assert((uint8_t)c.ax == 0xAAu && r[at(SS_, (uint16_t)(S0 + 6u))] == 0x00u && r[at(SS_, (uint16_t)(S0 + 4u))] == 0x0Fu);
    /* flags at return are ADD DI,DX's: AX = 00F0h (y=15 << 4), DI = 0100h + 00F0h = 01F0h, + DX (x=0) = 01F0h: no carry, not zero, PF of F0h even, AF 0 */
    assert((c.flags & 0x08D5u) == 0x0004u);
    /* the buffer byte goes through the bus: buffer at A000:0000, the cell (x=1, y=0) at A000:0201 reads plane 0 */
    c = setup(r, 1, 0, 1); put16(r, DS_, 0x6A5Cu, 0x0000u); put16(r, DS_, 0x6A5Eu, 0xA000u); r[0xA0201] = 0x11u;
    pr_ega_reset(ega); ega->plane[0][0x0201] = 0x77u; memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega;
    assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_RETURNED); assert((uint8_t)c.ax == 0x77u);
    c = setup(r, 1, 0, 1); put16(r, DS_, 0x6A5Cu, 0x0000u); put16(r, DS_, 0x6A5Eu, 0xA000u); r[0xA0201] = 0x11u; bus = flat_bus(r);
    assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_RETURNED); assert((uint8_t)c.ax == 0x11u);
    /* argument rejection is atomic */
    c = setup(r, 5, 3, 1); memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r);
    c.ip = 0x057Bu; saved = c; assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_ARGUMENT);
    c.ip = 0x057Au; c.sp = 0x000Fu; saved = c; assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_ARGUMENT);
    c.sp = 0xFFF9u; saved = c; assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE)); c.sp = S0;
    assert(pr_u3bd_cell_057a_cpu(NULL, PR_BUS_RAM_SIZE, &c) == PR_U3BD_057A_ARGUMENT);
    assert(pr_u3bd_cell_057a_cpu(&bus, 0, &c) == PR_U3BD_057A_ARGUMENT);
    assert(pr_u3bd_cell_057a_cpu(&bus, PR_BUS_RAM_SIZE, NULL) == PR_U3BD_057A_ARGUMENT);
    free(ega); free(before); free(r); puts("U3BD 057A synthetic, wrap, gate, bus and guard cases passed"); return 0;
}
