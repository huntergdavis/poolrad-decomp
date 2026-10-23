/* GAME.OVR U9E entry8 prefix; docs/re/U9E_ENTRY8_FULLSTATE_PREFIX_0DE3.md. */
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_prefix_0de3.h"

#define MASK (PR_U9E_ENTRY8_PREFIX_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define LOGIC_FLAGS 0x08D5u /* XOR clears AF in pinned DOSBox-X (flags.cpp:684-691). */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static int in_code(const pr_u9e_entry8_prefix_cpu *cpu, size_t at)
{
    unsigned offset;
    for (offset = 0x0DE3u; offset <= 0x0E2Du; ++offset)
        if (at == physical(cpu->cs, (uint16_t)offset)) return 1;
    return 0;
}

static int writes_code(const pr_u9e_entry8_prefix_cpu *cpu, int calls)
{
    uint16_t bp = (uint16_t)(cpu->sp - 2u);
    size_t at = physical(cpu->ss, bp);
    if (in_code(cpu, at) || in_code(cpu, (at + 1u) & MASK)) return 1;
    if (in_code(cpu, physical(cpu->ss, (uint16_t)(bp - 3u)))) return 1;
    if (!calls) return 0;
    if (in_code(cpu, physical(cpu->ss, (uint16_t)(bp - 4u))) ||
        in_code(cpu, physical(cpu->ss, (uint16_t)(bp - 2u)))) return 1;
    at = physical(cpu->ss, (uint16_t)(bp - 6u));
    return in_code(cpu, at) || in_code(cpu, (at + 1u) & MASK);
}

pr_u9e_entry8_prefix_status pr_u9e_entry8_fullstate_prefix_0de3(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_prefix_cpu *cpu)
{
    uint16_t bp;
    uint8_t count;
    if (ram == NULL || cpu == NULL ||
        ram_size != PR_U9E_ENTRY8_PREFIX_RAM_SIZE || cpu->ip != 0x0DE3u)
        return PR_U9E_ENTRY8_PREFIX_ARGUMENT;

    /* +0DE3 PUSH BP; +0DE4 MOV BP,SP; +0DE6 SUB SP,4. The count is
     * read later at +0DED, after the first local byte store. These
     * offsets cannot overlap within the same 16-bit SS segment. */
    bp = (uint16_t)(cpu->sp - 2u);
    count = read8(ram, cpu->ss, (uint16_t)(bp + 8u));
    if (writes_code(cpu, count != 0u)) return PR_U9E_ENTRY8_PREFIX_ARGUMENT;
    write16(ram, cpu->ss, bp, cpu->bp);
    cpu->bp = bp;
    cpu->sp = (uint16_t)(bp - 4u);
    write8(ram, cpu->ss, (uint16_t)(bp - 3u), 0u);

    /* +0DED CMP BYTE SS:[BP+8],0; +0DF1 JBE +0E1F. */
    if (count == 0u) {
        cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) |
                                0x0044u); /* ZF and PF. */
        cpu->ip = 0x0E1Fu;
        return PR_U9E_ENTRY8_PREFIX_AT_TAIL;
    }

    /* +0DF3..+0E0D: snapshot count, initialize loop index, reread
     * range byte from the live SS frame, clear AH, and push that WORD. */
    write8(ram, cpu->ss, (uint16_t)(bp - 4u), count);
    write8(ram, cpu->ss, (uint16_t)(bp - 2u), 1u);
    cpu->ax = (uint16_t)read8(ram, cpu->ss, (uint16_t)(bp + 6u));
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~LOGIC_FLAGS) |
                            0x0044u); /* XOR AH,AH: ZF and PF. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->ax);
    cpu->ip = 0x0E0Eu;
    return PR_U9E_ENTRY8_PREFIX_AT_CALL;
}
