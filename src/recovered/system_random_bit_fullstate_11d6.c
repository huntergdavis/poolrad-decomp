/* START.EXE SYSTEM +11D6..+11F2; docs/re/SYSTEM_RANDOM_BIT_FULLSTATE_11D6.md. */
#include "poolrad/recovered/system_random_bit_fullstate_11d6.h"

#define MASK (PR_SYSTEM_120F_RAM_SIZE - 1u)
#define DEFINED_LOGIC_FLAGS 0x08C5u /* AF is undefined. */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read_word(const uint8_t *ram, uint16_t segment,
                          uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void write_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                       uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static int in_code(const pr_system_11d6_cpu *cpu, size_t at)
{
    unsigned offset;
    for (offset = 0x11D6u; offset <= 0x11F2u; ++offset)
        if (at == physical(cpu->cs, (uint16_t)offset)) return 1;
    for (offset = 0x120Fu; offset <= 0x1244u; ++offset)
        if (at == physical(cpu->cs, (uint16_t)offset)) return 1;
    return 0;
}

static int writes_code(const pr_system_11d6_cpu *cpu)
{
    unsigned i;
    size_t at = physical(cpu->ss, (uint16_t)(cpu->sp - 2u));
    if (in_code(cpu, at) || in_code(cpu, (at + 1u) & MASK)) return 1;
    for (i = 0u; i < 4u; ++i)
        if (in_code(cpu, physical(cpu->ds, (uint16_t)(0x4428u + i))))
            return 1;
    return 0;
}

static uint16_t and_flags(uint16_t incoming, uint8_t high_dx)
{
    uint8_t parity = high_dx;
    uint16_t flags = (uint16_t)(incoming & (uint16_t)~(DEFINED_LOGIC_FLAGS | 0x0010u)); /* pinned: AND clears AF */
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (!(parity & 1u)) flags |= 0x0004u;
    if (high_dx == 0u) flags |= 0x0040u;
    if (high_dx & 0x80u) flags |= 0x0080u;
    return flags;
}

pr_system_11d6_status pr_system_random_bit_fullstate_11d6(
    uint8_t *ram, size_t ram_size, pr_system_11d6_cpu *cpu)
{
    unsigned iteration;
    uint16_t bx, dx;
    uint8_t al = 0x80u;
    uint8_t dh;

    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_120F_RAM_SIZE ||
        cpu->ip != 0x11D6u || writes_code(cpu))
        return PR_SYSTEM_11D6_ARGUMENT;

    /* +11D6 CALL +120F pushes the live near return word at SS:SP-2. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write_word(ram, cpu->ss, cpu->sp, 0x11D9u);
    cpu->ip = 0x120Fu;
    if (pr_system_seed_fullstate_120f(ram, ram_size, cpu) !=
        PR_SYSTEM_120F_RETURNED)
        return PR_SYSTEM_11D6_CALLEE_FAILED;
    if (cpu->ip != 0x11D9u)
        return PR_SYSTEM_11D6_CALLEE_TRANSFER;

    /* +11D9 XCHG BX,AX; +11DA MOV AX,80; +11DD MOV CX,20. */
    bx = cpu->ax;
    dx = cpu->dx;
    cpu->ax = 0x0080u;
    cpu->cx = 0x0020u;
    for (iteration = 0u; iteration < 32u; ++iteration) {
        /* +11E0 TEST DH,80; +11E3 JNE +11EF. */
        if (dx & 0x8000u) break;
        /* +11E5 SHL BX,1; +11E7 RCL DX,1; +11E9 DEC AL;
         * +11EB LOOP +11E0. */
        dx = (uint16_t)((dx << 1) | (bx >> 15));
        bx = (uint16_t)(bx << 1);
        al = (uint8_t)(al - 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    if (cpu->cx == 0u) al = 0u; /* +11ED XOR AL,AL. */
    cpu->ax = al;
    cpu->bx = bx;
    cpu->dx = dx;

    /* +11EF AND DH,7F is the final defined FLAGS writer. */
    dh = (uint8_t)((dx >> 8) & 0x7Fu);
    cpu->dx = (uint16_t)((dx & 0x00FFu) | ((uint16_t)dh << 8));
    cpu->flags = and_flags(cpu->flags, dh);

    /* +11F2 RETF reads IP and CS at separately wrapped SS offsets. */
    cpu->ip = read_word(ram, cpu->ss, cpu->sp);
    cpu->cs = read_word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_SYSTEM_11D6_RETURNED;
}
