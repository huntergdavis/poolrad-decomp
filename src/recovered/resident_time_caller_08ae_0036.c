#include "poolrad/recovered/resident_time_caller_08ae_0036.h"

static uint16_t even_parity_32(uint32_t value)
{
    unsigned odd = 0u;
    unsigned i;
    for (i = 0u; i < 32u; i++) {
        odd ^= value & 1u;
        value >>= 1;
    }
    return odd == 0u ? 0x0004u : 0u;
}

void pr_rec_res_08ae_0036_mul_local(pr_rec_res_08ae_001f_cpu *cpu,
                                    const uint8_t operand[2])
{
    const uint16_t factor = (uint16_t)((uint16_t)operand[0] |
                                       ((uint16_t)operand[1] << 8));
    const uint32_t product = (uint32_t)cpu->ax * factor;
    uint16_t flags = (uint16_t)(cpu->flags & (uint16_t)~0x0845u);

    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    /* CF/OF are defined. DOSBox-X also sets ZF from AX and PF from
     * parity over DX:AX; the instruction leaves those flags undefined. */
    if (cpu->dx != 0u)
        flags |= 0x0801u;
    if (cpu->ax == 0u)
        flags |= 0x0040u;
    flags |= even_parity_32(product);
    cpu->flags = flags;
    cpu->ip = 0x0039u;
}
