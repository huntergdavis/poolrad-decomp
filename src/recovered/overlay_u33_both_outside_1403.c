/* GAME.OVR U33:+1403..1433; docs/re/U33_BOTH_OUTSIDE_1403.md. */
#include "poolrad/recovered/overlay_u33_both_outside_1403.h"
#define MASK (PR_U33_1403_RAM_SIZE - 1u)
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}
static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8u);
}
static void compare_byte(pr_u33_1403_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    uint8_t parity = result;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 1u;
    if ((parity & 1u) == 0u) cpu->flags |= 4u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & 0x80u) != 0u) cpu->flags |= 0x80u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u) cpu->flags |= 0x800u;
}
pr_u33_1403_status pr_u33_both_outside_1403(
    uint8_t *ram, size_t ram_size, pr_u33_1403_cpu *cpu)
{
    uint8_t value, result = 0;
    size_t at;
    if (!ram || !cpu || ram_size < PR_U33_1403_RAM_SIZE)
        return PR_U33_1403_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, physical(cpu->ss, cpu->sp), cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    value = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))];
    compare_byte(cpu, value, 0);
    if (value < 0x80u) {
        value = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))];
        compare_byte(cpu, value, 11);
        if (value < 11u) goto store;
    }
    value = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))];
    compare_byte(cpu, value, 0);
    if (value >= 0x80u) {
        result = 1;
    } else {
        value = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))];
        compare_byte(cpu, value, 6);
        if (value >= 6u) result = 1;
    }
store:
    at = physical(cpu->ss, (uint16_t)(cpu->bp - 1u));
    ram[at] = result;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[at]);
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    /* RETF wraps SP between its IP and CS word pops. */
    cpu->cs = word_at(ram, physical(cpu->ss, (uint16_t)(cpu->sp + 2u)));
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U33_1403_OK;
}
