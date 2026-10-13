/* Resident cold-loader post-read branch at loaded +02A1.
 * Evidence: docs/re/RESIDENT_LOADER_READ_BRANCH_02A1.md. */
#include "poolrad/recovered/resident_loader_read_branch_02a1.h"

#define STATUS_FLAGS 0x08D5u /* OF, SF, ZF, AF, PF, CF */

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return ((((uint32_t)segment << 4) + offset) & 0xFFFFFu);
}

static uint16_t read_word(const uint8_t *ram, uint16_t segment,
                          uint16_t offset)
{
    uint32_t low = physical(segment, offset);
    uint32_t high = physical(segment, (uint16_t)(offset + 1u));
    return (uint16_t)(ram[low] | ((uint16_t)ram[high] << 8));
}

static int even_parity(uint8_t value)
{
    unsigned bits = 0;
    while (value != 0u) {
        bits += value & 1u;
        value >>= 1;
    }
    return (bits & 1u) == 0u;
}

static uint16_t cmp_flags(uint16_t old_flags, uint16_t left,
                          uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    uint16_t flags = (uint16_t)(old_flags & (uint16_t)~STATUS_FLAGS);
    if (left < right) flags |= 0x0001u;
    if (even_parity((uint8_t)result)) flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    return flags;
}

static uint16_t zero_xor_flags(uint16_t old_flags)
{
    /* The native DOSBox result clears undefined AF for XOR. */
    return (uint16_t)((old_flags & (uint16_t)~STATUS_FLAGS) | 0x0044u);
}

pr_loader_read_branch_02a1_status pr_resident_loader_read_branch_02a1(
    const uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu)
{
    uint16_t rounded, paragraph, second_count;

    if (ram == NULL || ram_size != PR_HANDLER_02E6_RAM_SIZE ||
        cpu == NULL || cpu->cs != 0x12C6u || cpu->ip != 0x02A1u)
        return PR_LOADER_READ_BRANCH_02A1_ARGUMENT;

    /* +02A1 JB is the first read's carry/error gate. */
    if ((cpu->flags & 1u) != 0u) {
        cpu->ip = 0x02E1u;
        return PR_LOADER_READ_BRANCH_02A1_AT_EPILOGUE;
    }

    /* +02A3 CMP AX,CX; +02A5 JB selects the short-read error value. */
    cpu->flags = cmp_flags(cpu->flags, cpu->ax, cpu->cx);
    if ((cpu->flags & 1u) != 0u) {
        cpu->ax = 0x0064u;
        cpu->ip = 0x02E1u;
        return PR_LOADER_READ_BRANCH_02A1_AT_EPILOGUE;
    }

    /* +02A7..+02BC: round the first read length to paragraphs, advance
     * DS from ES:[0010], clear DX, and test the second requested count. */
    rounded = (uint16_t)(cpu->ax + 0x000Fu);
    paragraph = (uint16_t)(rounded >> 4);
    cpu->ax = (uint16_t)(paragraph + read_word(ram, cpu->es, 0x0010u));
    cpu->ds = cpu->ax;
    cpu->dx = 0u;
    cpu->flags = zero_xor_flags(cpu->flags); /* +02B5 XOR DX,DX */
    second_count = read_word(ram, cpu->es, 0x000Au);
    cpu->cx = second_count;
    if (second_count != 0u) {
        cpu->ax = (uint16_t)((cpu->ax & 0x00FFu) | 0x3F00u);
        cpu->ip = 0x02C0u;
        return PR_LOADER_READ_BRANCH_02A1_AT_SECOND_DOS_READ;
    }

    /* +02DA XOR AX,AX; +02DC JMP +02E1. */
    cpu->ax = 0u;
    cpu->flags = zero_xor_flags(cpu->flags);
    cpu->ip = 0x02E1u;
    return PR_LOADER_READ_BRANCH_02A1_AT_EPILOGUE;
}
