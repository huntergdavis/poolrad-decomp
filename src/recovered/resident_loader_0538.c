/* Loaded resident cold-loader helper +0538..+0552.
 * Evidence: docs/re/RESIDENT_LOADER_0538.md. */
#include "poolrad/recovered/resident_loader_0538.h"

#define RAM_MASK (PR_HANDLER_02E6_RAM_SIZE - 1u)
#define SUB_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

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
    unsigned odd = 0u;
    while (value != 0u) {
        odd ^= value & 1u;
        value >>= 1u;
    }
    return odd == 0u;
}

static uint16_t subtract(pr_handler_02e6_cpu *cpu,
                         uint16_t minuend, uint16_t subtrahend)
{
    uint16_t result = (uint16_t)(minuend - subtrahend);
    uint16_t flags = (uint16_t)(cpu->flags & ~SUB_FLAGS);
    if (minuend < subtrahend)
        flags |= 0x0001u;
    if (even_parity((uint8_t)result))
        flags |= 0x0004u;
    if (((minuend ^ subtrahend ^ result) & 0x0010u) != 0u)
        flags |= 0x0010u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    if (((minuend ^ subtrahend) & (minuend ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->flags = flags;
    return result;
}

pr_loader_0538_status pr_resident_loader_0538(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu)
{
    uint16_t selector, base, value;

    if (ram == NULL || ram_size < PR_HANDLER_02E6_RAM_SIZE || cpu == NULL)
        return PR_LOADER_0538_ARGUMENT;

    selector = read16(ram, cpu->ds, 0x43FAu);
    base = read16(ram, cpu->ds, 0x43F6u);
    if (selector != 0u) {
        cpu->es = selector;
        value = read16(ram, cpu->es, 0x0010u);
        cpu->ax = subtract(cpu, value, base);
        if ((cpu->flags & 0x0001u) == 0u)
            goto near_return;
    }

    value = read16(ram, cpu->ds, 0x43F8u);
    cpu->ax = subtract(cpu, value, base);

near_return:
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_LOADER_0538_RETURNED;
}
