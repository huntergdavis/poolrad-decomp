/* Loaded resident cold-loader continuation +0355..+0362.
 * Evidence: docs/re/RESIDENT_LOADER_0355.md. */
#include "poolrad/recovered/resident_loader_0355.h"

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

static unsigned even_parity(uint8_t value)
{
    unsigned bits = 0u;
    while (value != 0u) {
        bits ^= value & 1u;
        value >>= 1u;
    }
    return bits == 0u;
}

static void add16_flags(pr_handler_02e6_cpu *cpu, uint16_t left,
                        uint16_t right, uint16_t result)
{
    uint16_t flags = (uint16_t)(cpu->flags & ~ARITH_FLAGS);
    if ((uint32_t)left + right > 0xFFFFu)
        flags |= 0x0001u;
    if (even_parity((uint8_t)result))
        flags |= 0x0004u;
    if (((left & 0x000Fu) + (right & 0x000Fu)) > 0x000Fu)
        flags |= 0x0010u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    if ((~(left ^ right) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->flags = flags;
}

pr_loader_0355_status pr_resident_loader_0355(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu)
{
    uint16_t size, paragraphs, result;

    if (ram == NULL || ram_size < PR_HANDLER_02E6_RAM_SIZE || cpu == NULL)
        return PR_LOADER_0355_ARGUMENT;

    size = read16(ram, cpu->es, 0x000Au);
    size = (uint16_t)(size + 15u);
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    paragraphs = (uint16_t)(size >> 4u);
    result = (uint16_t)(paragraphs + cpu->ax);
    add16_flags(cpu, paragraphs, cpu->ax, result);
    cpu->dx = result;
    cpu->ip = 0x0363u;
    return PR_LOADER_0355_AT_CALL_0538;
}
