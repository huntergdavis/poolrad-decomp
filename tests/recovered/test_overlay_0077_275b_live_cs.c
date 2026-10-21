/* U77 +275B: the far segment of both resident calls is the loader-relocated word in the caller's code, not a constant.
 * The two resident models are replaced by spies that record the CS they are entered with and return through the frame, so the
 * test observes what a differential against the original cannot (the originals' residents never look at their entry CS).
 * Failing on the old model, which assigned CS := 00BAh whatever the word said, is the point. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "poolrad/recovered/overlay_0077_275b_cpu.h"

#define SIZE PR_U9E_10E4_RAM_SIZE
#define MASK (SIZE - 1u)

static uint8_t ram[SIZE];
static unsigned spied, entry_cs[2], entry_ip[2];

static size_t physical(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & MASK; }
static void put16(uint16_t s, uint16_t o, uint16_t v) { size_t at = physical(s, o); ram[at] = (uint8_t)v; ram[(at + 1) & MASK] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t s, uint16_t o) { size_t at = physical(s, o); return (uint16_t)(ram[at] | (ram[(at + 1) & MASK] << 8)); }

/* RETF 4 through the frame the caller pushed (CS:IP return address, then the 4 argument bytes). */
static pr_res00ba_status spy(pr_res00ba_cpu *c, uint16_t ax)
{
    entry_cs[spied] = c->cs; entry_ip[spied] = c->ip; ++spied;
    c->ax = ax;
    c->ip = get16(c->ss, c->sp); c->cs = get16(c->ss, (uint16_t)(c->sp + 2u)); c->sp = (uint16_t)(c->sp + 4u + 4u);
    return PR_RES00BA_OK;
}
pr_res00ba_status pr_res00ba_0bb8_cpu(uint8_t *r, size_t n, pr_res00ba_cpu *c, size_t *budget) { (void)r; (void)n; (void)budget; return spy(c, 0); }
pr_res00ba_status pr_res00ba_core_cpu(uint8_t *r, size_t n, pr_res00ba_cpu *c, size_t *budget) { (void)r; (void)n; (void)budget; return spy(c, 0); }
pr_res00ba_status pr_res00ba_helpers_cpu(uint8_t *r, size_t n, pr_res00ba_cpu *c, size_t *budget) { (void)r; (void)n; (void)budget; return spy(c, 0x0010); }

static pr_ovl0077_275b_state frame(uint16_t first, uint16_t second)
{
    pr_ovl0077_275b_state c;
    memset(ram, 0, SIZE); memset(&c, 0, sizeof c);
    c.cs = 0x0814; c.ds = 0x1000; c.es = 0x1000; c.ss = 0x2000; c.sp = 0x8000; c.ip = 0x275B; c.flags = 0x0202; c.bp = 0x7F01;
    put16(c.ss, 0x8000, 0x0000); put16(c.ss, 0x8002, 0xDEAD);
    put16(c.ss, 0x8004, 0x0200); put16(c.ss, 0x8006, 0x4000);      /* record */
    put16(c.ss, 0x8008, 0x0100); put16(c.ss, 0x800A, 0x3000);      /* target */
    put16(c.cs, 0x276A, first);                                    /* segment word of the LCALL at 2767 */
    put16(c.cs, 0x27AB, second);                                   /* segment word of the LCALL at 27A8 */
    spied = 0;
    return c;
}

int main(void)
{
    static const uint16_t words[][2] = { {0x00BAu, 0x00BAu}, {0x1234u, 0x5678u}, {0x0001u, 0xFFF0u}, {0x00BAu + 0x1000u, 0x00BAu + 0x1000u} };
    for (unsigned k = 0; k < sizeof words / sizeof words[0]; ++k) {
        pr_ovl0077_275b_state c = frame(words[k][0], words[k][1]);
        size_t budget = 1u << 20;
        pr_ovl0077_275b_diagnostic d;
        assert(pr_ovl0077_275b_cpu(ram, SIZE, &c, &budget, &d) == PR_OVL0077_275B_RETURNED);
        assert(spied == 2);
        assert(entry_cs[0] == words[k][0] && entry_ip[0] == 0x0BB8u);   /* 0BB8 is entered at the word's segment */
        assert(entry_cs[1] == words[k][1] && entry_ip[1] == 0x13ABu);   /* 13AB likewise, from its own word */
        assert(c.cs == 0xDEAD && c.ip == 0x0000 && c.bp == 0x7F01);
    }
    printf("U77 +275B: live segment words passed\n");
    return 0;
}
