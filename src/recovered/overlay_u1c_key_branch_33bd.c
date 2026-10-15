#include "poolrad/recovered/overlay_u1c_key_branch_33bd.h"

enum {
    FLAG_CF = 0x0001u, FLAG_PF = 0x0004u, FLAG_AF = 0x0010u,
    FLAG_ZF = 0x0040u, FLAG_SF = 0x0080u, FLAG_OF = 0x0800u,
    CMP_FLAGS = FLAG_CF | FLAG_PF | FLAG_AF | FLAG_ZF | FLAG_SF | FLAG_OF
};

static uint16_t cmp_word_one_flags(uint16_t old, uint16_t value)
{
    const uint16_t result = (uint16_t)(value - 1u);
    uint8_t low = (uint8_t)result;
    unsigned parity = 0u;
    unsigned bit;
    uint16_t flags = (uint16_t)(old & (uint16_t)~CMP_FLAGS);

    for (bit = 0u; bit < 8u; bit++) {
        parity ^= (unsigned)(low & 1u);
        low >>= 1;
    }
    if (value == 0u) flags |= FLAG_CF;
    if (parity == 0u) flags |= FLAG_PF;
    if (((value ^ 1u ^ result) & 0x0010u) != 0u) flags |= FLAG_AF;
    if (result == 0u) flags |= FLAG_ZF;
    if ((result & 0x8000u) != 0u) flags |= FLAG_SF;
    if (((value ^ 1u) & (value ^ result) & 0x8000u) != 0u)
        flags |= FLAG_OF;
    return flags;
}

int pr_rec_u1c_33bd_branch(pr_rec_u1c_33bd_cpu *cpu,
                            const uint8_t *ram, size_t ram_size)
{
    uint32_t offset;
    uint32_t address;
    uint16_t value;
    pr_rec_u1c_33bd_cpu next;

    if (cpu == NULL || ram == NULL || ram_size != 655360u ||
        cpu->cs != 0x3582u || cpu->ip != 0x33bdu)
        return 0;

    offset = (uint32_t)cpu->di + 0x0594u;
    if (offset + 2u > 0x10000u) return 0;
    address = ((uint32_t)cpu->es << 4) + offset;
    if ((uint64_t)address + 2u > ram_size) return 0;

    value = (uint16_t)((uint16_t)ram[address] |
                       ((uint16_t)ram[address + 1u] << 8));
    next = *cpu;
    next.flags = cmp_word_one_flags(next.flags, value);
    next.ip = value > 1u ? 0x33d2u : 0x33c5u;
    *cpu = next;
    return 1;
}
