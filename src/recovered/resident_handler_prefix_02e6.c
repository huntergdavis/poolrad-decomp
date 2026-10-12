/* Loaded INT 3Fh handler +02E6 to first CALL +0338.
 * Evidence: docs/re/RESIDENT_HANDLER_PREFIX_02E6.md. */
#include "poolrad/recovered/resident_handler_prefix_02e6.h"

#define RAM_MASK (PR_HANDLER_02E6_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t address = physical(segment, offset);
    return (uint16_t)(ram[address] |
                      ((uint16_t)ram[(address + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_handler_02e6_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
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

static void set_sub16_flags(pr_handler_02e6_cpu *cpu, uint16_t left,
                            uint16_t right, uint16_t result)
{
    uint16_t flags = (uint16_t)(cpu->flags & ~ARITH_FLAGS);
    if (left < right)
        flags |= 0x0001u;
    if (even_parity((uint8_t)result))
        flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x0010u) != 0u)
        flags |= 0x0010u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->flags = flags;
}

static void set_add16_flags(pr_handler_02e6_cpu *cpu, uint16_t left,
                            uint16_t right, uint16_t result)
{
    uint16_t flags = (uint16_t)(cpu->flags & ~ARITH_FLAGS);
    if ((uint32_t)left + right > 0xFFFFu)
        flags |= 0x0001u;
    if (even_parity((uint8_t)result))
        flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x0010u) != 0u)
        flags |= 0x0010u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    if (((~(left ^ right)) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->flags = flags;
}

pr_handler_02e6_status pr_resident_handler_prefix_02e6(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu)
{
    uint16_t old, result, exchanged;

    if (ram == NULL || ram_size < PR_HANDLER_02E6_RAM_SIZE || cpu == NULL)
        return PR_HANDLER_02E6_ARGUMENT;

    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->bx);
    push16(ram, cpu, cpu->cx);
    push16(ram, cpu, cpu->dx);
    push16(ram, cpu, cpu->si);
    push16(ram, cpu, cpu->di);
    push16(ram, cpu, cpu->ds);
    push16(ram, cpu, cpu->es);

    /* The loaded MOV AX immediate is relocated; read it from the caller's
     * RAM rather than assuming the original executable's segment word. */
    cpu->ax = read16(ram, cpu->cs, 0x02F2u);
    cpu->ds = cpu->ax;
    cpu->flags |= 0x0200u; /* STI */

    old = (uint16_t)(cpu->bp + 2u);
    cpu->bx = read16(ram, cpu->ss, old);
    cpu->es = read16(ram, cpu->ss, (uint16_t)(old + 2u));
    push16(ram, cpu, read16(ram, cpu->es, cpu->bx));

    old = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 2u));
    result = (uint16_t)(old - 2u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp + 2u), result);
    set_sub16_flags(cpu, old, 2u, result);
    if (result == 0u) {
        cpu->ip = 0x0303u;
        return PR_HANDLER_02E6_AT_LOADER;
    }

    old = cpu->bp;
    cpu->bp = (uint16_t)(old + 6u);
    set_add16_flags(cpu, old, 6u, cpu->bp);
    cpu->ax = read16(ram, cpu->ss, cpu->bp);
    exchanged = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
    cpu->ax = exchanged;
    write16(ram, cpu->ss, cpu->bp, cpu->ax);
    cpu->ip = 0x0314u;
    return PR_HANDLER_02E6_AT_LOADER;
}
