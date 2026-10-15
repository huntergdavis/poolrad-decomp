#include "poolrad/recovered/overlay_u3c2_entry_prefix_0909.h"

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6C_3F31_RAM_SIZE - 1u);
}

pr_u3c2_0909_prefix_status pr_u3c2_entry_prefix_0909(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu)
{
    size_t at;
    uint16_t before, result, status = 0;
    uint8_t parity;
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE ||
        cpu->ip != PR_U3C2_0909_PREFIX_ENTRY)
        return PR_U3C2_0909_PREFIX_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & (PR_U6C_3F31_RAM_SIZE - 1u)] = (uint8_t)(cpu->bp >> 8u);
    cpu->bp = cpu->sp;
    before = cpu->sp;
    result = (uint16_t)(before - 0x20u);
    if (before < 0x20u) status |= 0x0001u;
    if (((before ^ 0x20u ^ result) & 0x10u) != 0) status |= 0x0010u;
    if (result == 0) status |= 0x0040u;
    if (result & 0x8000u) status |= 0x0080u;
    if (((before ^ 0x20u) & (before ^ result) & 0x8000u) != 0) status |= 0x0800u;
    parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if (!(parity & 1u)) status |= 0x0004u;
    cpu->sp = result;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08D5u) | status);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 3u))] = 0;
    cpu->ip = PR_U3C2_0909_PREFIX_STOP;
    return PR_U3C2_0909_PREFIX_OK;
}
