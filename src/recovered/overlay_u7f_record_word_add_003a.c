/* GAME.OVR U7F:+003A..0053; see docs/re/U7F_RECORD_WORD_ADD_003A.md. */
#include "poolrad/recovered/overlay_u7f_record_word_add_003a.h"
#define MASK (PR_U7F_003A_RAM_SIZE - 1u)
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
static void load_record(const uint8_t *ram, pr_u7f_003a_cpu *cpu)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
}
pr_u7f_003a_status pr_u7f_record_word_add_003a(
    uint8_t *ram, size_t ram_size, pr_u7f_003a_cpu *cpu)
{
    uint16_t left, right;
    uint32_t sum;
    uint8_t parity;
    size_t at;
    if (!ram || !cpu || ram_size < PR_U7F_003A_RAM_SIZE)
        return PR_U7F_003A_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, physical(cpu->ss, cpu->sp), cpu->bp);
    cpu->bp = cpu->sp;
    load_record(ram, cpu);
    cpu->ax = word_at(ram, physical(cpu->es, (uint16_t)(cpu->di + 0x102u)));
    left = cpu->ax;
    right = word_at(ram, physical(cpu->ss, (uint16_t)(cpu->bp + 6u)));
    sum = (uint32_t)left + right;
    cpu->ax = (uint16_t)sum;
    parity = (uint8_t)cpu->ax;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (sum > 0xFFFFu) cpu->flags |= 1u;
    if ((parity & 1u) == 0u) cpu->flags |= 4u;
    if (((left ^ right ^ cpu->ax) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (cpu->ax == 0u) cpu->flags |= 0x40u;
    if ((cpu->ax & 0x8000u) != 0u) cpu->flags |= 0x80u;
    if (((uint16_t)~(left ^ right) & (left ^ cpu->ax) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    load_record(ram, cpu);
    put_word(ram, physical(cpu->es, (uint16_t)(cpu->di + 0x102u)), cpu->ax);
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U7F_003A_OK;
}
