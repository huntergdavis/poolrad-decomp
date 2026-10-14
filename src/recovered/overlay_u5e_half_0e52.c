/* DOS v1.3 unit 0x5E:+0x0E52..+0x0E7A.
 * Evidence: docs/re/U5E_HALF_0E52.md. */
#include "poolrad/recovered/overlay_u5e_half_0e52.h"

#define RAM_MASK (PR_U5E_0E52_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
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

pr_u5e_0e52_status pr_u5e_half_0e52(
    uint8_t *ram, size_t ram_size, pr_u5e_0e52_cpu *cpu)
{
    size_t at;
    uint8_t before;
    if (ram == NULL || ram_size != PR_U5E_0E52_RAM_SIZE || cpu == NULL)
        return PR_U5E_0E52_ARGUMENT;

    /* +0E52..+0E5B: PUSH BP, frame allocation, argument-to-local copy. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(cpu->bp >> 8);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
        ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))]);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 2u))] = (uint8_t)cpu->ax;

    /* +0E5E..+0E65: read DS only after the frame writes. Odd values
     * increment the local byte, with 255 wrapping to zero. */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                        ram[physical(cpu->ds, 0x6DE3u)]);
    if ((cpu->ax & 1u) != 0u) {
        at = physical(cpu->ss, (uint16_t)(cpu->bp - 2u));
        ram[at] = (uint8_t)(ram[at] + 1u);
    }

    /* +0E68..+0E72: zero AH, SHR AX, and copy the result through BP-1.
     * This SHR overwrites every defined arithmetic flag from earlier
     * SUB/SHR/INC/XOR instructions. Nonarithmetic flags remain intact. */
    before = ram[physical(cpu->ss, (uint16_t)(cpu->bp - 2u))];
    cpu->ax = (uint16_t)(before >> 1u);
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | (before & 1u));
    if (even_parity((uint8_t)cpu->ax))
        cpu->flags |= 0x0004u;
    if (cpu->ax == 0u)
        cpu->flags |= 0x0040u;
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = (uint8_t)cpu->ax;
    cpu->ax = ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))];

    /* +0E75..+0E7A: POP BP; RETF 2. Individual word reads can cross
     * the segment-offset boundary; SP arithmetic wraps at 16 bits. */
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & RAM_MASK);
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_U5E_0E52_OK;
}
