/* Loaded resident cold-loader +0338 to its first reached nested CALL.
 * Evidence: docs/re/RESIDENT_LOADER_PREFIX_0338.md. */
#include "poolrad/recovered/resident_loader_prefix_0338.h"

#define RAM_MASK (PR_HANDLER_02E6_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return (uint16_t)(ram[physical(segment, offset)] |
                      ((uint16_t)ram[physical(segment,
                                              (uint16_t)(offset + 1u))] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    ram[physical(segment, offset)] = (uint8_t)value;
    ram[physical(segment, (uint16_t)(offset + 1u))] = (uint8_t)(value >> 8);
}

static unsigned even_parity(uint8_t value)
{
    unsigned bits = 0u;
    while (value != 0u) {
        bits ^= value & 1u;
        value >>= 1u;
    }
    return bits == 0u;
}

static void set_inc16_flags(pr_handler_02e6_cpu *cpu, uint16_t before,
                            uint16_t result)
{
    uint16_t flags = (uint16_t)(cpu->flags & ~(ARITH_FLAGS & ~0x0001u));
    if (even_parity((uint8_t)result))
        flags |= 0x0004u;
    if ((before & 0x000Fu) == 0x000Fu)
        flags |= 0x0010u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    if (before == 0x7FFFu)
        flags |= 0x0800u;
    cpu->flags = flags;
}

static void set_cmp_zero_flags(pr_handler_02e6_cpu *cpu, uint16_t value)
{
    uint16_t flags = (uint16_t)(cpu->flags & ~ARITH_FLAGS);
    if (even_parity((uint8_t)value))
        flags |= 0x0004u;
    if (value == 0u)
        flags |= 0x0040u;
    if ((value & 0x8000u) != 0u)
        flags |= 0x0080u;
    cpu->flags = flags;
}

pr_loader_0338_status pr_resident_loader_prefix_0338(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu)
{
    uint16_t before, result, field;

    if (ram == NULL || ram_size < PR_HANDLER_02E6_RAM_SIZE || cpu == NULL)
        return PR_LOADER_0338_ARGUMENT;

    before = read16(ram, cpu->ds, 0x43E0u);
    result = (uint16_t)(before + 1u);
    write16(ram, cpu->ds, 0x43E0u, result);
    set_inc16_flags(cpu, before, result);

    field = read16(ram, cpu->es, 0x0010u);
    set_cmp_zero_flags(cpu, field);
    if (field != 0u) {
        write16(ram, cpu->es, 0x0012u, 1u);
        cpu->ip = 0x03B6u;
        return PR_LOADER_0338_CALL_0435;
    }

    before = read16(ram, cpu->ds, 0x43E2u);
    result = (uint16_t)(before + 1u);
    write16(ram, cpu->ds, 0x43E2u, result);
    set_inc16_flags(cpu, before, result);
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->es);
    cpu->ip = 0x0352u;
    return PR_LOADER_0338_CALL_0553;
}
