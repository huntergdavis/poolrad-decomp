/* GAME.OVR U7F +0FE3..+100F; docs/re/U7F_ROLL_RETURN_0FE3.md. */
#include "poolrad/recovered/overlay_u7f_roll_return_0fe3.h"

#define MASK (PR_U7F_0FE3_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t cmp8_flags(uint16_t incoming, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    uint8_t parity = result;
    uint16_t flags = (uint16_t)(incoming & (uint16_t)~0x08D5u);
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (left < right) flags |= 0x0001u;
    if (!(parity & 1u)) flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x80u) flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) flags |= 0x0800u;
    return flags;
}

pr_u7f_0fe3_status pr_u7f_roll_return_0fe3(
    uint8_t *ram, size_t ram_size, pr_u7f_0fe3_cpu *cpu)
{
    uint8_t roll, tier;
    if (ram == NULL || cpu == NULL || ram_size != PR_U7F_0FE3_RAM_SIZE ||
        cpu->ip != 0x0FE3u)
        return PR_U7F_0FE3_ARGUMENT;

    roll = (uint8_t)cpu->ax;
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 2u))] = roll;
    roll = ram[physical(cpu->ss, (uint16_t)(cpu->bp - 2u))];
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | roll);
    cpu->flags = cmp8_flags(cpu->flags, roll, 1u);
    if (roll >= 1u) {
        cpu->flags = cmp8_flags(cpu->flags, roll, 14u);
        if (roll <= 14u) {
            tier = 1u;
            goto return_tier;
        }
    }
    cpu->flags = cmp8_flags(cpu->flags, roll, 15u);
    if (roll >= 15u) {
        cpu->flags = cmp8_flags(cpu->flags, roll, 19u);
        if (roll <= 19u) {
            tier = 2u;
            goto return_tier;
        }
    }
    tier = 3u;

return_tier:
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = tier;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
        ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))]);
    cpu->sp = cpu->bp;
    cpu->bp = word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = word(ram, cpu->ss, cpu->sp);
    cpu->cs = word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U7F_0FE3_RETURNED;
}
