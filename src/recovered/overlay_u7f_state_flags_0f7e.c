/* GAME.OVR unit 0x7F:+0F7E..+0FD1; docs/re/U7F_STATE_FLAGS_0F7E.md. */
#include "poolrad/recovered/overlay_u7f_state_flags_0f7e.h"
#define MASK (PR_U7F_0F7E_RAM_SIZE - 1u)
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}
static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}
static uint16_t word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return word_at(ram, physical(segment, offset));
}
static void put(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8u);
}
static void les_argument(const uint8_t *ram, pr_u7f_0f7e_cpu *cpu, uint16_t displacement)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp + displacement));
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
}
static void logical_flags(pr_u7f_0f7e_cpu *cpu)
{
    uint8_t parity = (uint8_t)cpu->ax;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if ((parity & 1u) == 0u) cpu->flags |= 4u;
    if (cpu->ax == 0u) cpu->flags |= 0x40u;
    if (cpu->ax & 0x8000u) cpu->flags |= 0x80u;
}

pr_u7f_0f7e_status pr_u7f_state_flags_0f7e(
    uint8_t *ram, size_t ram_size, pr_u7f_0f7e_cpu *cpu)
{
    size_t at, index_at;
    if (ram == NULL || cpu == NULL || ram_size != PR_U7F_0F7E_RAM_SIZE)
        return PR_U7F_0F7E_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    les_argument(ram, cpu, 10u);
    ram[physical(cpu->es, cpu->di)] = 0u;
    les_argument(ram, cpu, 6u);
    ram[physical(cpu->es, cpu->di)] = 0u;
    index_at = physical(cpu->ss, (uint16_t)(cpu->bp - 1u));
    ram[index_at] = 0u;
    for (;;) {
        uint8_t index = ram[index_at];
        cpu->ax = index < 0x80u ? index : (uint16_t)(0xFF00u | index);
        cpu->di = (uint16_t)(cpu->ax << 2u);
        cpu->ax = word(ram, cpu->ds, (uint16_t)(cpu->di + 0x67F4u));
        cpu->ax = (uint16_t)(cpu->ax | word(ram, cpu->ds, (uint16_t)(cpu->di + 0x67F6u)));
        if (cpu->ax != 0u) {
            les_argument(ram, cpu, 10u);
            ram[physical(cpu->es, cpu->di)] = 1u;
        }
        if (ram[index_at] == 6u) break;
        ram[index_at] = (uint8_t)(ram[index_at] + 1u);
    }
    cpu->ax = word(ram, cpu->ds, 0x6810u);
    cpu->ax = (uint16_t)(cpu->ax | word(ram, cpu->ds, 0x6812u));
    logical_flags(cpu);
    if (cpu->ax != 0u) {
        les_argument(ram, cpu, 6u);
        ram[physical(cpu->es, cpu->di)] = 1u;
    }
    cpu->sp = cpu->bp;
    cpu->bp = word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 12u);
    return PR_U7F_0F7E_OK;
}
