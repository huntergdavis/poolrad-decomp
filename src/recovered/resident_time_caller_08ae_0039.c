#include "poolrad/recovered/resident_time_caller_08ae_0039.h"

#include <stdint.h>

enum {
    FLAG_CF = 0x0001u, FLAG_PF = 0x0004u, FLAG_AF = 0x0010u,
    FLAG_ZF = 0x0040u, FLAG_SF = 0x0080u, FLAG_OF = 0x0800u,
    ARITH_FLAGS = FLAG_CF | FLAG_PF | FLAG_AF | FLAG_ZF | FLAG_SF | FLAG_OF
};

static uint16_t arithmetic_flags(uint16_t left, uint16_t right,
                                 uint32_t sum)
{
    const uint16_t result = (uint16_t)sum;
    uint8_t low = (uint8_t)result;
    unsigned parity = 0u;
    unsigned bit;
    uint16_t flags = 0u;

    for (bit = 0u; bit < 8u; bit++) {
        parity ^= (unsigned)(low & 1u);
        low >>= 1;
    }
    if (sum > 0xffffu) flags |= FLAG_CF;
    if (parity == 0u) flags |= FLAG_PF;
    if (((left ^ right ^ result) & 0x0010u) != 0u) flags |= FLAG_AF;
    if (result == 0u) flags |= FLAG_ZF;
    if ((result & 0x8000u) != 0u) flags |= FLAG_SF;
    if (((~(left ^ right) & (left ^ result)) & 0x8000u) != 0u)
        flags |= FLAG_OF;
    return flags;
}

void pr_rec_res_08ae_0039_accumulate(pr_rec_res_08ae_001f_cpu *cpu)
{
    const uint32_t low_sum = (uint32_t)cpu->ax + cpu->cx;
    const uint32_t high_sum = (uint32_t)cpu->bx +
                              (uint32_t)(low_sum > 0xffffu);
    const uint16_t final_flags = arithmetic_flags(0u, cpu->bx, high_sum);

    /* XOR DX,DX clears carry; ADD AX,CX supplies carry to ADC DX,BX.
     * The final ADC defines every arithmetic flag, including AF, so the
     * undefined AF from XOR is not observable at this gate's exit. */
    cpu->ax = (uint16_t)low_sum;
    cpu->dx = (uint16_t)high_sum;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | final_flags);
    cpu->ip = 0x003fu;
}
