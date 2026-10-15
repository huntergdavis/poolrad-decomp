/* DOS v1.3 GAME.OVR U3B0 entry4 +0DC7..+0DCE, 8 original bytes. */
#include "poolrad/recovered/overlay_u3b0_entry4_node_gate_0dc7.h"

#define MASK (PR_U3B0_0DC7_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

pr_u3b0_0dc7_status pr_u3b0_entry4_node_gate_0dc7(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dc7_cpu *cpu)
{
    uint16_t flags;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0DC7_RAM_SIZE ||
        cpu->ip != 0x0DC7u)
        return PR_U3B0_0DC7_ARGUMENT;

    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x14u));
    cpu->ax = (uint16_t)(cpu->ax |
                         read16(ram, cpu->ss,
                                (uint16_t)(cpu->bp + 0x16u)));
    flags = cpu->flags & (uint16_t)~ARITH_FLAGS;
    if (even_parity((uint8_t)cpu->ax)) flags |= 0x0004u;
    if (cpu->ax == 0u) flags |= 0x0040u;
    if ((cpu->ax & 0x8000u) != 0u) flags |= 0x0080u;
    cpu->flags = flags;
    cpu->ip = cpu->ax != 0u ? 0x0DEAu : 0x0DCFu;
    return PR_U3B0_0DC7_OK;
}
