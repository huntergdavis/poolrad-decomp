#include "poolrad/recovered/resident_time_caller_08ae_00aa.h"

#include <stdint.h>

enum {
    FLAG_CF = 0x0001u, FLAG_PF = 0x0004u, FLAG_AF = 0x0010u,
    FLAG_ZF = 0x0040u, FLAG_SF = 0x0080u, FLAG_OF = 0x0800u,
    ARITH_FLAGS = FLAG_CF | FLAG_PF | FLAG_AF | FLAG_ZF | FLAG_SF | FLAG_OF
};

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

void pr_rec_res_08ae_00aa_branch(pr_rec_res_08ae_001f_cpu *cpu)
{
    const uint16_t flags = cpu->flags;
    const unsigned zero = (flags & FLAG_ZF) != 0u;
    const unsigned sign = (flags & FLAG_SF) != 0u;
    const unsigned overflow = (flags & FLAG_OF) != 0u;

    if (!zero && sign == overflow) { /* JG +00B2 */
        cpu->ip = 0x00b2u;
    } else if (sign != overflow) { /* JL +00C4 */
        cpu->ip = 0x00c4u;
    } else {
        /* Equal high WORD: CMP AX,CX, then unsigned JBE +00C4. */
        const uint16_t compared = cmp_flags(cpu->ax, cpu->cx);
        cpu->flags = (uint16_t)((flags & (uint16_t)~ARITH_FLAGS) | compared);
        cpu->ip = (compared & (FLAG_CF | FLAG_ZF)) != 0u ?
                  0x00c4u : 0x00b2u;
    }
}
