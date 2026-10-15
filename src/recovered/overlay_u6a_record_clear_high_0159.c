/* Unit 6A:+0159..+019B; docs/re/U6A_RECORD_CLEAR_HIGH_0159.md. */
#include "poolrad/recovered/overlay_u6a_record_clear_high_0159.h"

#define MASK (PR_U6A_0159_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t index_byte(const uint8_t *ram, const pr_u6a_0159_cpu *cpu)
{
    return ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))];
}

static void load_record(const uint8_t *ram, pr_u6a_0159_cpu *cpu)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
}

static void compare_index(pr_u6a_0159_cpu *cpu, uint8_t left)
{
    uint8_t result = (uint8_t)(left - 20u);
    uint8_t parity = result;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < 20u) cpu->flags |= 0x0001u;
    if ((parity & 1u) == 0u) cpu->flags |= 0x0004u;
    if (((left ^ 20u ^ result) & 0x10u) != 0u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if ((result & 0x80u) != 0u) cpu->flags |= 0x0080u;
    if (((left ^ 20u) & (left ^ result) & 0x80u) != 0u) cpu->flags |= 0x0800u;
}

pr_u6a_0159_status pr_u6a_record_clear_high_0159(
    uint8_t *ram, size_t ram_size, pr_u6a_0159_cpu *cpu)
{
    size_t at, local;
    if (!ram || !cpu || ram_size < PR_U6A_0159_RAM_SIZE)
        return PR_U6A_0159_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & MASK] = (uint8_t)(cpu->bp >> 8u);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    local = physical(cpu->ss, (uint16_t)(cpu->bp - 1u));
    ram[local] = 0;
    for (;;) {
        cpu->ax = index_byte(ram, cpu); /* MOV AL; XOR AH,AH. */
        load_record(ram, cpu);
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        at = physical(cpu->es, (uint16_t)(cpu->di + 0x17u));
        if (ram[at] > 0x7Fu) {
            cpu->ax = index_byte(ram, cpu);
            load_record(ram, cpu);
            cpu->di = (uint16_t)(cpu->di + cpu->ax);
            ram[physical(cpu->es, (uint16_t)(cpu->di + 0x17u))] = 0;
        }
        compare_index(cpu, index_byte(ram, cpu));
        if (index_byte(ram, cpu) == 20u)
            break;
        ram[local] = (uint8_t)(ram[local] + 1u);
    }
    /* The final CMP defines all arithmetic flags; subsequent ops are MOV,
     * LES, POP and RETF. Earlier flags cannot affect the returned state. */
    load_record(ram, cpu);
    ram[physical(cpu->es, (uint16_t)(cpu->di + 0x2Cu))] = 0;
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U6A_0159_OK;
}
