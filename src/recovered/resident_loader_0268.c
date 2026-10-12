/* Loaded resident cold-loader far-call target prefix +0268..+028E.
 * Evidence: docs/re/RESIDENT_LOADER_0268.md. */
#include "poolrad/recovered/resident_loader_0268.h"

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
    unsigned odd = 0u;
    while (value != 0u) {
        odd ^= value & 1u;
        value >>= 1u;
    }
    return odd == 0u;
}

static void adc_flags(pr_handler_02e6_cpu *cpu, uint16_t left,
                      uint16_t right, unsigned carry, uint32_t sum)
{
    uint16_t result = (uint16_t)sum;
    uint16_t flags = (uint16_t)(cpu->flags & ~ARITH_FLAGS);
    if (sum > 0xFFFFu)
        flags |= 0x0001u;
    if (even_parity((uint8_t)result))
        flags |= 0x0004u;
    if (((left & 0x000Fu) + (right & 0x000Fu) + carry) > 0x000Fu)
        flags |= 0x0010u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    if ((~(left ^ right) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->flags = flags;
}

pr_loader_0268_status pr_resident_loader_0268(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu)
{
    uint16_t left, right;
    uint32_t sum;
    unsigned carry;

    if (ram == NULL || ram_size < PR_HANDLER_02E6_RAM_SIZE || cpu == NULL)
        return PR_LOADER_0268_ARGUMENT;

    cpu->bx = cpu->sp; /* +0268 MOV BX,SP */
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bx + 4u));
    cpu->sp = (uint16_t)(cpu->sp - 2u); /* +026E PUSH DS */
    write16(ram, cpu->ss, cpu->sp, cpu->ds);
    cpu->bx = read16(ram, cpu->ds, 0x43FCu);
    cpu->di = (uint16_t)(read16(ram, cpu->ds, 0x4422u) + 0x0010u);
    cpu->dx = read16(ram, cpu->es, 0x0004u);
    cpu->cx = read16(ram, cpu->es, 0x0006u);

    sum = (uint32_t)cpu->dx + read16(ram, cpu->ds, 0x43E8u);
    cpu->dx = (uint16_t)sum;
    carry = sum > 0xFFFFu;
    left = cpu->cx;
    right = read16(ram, cpu->ds, 0x43EAu);
    sum = (uint32_t)left + right + carry;
    cpu->cx = (uint16_t)sum;
    adc_flags(cpu, left, right, carry, sum);

    cpu->ax = 0x4200u;
    cpu->ip = 0x028Fu;
    return PR_LOADER_0268_AT_DOS_SEEK;
}
