/* Declared substitution for the native DOSBox-X run of U23 +0239 (docs/re/OVL_0023_0239_CPU.md).
 * The 275B model hard-codes the resident segment 00BA, which is DOS's own memory under DOS, so the capacity check cannot run
 * natively. On both sides it is replaced by this stub: BX := SP; ES:DI := the ITEM far pointer at SS:[BX+4]; AL := ES:[DI+3Ah];
 * RETF 8. The DOS side runs these same instructions from U77:275B; the C side links this function in place of the model.
 * Everything else (GetMem, Move, StrLoad, 1693 with the EGA, 0BB8) is the real closure. */
#include "poolrad/recovered/overlay_0077_275b_cpu.h"
#define MASK (PR_U9E_10E4_RAM_SIZE - 1u)
static size_t physical(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & MASK; }
static uint16_t word(const uint8_t *r, uint16_t s, uint16_t o) { size_t p = physical(s, o); return (uint16_t)(r[p] | ((uint16_t)r[(p + 1) & MASK] << 8)); }
pr_ovl0077_275b_status pr_ovl0077_275b_cpu(uint8_t *ram, size_t ram_size, pr_ovl0077_275b_state *c, size_t *budget,
                                          pr_ovl0077_275b_diagnostic *diag)
{
    (void)budget;
    if (ram == NULL || c == NULL || ram_size != PR_U9E_10E4_RAM_SIZE || c->ip != 0x275Bu)
        return PR_OVL0077_275B_ARGUMENT;
    if (diag != NULL) { diag->first_status = -1; diag->second_status = -1; diag->calls = 0u; }
    c->bx = c->sp;
    c->di = word(ram, c->ss, (uint16_t)(c->bx + 4u));
    c->es = word(ram, c->ss, (uint16_t)(c->bx + 6u));
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->es, (uint16_t)(c->di + 0x3Au))]);
    c->ip = word(ram, c->ss, c->sp);
    c->cs = word(ram, c->ss, (uint16_t)(c->sp + 2u));
    c->sp = (uint16_t)(c->sp + 12u);
    return PR_OVL0077_275B_RETURNED;
}
