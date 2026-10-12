/* Loaded resident cold-loader size helper +0553..+055E.
 * Evidence: docs/re/RESIDENT_LOADER_0553.md. */
#include "poolrad/recovered/resident_loader_0553.h"

#define RAM_MASK (PR_HANDLER_02E6_RAM_SIZE - 1u)
#define ADD_FLAGS 0x08D5u /* OF SF ZF AF PF CF */
#define SHR_DEFINED_FLAGS 0x00C5u /* SF ZF PF CF, count=4 */

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

static void add15_flags(pr_handler_02e6_cpu *cpu, uint16_t before,
                        uint16_t sum)
{
    uint16_t flags = (uint16_t)(cpu->flags & ~ADD_FLAGS);
    if ((uint32_t)before + 15u > 0xFFFFu)
        flags |= 0x0001u;
    if (even_parity((uint8_t)sum))
        flags |= 0x0004u;
    if ((before & 0x000Fu) != 0u)
        flags |= 0x0010u;
    if (sum == 0u)
        flags |= 0x0040u;
    if ((sum & 0x8000u) != 0u)
        flags |= 0x0080u;
    if ((~(before ^ 0x000Fu) & (before ^ sum) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->flags = flags;
}

pr_loader_0553_status pr_resident_loader_0553(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu)
{
    uint16_t before, sum, result, flags;

    if (ram == NULL || ram_size < PR_HANDLER_02E6_RAM_SIZE || cpu == NULL)
        return PR_LOADER_0553_ARGUMENT;

    before = read16(ram, cpu->es, 0x0008u);
    sum = (uint16_t)(before + 15u);
    cpu->ax = sum;
    add15_flags(cpu, before, sum);
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);

    result = (uint16_t)(sum >> 4u);
    flags = (uint16_t)(cpu->flags & ~SHR_DEFINED_FLAGS);
    if ((sum & 0x0008u) != 0u)
        flags |= 0x0001u;
    if (even_parity((uint8_t)result))
        flags |= 0x0004u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    cpu->ax = result;
    cpu->flags = flags;

    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_LOADER_0553_RETURNED;
}
