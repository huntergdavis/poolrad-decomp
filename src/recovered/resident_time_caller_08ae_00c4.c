#include "poolrad/recovered/resident_time_caller_08ae_00c4.h"

#include <stdint.h>

enum {
    FLAG_CF = 0x0001u, FLAG_PF = 0x0004u, FLAG_AF = 0x0010u,
    FLAG_ZF = 0x0040u, FLAG_SF = 0x0080u, FLAG_OF = 0x0800u,
    ARITH_FLAGS = FLAG_CF | FLAG_PF | FLAG_AF | FLAG_ZF | FLAG_SF | FLAG_OF
};

static uint16_t word_at(const uint8_t bytes[8], unsigned offset)
{
    return (uint16_t)((uint16_t)bytes[offset] |
                      ((uint16_t)bytes[offset + 1u] << 8));
}

static uint16_t cmp_flags(uint16_t left, uint16_t right)
{
    const uint16_t result = (uint16_t)(left - right);
    uint8_t low = (uint8_t)result;
    unsigned odd = 0u;
    unsigned bit;
    uint16_t flags = 0u;

    for (bit = 0u; bit < 8u; bit++) {
        odd ^= (unsigned)(low & 1u);
        low >>= 1;
    }
    if (left < right) flags |= FLAG_CF;
    if (odd == 0u) flags |= FLAG_PF;
    if (((left ^ right ^ result) & 0x0010u) != 0u) flags |= FLAG_AF;
    if (result == 0u) flags |= FLAG_ZF;
    if ((result & 0x8000u) != 0u) flags |= FLAG_SF;
    if ((((left ^ right) & (left ^ result)) & 0x8000u) != 0u)
        flags |= FLAG_OF;
    return flags;
}

void pr_rec_res_08ae_00c4_branch(pr_rec_res_08ae_001f_cpu *cpu,
                                 const uint8_t locals[8])
{
    const uint16_t preserved = (uint16_t)(cpu->flags & (uint16_t)~ARITH_FLAGS);
    const uint16_t upper = word_at(locals, 6u);
    uint16_t compared;
    unsigned sign, overflow, zero;

    cpu->ax = word_at(locals, 0u);
    cpu->dx = word_at(locals, 2u);
    compared = cmp_flags(cpu->dx, upper);
    cpu->flags = (uint16_t)(preserved | compared);
    sign = (compared & FLAG_SF) != 0u;
    overflow = (compared & FLAG_OF) != 0u;
    zero = (compared & FLAG_ZF) != 0u;
    if (!zero && sign == overflow) { /* JG +00D6 */
        cpu->ip = 0x00d6u;
    } else if (sign != overflow) { /* JL +0089 */
        cpu->ip = 0x0089u;
    } else {
        /* Equal high WORD: CMP AX,[BP-4], then JB +0089. */
        compared = cmp_flags(cpu->ax, word_at(locals, 4u));
        cpu->flags = (uint16_t)(preserved | compared);
        cpu->ip = (compared & FLAG_CF) != 0u ? 0x0089u : 0x00d6u;
    }
}
