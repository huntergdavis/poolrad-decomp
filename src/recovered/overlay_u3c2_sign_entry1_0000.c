/* DOS v1.3 GAME.OVR U3C2 entry 1 +0000..+002D, 46 original bytes. */
#include "poolrad/recovered/overlay_u3c2_sign_entry1_0000.h"

#define MASK (PR_U3C2_0000_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint8_t even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

pr_u3c2_0000_status pr_u3c2_sign_entry1_0000(
    uint8_t *ram, size_t ram_size, pr_u3c2_0000_cpu *cpu)
{
    uint16_t value, flags;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3C2_0000_RAM_SIZE ||
        cpu->ip != 0u)
        return PR_U3C2_0000_ARGUMENT;

    /* PUSH BP; MOV BP,SP; SUB SP,2. Its flags are replaced by CMP. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);

    value = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    flags = cpu->flags & (uint16_t)~ARITH_FLAGS;
    if (even_parity((uint8_t)value)) flags |= 0x0004u;
    if (value == 0u) flags |= 0x0040u;
    if ((value & 0x8000u) != 0u) flags |= 0x0080u;
    cpu->flags = flags; /* CMP value,0; positive path repeats same CMP. */

    if ((value & 0x8000u) != 0u) {
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0xFFFFu);
    } else if (value != 0u) {
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);
    } else {
        cpu->ax = 0u; /* XOR AX,AX; AF follows Unicorn convention. */
        cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) |
                                0x0044u);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);
    }

    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->cs = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u + 2u);
    return PR_U3C2_0000_OK;
}
