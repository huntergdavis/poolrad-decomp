/* START.EXE SYSTEM +11C0..+11D5; docs/re/SYSTEM_RANDOM_FULLSTATE_11C0.md. */
#include "poolrad/recovered/system_random_fullstate_11c0.h"

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

static int in_code(const pr_system_11c0_cpu *cpu, size_t at)
{
    unsigned offset;
    for (offset = 0x11C0u; offset <= 0x11D5u; ++offset)
        if (at == physical(cpu->cs, (uint16_t)offset)) return 1;
    for (offset = 0x120Fu; offset <= 0x1244u; ++offset)
        if (at == physical(cpu->cs, (uint16_t)offset)) return 1;
    return 0;
}

static int writes_code(const pr_system_11c0_cpu *cpu)
{
    unsigned i;
    uint16_t near_sp = (uint16_t)(cpu->sp - 2u);
    size_t at = physical(cpu->ss, near_sp);
    if (in_code(cpu, at) || in_code(cpu, (at + 1u) & MASK)) return 1;
    for (i = 0u; i < 4u; ++i)
        if (in_code(cpu, physical(cpu->ds, (uint16_t)(0x4428u + i))))
            return 1;
    return 0;
}

static uint16_t or_flags(uint16_t incoming, uint16_t value)
{
    uint8_t parity = (uint8_t)value;
    uint16_t flags = (uint16_t)(incoming & (uint16_t)~DEFINED_LOGIC_FLAGS);
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (!(parity & 1u)) flags |= 0x0004u;
    if (value == 0u) flags |= 0x0040u;
    if (value & 0x8000u) flags |= 0x0080u;
    return flags;
}

pr_system_11c0_status pr_system_random_fullstate_11c0(
    uint8_t *ram, size_t ram_size, pr_system_11c0_cpu *cpu)
{
    uint16_t quotient, remainder;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_120F_RAM_SIZE ||
        cpu->ip != 0x11C0u || writes_code(cpu))
        return PR_SYSTEM_11C0_ARGUMENT;

    /* +11C0 near CALL +120F: the return IP remains in live SS RAM. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write_word(ram, cpu->ss, cpu->sp, 0x11C3u);
    cpu->ip = 0x120Fu;
    if (pr_system_seed_fullstate_120f(ram, ram_size, cpu) !=
        PR_SYSTEM_120F_RETURNED)
        return PR_SYSTEM_11C0_CALLEE_FAILED;
    if (cpu->ip != 0x11C3u)
        return PR_SYSTEM_11C0_CALLEE_TRANSFER;

    /* +11C3..+11CD: XOR AX,AX; MOV BX,SP; SS:[BX+4]; OR BX,BX. */
    cpu->ax = 0u;
    cpu->bx = cpu->sp;
    cpu->bx = read_word(ram, cpu->ss, (uint16_t)(cpu->bx + 4u));
    cpu->flags = or_flags(cpu->flags, cpu->bx);

    if (cpu->bx != 0u) {
        /* +11CF..+11D2: XCHG DX,AX; DIV BX; XCHG DX,AX.
         * DX is zero at DIV, so a nonzero divisor cannot overflow. */
        quotient = (uint16_t)(cpu->dx / cpu->bx);
        remainder = (uint16_t)(cpu->dx % cpu->bx);
        cpu->ax = remainder;
        cpu->dx = quotient;
        /* DIV's arithmetic FLAGS are architecturally undefined. */
    }

    /* +11D3 RETF 2 reads the two words at separately wrapped SS offsets. */
    cpu->ip = read_word(ram, cpu->ss, cpu->sp);
    cpu->cs = read_word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_SYSTEM_11C0_RETURNED;
}
