/* GAME.OVR U3C2 entry1:+0000..002D; docs/re/U3C2_SIGNUM_0000.md. */
#include "poolrad/recovered/overlay_u3c2_signum_0000.h"
#define MASK (PR_U3C2_0000_RAM_SIZE - 1u)
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
static void compare_zero(pr_u3c2_0000_cpu *cpu, uint16_t value)
{
    uint8_t parity = (uint8_t)value;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if ((parity & 1u) == 0u) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if ((value & 0x8000u) != 0u) cpu->flags |= 0x80u;
    /* CMP word,0: CF/AF/OF are zero. XOR AX,AX on the zero path has
     * identical defined flags; choose zero for its undefined AF. */
}
pr_u3c2_0000_status pr_u3c2_signum_0000(
    uint8_t *ram, size_t ram_size, pr_u3c2_0000_cpu *cpu)
{
    uint16_t value, result;
    size_t at;
    if (!ram || !cpu || ram_size < PR_U3C2_0000_RAM_SIZE)
        return PR_U3C2_0000_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, physical(cpu->ss, cpu->sp), cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    value = word_at(ram, physical(cpu->ss, (uint16_t)(cpu->bp + 6u)));
    compare_zero(cpu, value);
    if ((value & 0x8000u) != 0u) {
        result = 0xFFFFu;
    } else {
        value = word_at(ram, physical(cpu->ss, (uint16_t)(cpu->bp + 6u)));
        compare_zero(cpu, value);
        if (value != 0u && value < 0x8000u) {
            result = 1u;
        } else {
            cpu->ax = 0;
            compare_zero(cpu, 0); /* Defined XOR flags; undefined AF chosen zero. */
            result = 0;
        }
    }
    at = physical(cpu->ss, (uint16_t)(cpu->bp - 2u));
    put_word(ram, at, result);
    cpu->ax = word_at(ram, at);
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_U3C2_0000_OK;
}
