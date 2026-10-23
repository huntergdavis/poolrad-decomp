#include "poolrad/recovered/overlay_u7f_044c_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W_CS 0x6000u
#define RES_CS 0x7000u
#define SS_ 0x2000u
#define S0 0x8000u
#define PSEG 0x3000u
#define QSEG 0x3400u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }
static uint16_t get16(const uint8_t *r, uint16_t s, uint16_t o) { size_t p = at(s, o); return (uint16_t)(r[p] | (r[(p + 1u) & 0xFFFFFu] << 8u)); }

/* INDEX 2 (array word at +88h+4), P (destination) class byte 0Ch so +0000 returns 0640h, P's word at +0102h = PW, P/Q array words PA/QA, Q's word at +0102h = QW. The pointer cells are at 3800:0000 (PP) and
 * 3800:0010 (QQ) unless the caller moves them; the body's call-site bytes and the 13AB far call's relocated segment word are set as the loader leaves them. */
static pr_u7f_044c_regs setup(uint8_t *r, uint16_t extra, uint16_t pseg, uint16_t poff, uint16_t pw, uint16_t pa, uint16_t qa, uint16_t qw, uint16_t cellseg, uint16_t celloff)
{
    pr_u7f_044c_regs c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_BUS_RAM_SIZE);
    c.cs = W_CS; c.ip = 0x044Cu; c.ds = 0x1000; c.ss = SS_; c.sp = S0; c.bp = 0x7777; c.ax = 0xABCD; c.flags = 0x0202;
    r[at(pseg, (uint16_t)(poff + 0x10u))] = 0x0C; r[at(pseg, (uint16_t)(poff + 0x16u))] = 0x00; put16(r, pseg, (uint16_t)(poff + 0x102u), pw); put16(r, pseg, (uint16_t)(poff + 0x88u + 4u), pa);
    put16(r, QSEG, 0x0100u + 0x88u + 4u, qa); put16(r, QSEG, 0x0100u + 0x102u, qw);
    put16(r, cellseg, celloff, poff); put16(r, cellseg, (uint16_t)(celloff + 2u), pseg); put16(r, 0x3800u, 0x0010u, 0x0100u); put16(r, 0x3800u, 0x0012u, QSEG);
    put16(r, c.ss, c.sp, 0x9000); put16(r, c.ss, (uint16_t)(c.sp + 2u), 0x8000); put16(r, c.ss, (uint16_t)(c.sp + 4u), 0xAB02u); put16(r, c.ss, (uint16_t)(c.sp + 6u), extra);
    put16(r, c.ss, (uint16_t)(c.sp + 8u), celloff); put16(r, c.ss, (uint16_t)(c.sp + 10u), cellseg); put16(r, c.ss, (uint16_t)(c.sp + 12u), 0x0010u); put16(r, c.ss, (uint16_t)(c.sp + 14u), 0x3800u);
    r[at(W_CS, 0x0C)] = 0x9A; put16(r, W_CS, 0x0D, 0x13ABu); put16(r, W_CS, 0x0F, RES_CS);
    r[at(W_CS, 0x45D)] = 0xE8; put16(r, W_CS, 0x45Eu, 0xFBA0u); r[at(W_CS, 0x4AD)] = 0xE8; put16(r, W_CS, 0x4AEu, 0xFB70u); r[at(W_CS, 0x4E9)] = 0xE8; put16(r, W_CS, 0x4EAu, 0xFB4Eu);
    r[at(W_CS, 0x4F8)] = 0x9A; put16(r, W_CS, 0x4F9u, 0x0B0Bu); put16(r, W_CS, 0x4FBu, 0x6200u); r[at(W_CS, 0x4FD)] = 0x9A; put16(r, W_CS, 0x4FEu, 0x1693u); put16(r, W_CS, 0x500u, 0x8200u);
    return c;
}
static pr_bus flat_bus(uint8_t *r) { pr_bus b; memset(&b, 0, sizeof b); b.ram = r; return b; }
static const pr_u7f_044c_limits LIM = {65536, 65536, 65536, 65536, 1000000};

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE);
    pr_u7f_044c_regs c, saved; size_t budget; pr_u7f_044c_diagnostic d; pr_bus bus; pr_ega *ega = malloc(sizeof *ega);
    assert(r && before && ega);
    /* transfer: PW + EXTRA = 0110h <= 0640h. Q's array word 0300h - 10h = 02F0h; P's 0200h + 10h = 0210h; Q's +0102 word goes -10h (+0020) then +10h (+003A): back to 0400h; no display */
    c = setup(r, 0x0010u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_RETURNED);
    assert(get16(r, QSEG, 0x0100u + 0x8Cu) == 0x02F0u && get16(r, PSEG, 0x0100u + 0x8Cu) == 0x0210u && get16(r, QSEG, 0x0100u + 0x102u) == 0x0400u && get16(r, PSEG, 0x0100u + 0x102u) == 0x0100u);
    assert(c.cs == 0x8000u && c.ip == 0x9000u && c.sp == S0 + 0x10u && c.bp == 0x7777u && d.callee == 2);
    /* the pointer chain goes through the bus: the PP cell in the aperture, plane 0 holding the pointer to P (0100h, 3000h) while the RAM bytes there are the 0A5h fill: the same transfer happens */
    c = setup(r, 0x0010u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0xA000u, 0x0100u); pr_ega_reset(ega); ega->plane[0][0x100] = 0x00; ega->plane[0][0x101] = 0x01; ega->plane[0][0x102] = 0x00; ega->plane[0][0x103] = 0x30;
    memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 100000; put16(r, 0xA000u, 0x0100u, 0xA5A5u); put16(r, 0xA000u, 0x0102u, 0xA5A5u);
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_RETURNED);
    assert(get16(r, QSEG, 0x0100u + 0x8Cu) == 0x02F0u && get16(r, PSEG, 0x0100u + 0x8Cu) == 0x0210u);
    /* the two words pushed for +0000 at 0455/0459 come from the PP cell through the bus (plane 0), not from its 0A5h RAM bytes: stop after the 7th instruction (LIMIT at 045D, BP = 7FFEh) and read them off the stack */
    c = setup(r, 0x0010u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0xA000u, 0x0100u); pr_ega_reset(ega); ega->plane[0][0x100] = 0x00; ega->plane[0][0x101] = 0x01; ega->plane[0][0x102] = 0x00; ega->plane[0][0x103] = 0x30;
    memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 7; put16(r, 0xA000u, 0x0100u, 0xA5A5u); put16(r, 0xA000u, 0x0102u, 0xA5A5u);
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_LIMIT && c.ip == 0x045Du);
    assert(get16(r, SS_, 0x7FF0u) == PSEG && get16(r, SS_, 0x7FEEu) == 0x0100u);
    /* the array word is read and written through the bus: P at A000:0000 (class byte read flat from RAM), its +0102 word and array word in the planes: plane 0 holds 0200h at +8Ch; the add leaves 0210h in all four planes */
    c = setup(r, 0x0010u, 0xA000u, 0x0000u, 0x0000u, 0x0000u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); pr_ega_reset(ega); ega->plane[0][0x8C] = 0x00; ega->plane[0][0x8D] = 0x02;
    memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 100000;
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_RETURNED);
    { unsigned p; for (p = 0; p < 4; ++p) assert(ega->plane[p][0x8C] == 0x10u && ega->plane[p][0x8D] == 0x02u); }
    assert(r[0xA008C] == 0x00u && r[0xA008D] == 0x00u && get16(r, QSEG, 0x0100u + 0x8Cu) == 0x02F0u);
    /* without an EGA the same state is flat RAM */
    c = setup(r, 0x0010u, 0xA000u, 0x0000u, 0x0000u, 0x0200u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_RETURNED); assert(get16(r, 0xA000u, 0x008Cu) == 0x0210u);
    /* the budget: 0 changes nothing; 7 stops before the first call (7 instructions precede 045D) */
    c = setup(r, 0x0010u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); saved = c; memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 0;
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_LIMIT); assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE));
    c = setup(r, 0x0010u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); bus = flat_bus(r); budget = 7;
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_LIMIT); assert(c.cs == W_CS && c.ip == 0x045Du && budget == 0 && d.callee == -1);
    /* a corrupted call site stops CODE before the return frame; a corrupted far call inside +0000 is a CALLEE stop that names +0000 */
    c = setup(r, 0x0010u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); r[at(W_CS, 0x45Du)] = 0x9Au; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_CODE); assert(c.ip == 0x045Du && d.site == 0x045Du);
    c = setup(r, 0x0010u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); r[at(W_CS, 0x4E9u)] = 0x9Au; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_CODE); assert(d.site == 0x04E9u);
    c = setup(r, 0x0010u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); r[at(W_CS, 0x0Cu)] = 0xE8u; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_CALLEE); assert(d.callee == 0);
    /* the message branch: EXTRA 7000h pushes PW + EXTRA above 0640h; the far call to StrLoad is checked before its return frame */
    c = setup(r, 0x7000u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); r[at(W_CS, 0x4F8u)] = 0xE8u; bus = flat_bus(r); budget = 100000;
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_CODE); assert(d.site == 0x04F8u && c.ip == 0x04F8u);
    assert(get16(r, QSEG, 0x0100u + 0x8Cu) == 0x0300u && get16(r, PSEG, 0x0100u + 0x8Cu) == 0x0200u);      /* the arrays are untouched on that branch */
    /* argument rejection is atomic */
    c = setup(r, 0x0010u, PSEG, 0x0100u, 0x0100u, 0x0200u, 0x0300u, 0x0400u, 0x3800u, 0x0000u); memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 100000;
    c.ip = 0x044Du; saved = c; assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_ARGUMENT);
    c.ip = 0x044Cu; c.sp = 0x01FFu; saved = c; assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_ARGUMENT);
    c.sp = 0xFFEFu; saved = c; assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE) && budget == 100000u); c.sp = S0;
    assert(pr_u7f_044c_cpu(NULL, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U7F_044C_ARGUMENT);
    assert(pr_u7f_044c_cpu(&bus, 0, &c, &budget, &LIM, &d) == PR_U7F_044C_ARGUMENT);
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, NULL, &budget, &LIM, &d) == PR_U7F_044C_ARGUMENT);
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, NULL, &LIM, &d) == PR_U7F_044C_ARGUMENT);
    assert(pr_u7f_044c_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, NULL, &d) == PR_U7F_044C_ARGUMENT);
    free(ega); free(before); free(r); puts("U7F044C synthetic, bus and guard cases passed"); return 0;
}
