/* Bounded U3C2 entry4 prefix; docs/re/U3C2_ENTRY4_PREFIX_057B.md. */
#include "poolrad/recovered/overlay_u3c2_entry4_prefix_057b.h"

#define RAM_MASK (PR_U3C2_057B_PREFIX_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4u) + offset) & RAM_MASK;
}

static void put_word(uint8_t *ram, size_t address, uint16_t value)
{
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & RAM_MASK] = (uint8_t)(value >> 8u);
}

static void compare_byte_zero(pr_u3c2_057b_prefix_cpu *cpu, uint8_t value)
{
    uint8_t parity = value;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if ((parity & 1u) == 0u) cpu->flags |= 0x0004u;
    if (value == 0u) cpu->flags |= 0x0040u;
    if ((value & 0x80u) != 0u) cpu->flags |= 0x0080u;
    /* CMP byte,0 defines CF/AF/OF as zero; all other flags survive. */
}

pr_u3c2_057b_prefix_status pr_u3c2_entry4_prefix_057b(
    uint8_t *ram, size_t ram_size, pr_u3c2_057b_prefix_cpu *cpu)
{
    uint8_t value;
    if (!ram || !cpu || ram_size < PR_U3C2_057B_PREFIX_RAM_SIZE ||
        cpu->ip != PR_U3C2_057B_PREFIX_ENTRY)
        return PR_U3C2_057B_PREFIX_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, physical(cpu->ss, cpu->sp), cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    value = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 0x0Eu))];
    compare_byte_zero(cpu, value);
    cpu->ip = PR_U3C2_057B_PREFIX_STOP;
    return PR_U3C2_057B_PREFIX_OK;
}
