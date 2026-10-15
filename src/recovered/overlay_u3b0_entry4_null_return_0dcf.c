/* DOS v1.3 GAME.OVR U3B0 entry4 null branch and shared far return. */
#include "poolrad/recovered/overlay_u3b0_entry4_null_return_0dcf.h"

#define MASK (PR_U3B0_0DC7_RAM_SIZE - 1u)
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

pr_u3b0_0dcf_status pr_u3b0_entry4_null_return_0dcf(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dc7_cpu *cpu)
{
    uint16_t bp;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0DC7_RAM_SIZE ||
        cpu->ip != 0x0DCFu)
        return PR_U3B0_0DCF_ARGUMENT;

    bp = cpu->bp;

    /* LES DI,SS:[BP+0A]; XOR AX,AX; MOV ES:[DI],AX. */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(bp + 0x0Au));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(bp + 0x0Cu));
    cpu->ax = 0u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x0044u);
    write16(ram, cpu->es, cpu->di, 0u);

    /* The first store may alias the second LES pointer. */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(bp + 0x06u));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(bp + 0x08u));
    cpu->ax = 0u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x0044u);
    write16(ram, cpu->es, cpu->di, 0u);
    write16(ram, cpu->es, (uint16_t)(cpu->di + 2u), 0u);

    /* MOV [BP-1],0; JMP +10A1; MOV AL,[BP-1]. */
    ram[physical(cpu->ss, (uint16_t)(bp - 1u))] = 0u;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         ram[physical(cpu->ss, (uint16_t)(bp - 1u))]);

    /* MOV SP,BP; POP BP; RETF 0x28. Read after all aliasable stores. */
    cpu->sp = bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->cs = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u + 0x28u);
    return PR_U3B0_0DCF_OK;
}
