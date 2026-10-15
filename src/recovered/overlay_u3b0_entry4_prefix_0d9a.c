/* DOS v1.3 GAME.OVR U3B0 entry4 +0D9A..+0DAE, 21 original bytes. */
#include "poolrad/recovered/overlay_u3b0_entry4_prefix_0d9a.h"

#define MASK (PR_U3B0_0D9A_RAM_SIZE - 1u)
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

static void push16(uint8_t *ram, pr_u3b0_0d9a_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint8_t even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void sub_sp_flags(pr_u3b0_0d9a_cpu *cpu, uint16_t before,
                         uint16_t result)
{
    uint16_t flags = cpu->flags & (uint16_t)~ARITH_FLAGS;
    if (before < 0x018Eu) flags |= 0x0001u;
    if (even_parity((uint8_t)result)) flags |= 0x0004u;
    if (((before ^ 0x018Eu ^ result) & 0x0010u) != 0u)
        flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (((before ^ 0x018Eu) & (before ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->flags = flags;
}

pr_u3b0_0d9a_status pr_u3b0_entry4_prefix_0d9a(
    uint8_t *ram, size_t ram_size, pr_u3b0_0d9a_cpu *cpu)
{
    uint16_t before, source_offset;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0D9A_RAM_SIZE ||
        cpu->ip != 0x0D9Au)
        return PR_U3B0_0D9A_ARGUMENT;

    push16(ram, cpu, cpu->bp);                   /* +0D9A PUSH BP */
    cpu->bp = cpu->sp;                           /* +0D9B MOV BP,SP */
    before = cpu->sp;
    cpu->sp = (uint16_t)(before - 0x018Eu);      /* +0D9D SUB SP,018E */
    sub_sp_flags(cpu, before, cpu->sp);
    source_offset = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x2Au));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x2Cu));
    cpu->di = source_offset;                     /* +0DA1 LES DI,[BP+2A] */
    push16(ram, cpu, cpu->es);                   /* +0DA4 PUSH ES */
    push16(ram, cpu, cpu->di);                   /* +0DA5 PUSH DI */
    cpu->di = (uint16_t)(cpu->bp - 0x2Au);       /* +0DA6 LEA DI,[BP-2A] */
    push16(ram, cpu, cpu->ss);                   /* +0DA9 PUSH SS */
    push16(ram, cpu, cpu->di);                   /* +0DAA PUSH DI */
    cpu->ax = 0x0028u;                          /* +0DAB MOV AX,28 */
    push16(ram, cpu, cpu->ax);                   /* +0DAE PUSH AX */
    cpu->ip = 0x0DAFu;
    return PR_U3B0_0D9A_OK;
}
