/* Unit 86:+323D..+3289; docs/re/U86_RECORD_PREDICATE_323D.md. */
#include "poolrad/recovered/overlay_u86_record_predicate_323d.h"

#define MASK (PR_U86_323D_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void parity(pr_u86_323d_cpu *cpu, uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    if (!(value & 1u)) cpu->flags |= 4u;
}

static void or16(pr_u86_323d_cpu *cpu, uint16_t value)
{
    cpu->ax = (uint16_t)(cpu->ax | value);
    cpu->flags &= (uint16_t)~ARITH_FLAGS; /* undefined AF normalized to zero */
    parity(cpu, (uint8_t)cpu->ax);
    if (!cpu->ax) cpu->flags |= 0x40u;
    if (cpu->ax & 0x8000u) cpu->flags |= 0x80u;
}

static void cmp8(pr_u86_323d_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    parity(cpu, result);
    if (left < right) cpu->flags |= 1u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (!result) cpu->flags |= 0x40u;
    if (result & 0x80u) cpu->flags |= 0x80u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x800u;
}

static uint8_t table_byte(const uint8_t *ram, pr_u86_323d_cpu *cpu)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->ax = ram[physical(cpu->es, (uint16_t)(cpu->di + 0x2Eu))];
    cpu->di = cpu->ax;
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    cpu->di = (uint16_t)(cpu->di << 4);
    return ram[physical(cpu->ds, (uint16_t)(cpu->di + 0x5582u))];
}

pr_u86_323d_status pr_u86_record_predicate_323d(
    uint8_t *ram, size_t ram_size, pr_u86_323d_cpu *cpu)
{
    uint8_t result = 0;
    size_t at;
    if (!ram || !cpu || ram_size < PR_U86_323D_RAM_SIZE)
        return PR_U86_323D_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    or16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    if (cpu->ax) {
        uint8_t value = table_byte(ram, cpu);
        cmp8(cpu, value, 10u);
        if (value > 10u) {
            value = table_byte(ram, cpu);
            cmp8(cpu, value, 14u);
            if (value < 14u) result = 1;
        }
    }
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = result;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
              ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))]);
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U86_323D_OK;
}
