#include "poolrad/recovered/overlay_u38_0cc3_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define U_CS 0x6000u
#define DS_ 0x1000u
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }

/* The record at 3000:0100 has type 3 in its byte +3 high nibble; the key argument is 00011234h; nodes are linked from DS:[6DBB]. */
static pr_u38_0cc3_regs setup(uint8_t *r)
{
    pr_u38_0cc3_regs c; memset(&c, 0, sizeof c); memset(r, 0xA5, PR_BUS_RAM_SIZE);
    c.cs = U_CS; c.ip = 0x0CC3u; c.ds = DS_; c.ss = SS_; c.sp = S0; c.bp = 0x7777; c.ax = 0xABCD; c.flags = 0x0202;
    put16(r, c.ss, c.sp, 0x9000); put16(r, c.ss, (uint16_t)(c.sp + 2u), 0x8000); put16(r, c.ss, (uint16_t)(c.sp + 4u), 0x00AB);
    put16(r, c.ss, (uint16_t)(c.sp + 6u), 0x0100); put16(r, c.ss, (uint16_t)(c.sp + 8u), 0x3000); put16(r, c.ss, (uint16_t)(c.sp + 10u), 0x1234);
    put16(r, c.ss, (uint16_t)(c.sp + 12u), 0x0001); r[at(0x3000u, 0x0103u)] = 0x35u;
    put16(r, DS_, 0x6DBBu, 0); put16(r, DS_, 0x6DBDu, 0);
    r[at(U_CS, 0x0D44u)] = 0x9Au; put16(r, U_CS, 0x0D45u, 0x0B0Bu); put16(r, U_CS, 0x0D47u, 0x7000u);
    r[at(U_CS, 0x0D49u)] = 0x9Au; put16(r, U_CS, 0x0D4Au, 0x1693u); put16(r, U_CS, 0x0D4Cu, 0x8200u);
    r[at(U_CS, 0x0EF7u)] = 0x9Au; put16(r, U_CS, 0x0EF8u, 0x0254u); put16(r, U_CS, 0x0EFAu, 0x7000u);
    return c;
}
static void node(uint8_t *r, uint16_t seg, uint16_t off, uint32_t key, uint8_t type, uint16_t nseg, uint16_t noff)
{
    put16(r, seg, off, (uint16_t)key); put16(r, seg, (uint16_t)(off + 2u), (uint16_t)(key >> 16u));
    put16(r, seg, (uint16_t)(off + 4u), noff); put16(r, seg, (uint16_t)(off + 6u), nseg); r[at(seg, (uint16_t)(off + 0x12u))] = type;
}
static pr_bus flat_bus(uint8_t *r) { pr_bus b; memset(&b, 0, sizeof b); b.ram = r; return b; }
static const pr_u38_0cc3_limits LIM = {65536, 65536, 65536, 65536, 1000000, 65536};

int main(void)
{
    uint8_t *r = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE);
    pr_u38_0cc3_regs c, saved; size_t budget; pr_u38_0cc3_diagnostic d; pr_bus bus; pr_ega *ega = malloc(sizeof *ega);
    assert(r && before && ega);
    /* an empty list: the body returns at once (RETF 0Ah pops five argument words); the 40-byte temporary is never loaded */
    c = setup(r); bus = flat_bus(r); budget = 100000;
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_RETURNED);
    assert(c.cs == 0x8000u && c.ip == 0x9000u && c.sp == S0 + 14u && c.bp == 0x7777u && d.callee == -1);
    /* two nodes, neither matching (a key, then the right key with the wrong type): the walk ends at the null link and returns */
    c = setup(r); node(r, 0x3101u, 0, 0x00011235u, 3, 0x3103u, 0); node(r, 0x3103u, 0, 0x00011234u, 4, 0, 0);
    put16(r, DS_, 0x6DBBu, 0); put16(r, DS_, 0x6DBDu, 0x3101u); bus = flat_bus(r); budget = 100000;
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_RETURNED && d.callee == -1);
    /* a match: the body reaches the message; with the StrLoad call corrupted it stops CODE at 0D44 before the return frame */
    c = setup(r); node(r, 0x3101u, 0, 0x00011235u, 3, 0x3103u, 0); node(r, 0x3103u, 0, 0x00011234u, 3, 0, 0);
    put16(r, DS_, 0x6DBDu, 0x3101u); r[at(U_CS, 0x0D44u)] = 0xE8u; bus = flat_bus(r); budget = 100000;
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_CODE && c.ip == 0x0D44u && d.site == 0x0D44u);
    /* the list nodes are read THROUGH THE BUS: a node in the aperture whose key is in plane 0 only matches with the EGA attached */
    c = setup(r); pr_ega_reset(ega); memset(&ega->plane[0][0x0100], 0, 0x20);
    ega->plane[0][0x0100] = 0x34u; ega->plane[0][0x0101] = 0x12u; ega->plane[0][0x0102] = 0x01u; ega->plane[0][0x0112] = 3u;
    put16(r, DS_, 0x6DBBu, 0x0100u); put16(r, DS_, 0x6DBDu, 0xA000u); r[at(U_CS, 0x0D44u)] = 0xE8u;
    memset(&bus, 0, sizeof bus); bus.ram = r; bus.ega = ega; budget = 100000;
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_CODE && c.ip == 0x0D44u);
    c = setup(r); put16(r, DS_, 0x6DBBu, 0x0100u); put16(r, DS_, 0x6DBDu, 0xA000u); memset(&r[0xA0100], 0, 0x20); bus = flat_bus(r); budget = 100000;
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_RETURNED);   /* flat RAM there holds no key */
    /* a cyclic list with no match never returns: the budget stops it at the next unexecuted instruction */
    c = setup(r); node(r, 0x3101u, 0, 0x00099999u, 3, 0x3101u, 0); put16(r, DS_, 0x6DBDu, 0x3101u); bus = flat_bus(r); budget = 500;
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_LIMIT && budget == 0 && c.cs == U_CS);
    assert(c.ip >= 0x0CEAu && c.ip < 0x0D31u);
    /* budget 0 changes nothing */
    c = setup(r); saved = c; memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 0;
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_LIMIT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE));
    /* argument rejection is atomic */
    c = setup(r); memcpy(before, r, PR_BUS_RAM_SIZE); bus = flat_bus(r); budget = 100000;
    c.ip = 0x0CC4u; saved = c; assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_ARGUMENT);
    c.ip = 0x0CC3u; c.sp = 0x01FFu; saved = c; assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_ARGUMENT);
    c.sp = 0xFFF3u; saved = c; assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_ARGUMENT);
    assert(!memcmp(&c, &saved, sizeof c) && !memcmp(r, before, PR_BUS_RAM_SIZE) && budget == 100000u); c.sp = S0;
    assert(pr_u38_0cc3_cpu(NULL, PR_BUS_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U38_0CC3_ARGUMENT);
    assert(pr_u38_0cc3_cpu(&bus, 0, &c, &budget, &LIM, &d) == PR_U38_0CC3_ARGUMENT);
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, NULL, &budget, &LIM, &d) == PR_U38_0CC3_ARGUMENT);
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, NULL, &LIM, &d) == PR_U38_0CC3_ARGUMENT);
    assert(pr_u38_0cc3_cpu(&bus, PR_BUS_RAM_SIZE, &c, &budget, NULL, &d) == PR_U38_0CC3_ARGUMENT);
    free(ega); free(before); free(r); puts("U38 0CC3 synthetic, bus and guard cases passed"); return 0;
}
