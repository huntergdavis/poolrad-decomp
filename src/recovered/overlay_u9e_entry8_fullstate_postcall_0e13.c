/* GAME.OVR U9E entry8 +0E13..+0E1E; see post-call evidence note. */
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_postcall_0e13.h"

#define MASK (PR_U9E_ENTRY8_POSTCALL_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t cmp8_flags(uint16_t incoming, uint8_t lhs, uint8_t rhs)
{
    uint8_t result = (uint8_t)(lhs - rhs);
    uint8_t parity = result;
    uint16_t flags = (uint16_t)(incoming & (uint16_t)~ARITH_FLAGS);
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (lhs < rhs) flags |= 0x0001u;
    if (!(parity & 1u)) flags |= 0x0004u;
    if ((lhs ^ rhs ^ result) & 0x10u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x80u) flags |= 0x0080u;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x80u) != 0u)
        flags |= 0x0800u;
    return flags;
}

static int write_aliases_code(const pr_u9e_entry8_postcall_cpu *cpu)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp - 3u));
    unsigned offset;
    for (offset = 0x0DE3u; offset <= 0x0E2Du; ++offset)
        if (at == physical(cpu->cs, (uint16_t)offset)) return 1;
    return 0;
}

pr_u9e_entry8_postcall_status pr_u9e_entry8_fullstate_postcall_0e13(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_postcall_cpu *cpu)
{
    uint8_t sum, index, count;
    if (ram == NULL || cpu == NULL ||
        ram_size != PR_U9E_ENTRY8_POSTCALL_RAM_SIZE ||
        cpu->ip != 0x0E13u || write_aliases_code(cpu))
        return PR_U9E_ENTRY8_POSTCALL_ARGUMENT;

    /* +0E13 INC AX; +0E14 ADD BYTE SS:[BP-3],AL. */
    cpu->ax = (uint16_t)(cpu->ax + 1u);
    sum = (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u)) +
                    (uint8_t)cpu->ax);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 3u))] = sum;

    /* +0E17 MOV AL,[BP-2]; +0E1A CMP AL,[BP-4]; +0E1D JNE. */
    index = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | index);
    count = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->flags = cmp8_flags(cpu->flags, index, count);
    cpu->ip = index == count ? 0x0E1Fu : 0x0E05u;
    return index == count ? PR_U9E_ENTRY8_POSTCALL_AT_TAIL :
                            PR_U9E_ENTRY8_POSTCALL_AT_LOOP;
}
