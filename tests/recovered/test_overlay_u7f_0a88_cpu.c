#include "poolrad/recovered/overlay_u7f_0a88_cpu.h"
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
static uint32_t pool_of(const uint8_t *r, int i) { return get16(r, DS_, (uint16_t)(0x67F4 + 4 * i)) | ((uint32_t)get16(r, DS_, (uint16_t)(0x67F6 + 4 * i)) << 16u); }

/* The record at G has class byte 0Ch at +10h (so +0000 returns 0064h + 05DCh = 0640h), the money word at +88h+2I and W at +102h; the pool dword for I at DS:[67F4h+4I]. */
static pr_u7f_0a88_regs setup(uint8_t *r, uint16_t gseg, uint16_t goff, uint16_t index, uint16_t amount, uint16_t money, uint16_t w, uint32_t pool)
{
    pr_u7f_0a88_regs c; int i = (int8_t)(uint8_t)index; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_BUS_RAM_SIZE);
    c.cs = W_CS; c.ip = 0x0A88u; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777; c.ax = 0xABCD; c.flags = 0x0202;
    put16(r, c.ss, c.sp, 0x9000); put16(r, c.ss, (uint16_t)(c.sp + 2u), 0x8000); put16(r, c.ss, (uint16_t)(c.sp + 4u), index); put16(r, c.ss, (uint16_t)(c.sp + 6u), amount);
    put16(r, c.ss, (uint16_t)(c.sp + 8u), goff); put16(r, c.ss, (uint16_t)(c.sp + 10u), gseg);
    put16(r, DS_, (uint16_t)(0x67F4 + 4 * i), (uint16_t)pool); put16(r, DS_, (uint16_t)(0x67F6 + 4 * i), (uint16_t)(pool >> 16u));
    r[at(gseg, (uint16_t)(goff + 0x10u))] = 0x0C; r[at(gseg, (uint16_t)(goff + 0x16u))] = 0x00; put16(r, gseg, (uint16_t)(goff + 0x88u + 2 * i), money); put16(r, gseg, (uint16_t)(goff + 0x102u), w);
    r[at(W_CS, 0x61)] = 0xE8; r[at(W_CS, 0x62)] = 0x9C; r[at(W_CS, 0x63)] = 0xFF; r[at(W_CS, 0x80)] = 0xE8; r[at(W_CS, 0x81)] = 0x7D; r[at(W_CS, 0x82)] = 0xFF;
    r[at(W_CS, 0x0C)] = 0x9A; put16(r, W_CS, 0x0D, 0x13ABu); put16(r, W_CS, 0x0F, RES_CS);
    r[at(W_CS, 0xA9D)] = 0xE8; put16(r, W_CS, 0xA9Eu, 0xF5B4u); r[at(W_CS, 0xAAE)] = 0x9A; put16(r, W_CS, 0xAAFu, 0x0B0Bu); put16(r, W_CS, 0xAB1u, 0x6200u);
    r[at(W_CS, 0xAB3)] = 0x9A; put16(r, W_CS, 0xAB4u, 0x1693u); put16(r, W_CS, 0xAB6u, 0x8200u); r[at(W_CS, 0xB51)] = 0xE8; put16(r, W_CS, 0xB52u, 0xF4E6u);
    return c;
}
static pr_bus flat_bus(uint8_t *r) { pr_bus b; memset(&b, 0, sizeof b); b.ram = r; return b; }
static const pr_u7f_0a88_limits LIM = {65536, 65536, 65536, 65536, 1000000};

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE);
    pr_u7f_0a88_regs c, saved; size_t budget; pr_u7f_0a88_diagnostic d; pr_bus bus; pr_ega *ega = malloc(sizeof *ega);
    assert(r && before && ega);
    /* not above (W + AMOUNT = 0105h <= 0640h), pool 100h >= 5: no clamp. Money 1234h + 5, W 0100h + 5 (by +003A), pool 100h - 5 */
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x0005u, 0x1234u, 0x0100u, 0x100u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_RETURNED);
    assert(get16(r, 0x3000u, 0x018Cu) == 0x1239u && get16(r, 0x3000u, 0x0202u) == 0x0105u && pool_of(r, 2) == 0xFBu);
    assert(c.cs == 0x8000u && c.ip == 0x9000u && c.sp == S0 + 12u && c.bp == 0x7777u && d.callee == 3);
    /* the clamp: pool 3 < AMOUNT 5 moves 3 and empties the pool; the high byte of the index word is ignored */
    c = setup(r, 0x3000u, 0x0100u, 0xA502u, 0x0005u, 0x1234u, 0x0100u, 3u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_RETURNED);
    assert(get16(r, 0x3000u, 0x018Cu) == 0x1237u && get16(r, 0x3000u, 0x0202u) == 0x0103u && pool_of(r, 2) == 0u);
    /* a negative pool (signed compare of the high word) clamps to its LOW word: FFFF0002h moves 2 and leaves FFFF0000h */
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x0005u, 0x1234u, 0x0100u, 0xFFFF0002u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_RETURNED);
    assert(get16(r, 0x3000u, 0x018Cu) == 0x1236u && pool_of(r, 2) == 0xFFFF0000u);
    /* a pool above 64K (high word 1) never clamps */
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x0005u, 0x1234u, 0x0100u, 0x10000u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_RETURNED); assert(pool_of(r, 2) == 0xFFFBu);
    /* the index byte is signed (CBW): FFh selects the pool at 67F0h and the money word at +86h */
    c = setup(r, 0x3000u, 0x0100u, 0x00FFu, 0x0005u, 0x1234u, 0x0100u, 0x100u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_RETURNED);
    assert(get16(r, 0x3000u, 0x0186u) == 0x1239u && pool_of(r, -1) == 0xFBu);
    /* the money RMW goes through the bus: the record in the aperture, planes 0Fh-mapped. Plane 0 holds 34h/12h at +8Ch/+8Dh; W and the class byte read flat by +0054's bus
     * reads from the planes (0), so not above; the money word becomes 1239h in all four planes and the RAM bytes under the aperture are not written */
    c = setup(r, 0xA000u, 0x0100u, 0x0002u, 0x0005u, 0x0000u, 0x0000u, 0x100u); pr_ega_reset(ega); ega->plane[0][0x18C] = 0x34u; ega->plane[0][0x18D] = 0x12u;
    memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_RETURNED);
    { unsigned p; for (p = 0; p < 4; ++p) assert(ega->plane[p][0x18C] == 0x39u && ega->plane[p][0x18D] == 0x12u); }
    assert(r[0xA018C] == 0x00u && r[0xA018D] == 0x00u);
    /* the budget: 0 changes nothing; 10 stops before the first call (10 instructions precede 0A9D) */
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x0005u, 0x1234u, 0x0100u, 0x100u); saved = c; memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 0;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_LIMIT); assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE));
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x0005u, 0x1234u, 0x0100u, 0x100u); bus = flat_bus(r); budget = 10;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_LIMIT); assert(c.cs == W_CS && c.ip == 0x0A9Du && budget == 0 && d.callee == -1);
    /* a corrupted call site stops CODE before the return frame; a corrupted far call inside +0000 is a CALLEE stop that names +0054 */
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x0005u, 0x1234u, 0x0100u, 0x100u); r[at(W_CS, 0xA9Du)] = 0x9Au; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_CODE); assert(c.ip == 0x0A9Du && d.site == 0x0A9Du);
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x0005u, 0x1234u, 0x0100u, 0x100u); r[at(W_CS, 0xB51u)] = 0x9Au; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_CODE); assert(d.site == 0x0B51u && c.ip == 0x0B51u);
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x0005u, 0x1234u, 0x0100u, 0x100u); r[at(W_CS, 0x0Cu)] = 0xE8u; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_CALLEE); assert(d.callee == 0);
    /* the above branch's display call is checked before its return frame: with the far call to StrLoad corrupted the body stops CODE at 0AAE, the pool and money untouched */
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x7000u, 0x1234u, 0x0100u, 0x100u); r[at(W_CS, 0xAAEu)] = 0xE8u; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_CODE); assert(d.site == 0x0AAEu && c.ip == 0x0AAEu);
    assert(get16(r, 0x3000u, 0x018Cu) == 0x1234u && pool_of(r, 2) == 0x100u);
    /* argument rejection is atomic */
    c = setup(r, 0x3000u, 0x0100u, 0x0002u, 0x0005u, 0x1234u, 0x0100u, 0x100u); memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 100000;
    c.ip = 0x0A89u; saved = c; assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_ARGUMENT);
    c.ip = 0x0A88u; c.sp = 0x01FFu; saved = c; assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_ARGUMENT);
    c.sp = 0xFFF5u; saved = c; assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE) && budget == 100000u); c.sp = S0;
    assert(pr_u7f_0a88_cpu(NULL, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_0A88_ARGUMENT);
    assert(pr_u7f_0a88_cpu(&bus, 0, &c, &budget, &LIM, &d) == PR_U7F_0A88_ARGUMENT);
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, NULL, &budget, &LIM, &d) == PR_U7F_0A88_ARGUMENT);
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, NULL, &LIM, &d) == PR_U7F_0A88_ARGUMENT);
    assert(pr_u7f_0a88_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, NULL, &d) == PR_U7F_0A88_ARGUMENT);
    free(ega); free(before); free(r); puts("U7F0A88 synthetic, bus and guard cases passed"); return 0;
}
