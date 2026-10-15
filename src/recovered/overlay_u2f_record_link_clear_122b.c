/* Unit 2F:+122B..+1279; docs/re/U2F_RECORD_LINK_CLEAR_122B.md. */
#include "poolrad/recovered/overlay_u2f_record_link_clear_122b.h"

#define MASK (PR_U2F_122B_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8u);
}

static void load_far(const uint8_t *ram, pr_u2f_122b_cpu *cpu,
                     uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
}

static void load_record(const uint8_t *ram, pr_u2f_122b_cpu *cpu)
{
    load_far(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
}

static void load_child(const uint8_t *ram, pr_u2f_122b_cpu *cpu)
{
    load_far(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x108u));
}

static uint16_t parity_flag(uint8_t value)
{
    value ^= (uint8_t)(value >> 4u);
    value ^= (uint8_t)(value >> 2u);
    value ^= (uint8_t)(value >> 1u);
    return (value & 1u) == 0u ? 0x0004u : 0u;
}

static void logic_flags(pr_u2f_122b_cpu *cpu)
{
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08D5u) | parity_flag((uint8_t)cpu->ax));
    if (cpu->ax == 0u) cpu->flags |= 0x0040u;
    if ((cpu->ax & 0x8000u) != 0u) cpu->flags |= 0x0080u;
}

static void compare_flags(pr_u2f_122b_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08D5u) | parity_flag(result));
    if (left < right) cpu->flags |= 0x0001u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if ((result & 0x80u) != 0u) cpu->flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u) cpu->flags |= 0x0800u;
}

pr_u2f_122b_status pr_u2f_record_link_clear_122b(
    uint8_t *ram, size_t ram_size, pr_u2f_122b_cpu *cpu)
{
    size_t at;
    uint8_t left, right;
    if (!ram || !cpu || ram_size < PR_U2F_122B_RAM_SIZE)
        return PR_U2F_122B_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, physical(cpu->ss, cpu->sp), cpu->bp);
    cpu->bp = cpu->sp;
    load_record(ram, cpu);
    ram[physical(cpu->es, (uint16_t)(cpu->di + 0x10Fu))] = 1;
    load_record(ram, cpu);
    load_child(ram, cpu);
    cpu->ax = word_at(ram, physical(cpu->es, (uint16_t)(cpu->di + 0x0Au)));
    cpu->ax |= word_at(ram, physical(cpu->es, (uint16_t)(cpu->di + 0x0Cu)));
    logic_flags(cpu);
    if (cpu->ax != 0u) {
        load_record(ram, cpu);
        load_child(ram, cpu);
        load_far(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x0Au));
        left = ram[physical(cpu->es, (uint16_t)(cpu->di + 0x10Eu))];
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | left);
        load_record(ram, cpu);
        right = ram[physical(cpu->es, (uint16_t)(cpu->di + 0x10Eu))];
        compare_flags(cpu, left, right);
        if (left == right) {
            load_record(ram, cpu);
            load_child(ram, cpu);
            cpu->ax = 0;
            logic_flags(cpu);
            put_word(ram, physical(cpu->es, (uint16_t)(cpu->di + 0x0Au)), cpu->ax);
            put_word(ram, physical(cpu->es, (uint16_t)(cpu->di + 0x0Cu)), cpu->ax);
        }
    }
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U2F_122B_OK;
}
