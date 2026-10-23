#include "poolrad/recovered/overlay_u68_up_preflight_068d_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic state only. The model reads the LCALL operand (CS:06C1/06C3) and the stub's five bytes from RAM, so those are placed; the composed +0350
 * model works from the frame in RAM. With an all-zero buffer +0350 returns its default 1 inside the grid and on a map other than 0/0Ah, and 0 out of
 * the grid on map 0 or 0Ah (the audit's Unicorn run of the original, cases cell00-* and map-0-out/map-10-out). */
#define CS_ 0x6000u
#define DS_ 0x1000u
#define SS_ 0x2000u
#define WS_ 0x4000u
#define WO_ 0x0100u
#define STS 0x0FD5u
#define STO 0x002Fu
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }
static uint16_t get16(const uint8_t *r, uint16_t s, uint16_t o) { size_t p = at(s, o); return (uint16_t)(r[p] | (r[(p + 1u) & 0xFFFFFu] << 8u)); }

static pr_u68_068d_cpu setup(uint8_t *r, uint8_t x, uint8_t y, int8_t dx, int8_t dy, uint8_t mapid, int loaded)
{
    pr_u68_068d_cpu c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_BUS_RAM_SIZE);
    c.cs = CS_; c.ip = 0x068Du; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777u; c.ax = 0xABCDu; c.flags = 0x0202u;
    put16(r, SS_, S0, 0x9000u); put16(r, SS_, (uint16_t)(S0 + 2u), 0x8000u);
    put16(r, CS_, 0x06C1u, STO); put16(r, CS_, 0x06C3u, STS);
    if (loaded) { r[at(STS, STO)] = 0xEAu; put16(r, STS, (uint16_t)(STO + 1u), 0x0350u); put16(r, STS, (uint16_t)(STO + 3u), 0x7000u); }
    else { r[at(STS, STO)] = 0xCDu; r[at(STS, (uint16_t)(STO + 1u))] = 0x3Fu; }
    r[at(DS_, 0x6AADu)] = x; r[at(DS_, 0x6AAEu)] = y; r[at(DS_, 0x6AAFu)] = 2u;
    r[at(DS_, 0x2736u + 2u)] = (uint8_t)dx; r[at(DS_, 0x273Fu + 2u)] = (uint8_t)dy;
    put16(r, DS_, 0x49D6u, WO_); put16(r, DS_, 0x49D8u, WS_); put16(r, WS_, (uint16_t)(WO_ + 0x5AAu), 0x5A5Au);
    r[at(DS_, 0x84DCu)] = mapid; put16(r, DS_, 0x6A5Cu, 0x0000u); put16(r, DS_, 0x6A5Eu, 0x5000u);
    memset(r + at(0x5000u, 0), 0, 0x402u);
    return c;
}
static pr_bus flat_bus(uint8_t *r) { pr_bus b; memset(&b, 0, sizeof b); b.ram = r; return b; }
static int returned(const pr_u68_068d_cpu *c) { return c->sp == S0 + 4u && c->cs == 0x8000u && c->ip == 0x9000u && c->bp == 0x7777u; }
static pr_u68_068d_status run(uint8_t *r, pr_u68_068d_cpu *c) { pr_bus b = flat_bus(r); return pr_u68_up_preflight_068d_cpu(&b, PR_BUS_RAM_SIZE, c); }

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE);
    pr_u68_068d_cpu c, saved; pr_bus bus; pr_ega *ega = malloc(sizeof *ega);
    assert(r && before && ega);
    /* in range (5,3) + (1,0) = (6,3): the candidate is never stored, and the window word is only cleared */
    c = setup(r, 5, 3, 1, 0, 1, 1); assert(run(r, &c) == PR_U68_068D_RETURNED && returned(&c));
    assert(r[at(DS_, 0x6AADu)] == 5u && r[at(DS_, 0x6AAEu)] == 3u && get16(r, WS_, WO_ + 0x5AAu) == 0u);
    /* each clamp stores its bound to the party byte and raises the window word: x 15+1 -> 0Fh, x 0-1 -> 0, y 15+1 -> 0Fh, y 0-1 -> 0 */
    c = setup(r, 15, 3, 1, 0, 1, 1); assert(run(r, &c) == PR_U68_068D_RETURNED && returned(&c));
    assert(r[at(DS_, 0x6AADu)] == 0x0Fu && r[at(DS_, 0x6AAEu)] == 3u && get16(r, WS_, WO_ + 0x5AAu) == 1u);
    c = setup(r, 0, 3, -1, 0, 1, 1); assert(run(r, &c) == PR_U68_068D_RETURNED); assert(r[at(DS_, 0x6AADu)] == 0u && get16(r, WS_, WO_ + 0x5AAu) == 1u);
    c = setup(r, 5, 15, 0, 1, 1, 1); assert(run(r, &c) == PR_U68_068D_RETURNED); assert(r[at(DS_, 0x6AAEu)] == 0x0Fu && get16(r, WS_, WO_ + 0x5AAu) == 1u);
    c = setup(r, 5, 0, 0, -1, 1, 1); assert(run(r, &c) == PR_U68_068D_RETURNED); assert(r[at(DS_, 0x6AAEu)] == 0u && get16(r, WS_, WO_ + 0x5AAu) == 1u);
    /* the compares are signed: x = 80h is -128 after CBW, so it clamps to 0 (the +0350 wrap leaves the party byte alone: it rewrites only its own frame) */
    c = setup(r, 0x80u, 3, 0, 0, 1, 1); assert(run(r, &c) == PR_U68_068D_RETURNED); assert(r[at(DS_, 0x6AADu)] == 0u && get16(r, WS_, WO_ + 0x5AAu) == 1u);
    /* the last CMP sets the return flags: y 3 vs 0 -> no carry, not zero, not sign; PF of 03h even */
    c = setup(r, 5, 3, 1, 0, 1, 1); assert(run(r, &c) == PR_U68_068D_RETURNED); assert((c.flags & 0x08D5u) == 0x0004u);
    /* a zero result (out of the grid on map 0 and on map 0Ah): JE past the clamps; the out-of-range x is left as it was */
    c = setup(r, 0x14u, 3, 1, 0, 0, 1); assert(run(r, &c) == PR_U68_068D_RETURNED && returned(&c));
    assert(r[at(DS_, 0x6AADu)] == 0x14u && get16(r, WS_, WO_ + 0x5AAu) == 0u && (c.flags & 0x0040u));
    c = setup(r, 0x14u, 3, 1, 0, 10, 1); assert(run(r, &c) == PR_U68_068D_RETURNED); assert(r[at(DS_, 0x6AADu)] == 0x14u);
    /* an unloaded stub (INT 3Fh): OVERLAY at the stub, with x, y, facing and the far return frame pushed and the window word already cleared */
    c = setup(r, 5, 3, 1, 0, 1, 0); assert(run(r, &c) == PR_U68_068D_OVERLAY);
    assert(c.cs == STS && c.ip == STO && c.sp == S0 - 18u && c.bp == S0 - 2u && get16(r, SS_, c.sp) == 0x06C5u && get16(r, SS_, (uint16_t)(c.sp + 2u)) == CS_);
    assert((uint8_t)get16(r, SS_, (uint16_t)(c.sp + 4u)) == 2u && (uint8_t)get16(r, SS_, (uint16_t)(c.sp + 6u)) == 3u && (uint8_t)get16(r, SS_, (uint16_t)(c.sp + 8u)) == 5u);
    assert(get16(r, WS_, WO_ + 0x5AAu) == 0u);
    /* a loaded far JMP to another offset is not +0350: OVERLAY too */
    c = setup(r, 5, 3, 1, 0, 1, 1); put16(r, STS, (uint16_t)(STO + 1u), 0x047Bu); assert(run(r, &c) == PR_U68_068D_OVERLAY);
    /* the window word stores go through the bus: window at A000:0000, the word at A000:05AA reaches the planes and not RAM */
    c = setup(r, 15, 3, 1, 0, 1, 1); put16(r, DS_, 0x49D6u, 0x0000u); put16(r, DS_, 0x49D8u, 0xA000u); put16(r, 0xA000u, 0x05AAu, 0x5A5Au);
    pr_ega_reset(ega); memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega;
    assert(pr_u68_up_preflight_068d_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U68_068D_RETURNED);
    assert(get16(r, 0xA000u, 0x05AAu) == 0x5A5Au && ega->plane[0][0x05AA] == 1u && ega->plane[0][0x05AB] == 0u);
    /* argument rejection is atomic */
    c = setup(r, 5, 3, 1, 0, 1, 1); memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r);
    c.ip = 0x068Eu; saved = c; assert(pr_u68_up_preflight_068d_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U68_068D_ARGUMENT);
    c.ip = 0x068Du; c.sp = 0x002Fu; saved = c; assert(pr_u68_up_preflight_068d_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U68_068D_ARGUMENT);
    c.sp = 0xFFFDu; saved = c; assert(pr_u68_up_preflight_068d_cpu(&bus, PR_BUS_RAM_SIZE, &c) == PR_U68_068D_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE)); c.sp = S0;
    assert(pr_u68_up_preflight_068d_cpu(NULL, PR_BUS_RAM_SIZE, &c) == PR_U68_068D_ARGUMENT);
    assert(pr_u68_up_preflight_068d_cpu(&bus, 0, &c) == PR_U68_068D_ARGUMENT);
    assert(pr_u68_up_preflight_068d_cpu(&bus, PR_BUS_RAM_SIZE, NULL) == PR_U68_068D_ARGUMENT);
    free(ega); free(before); free(r); puts("U68 068D clamp, zero-result, stub, bus and guard cases passed"); return 0;
}
