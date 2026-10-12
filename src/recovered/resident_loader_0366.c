/* Loaded resident cold-loader continuation +0366 to first call boundary.
 * Evidence: docs/re/RESIDENT_LOADER_0366.md. */
#include "poolrad/recovered/resident_loader_0366.h"

#define RAM_MASK (PR_HANDLER_02E6_RAM_SIZE - 1u)
#define SUB_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return (uint16_t)(ram[physical(segment, offset)] |
                      ((uint16_t)ram[physical(segment,
                                              (uint16_t)(offset + 1u))] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    ram[physical(segment, offset)] = (uint8_t)value;
    ram[physical(segment, (uint16_t)(offset + 1u))] = (uint8_t)(value >> 8);
}

static unsigned even_parity(uint8_t value)
{
    unsigned odd = 0u;
    while (value != 0u) {
        odd ^= value & 1u;
        value >>= 1u;
    }
    return odd == 0u;
}

static void subtract_dx_ax(pr_handler_02e6_cpu *cpu)
{
    uint16_t left = cpu->dx, right = cpu->ax;
    uint16_t result = (uint16_t)(left - right);
    uint16_t flags = (uint16_t)(cpu->flags & ~SUB_FLAGS);
    if (left < right)
        flags |= 0x0001u;
    if (even_parity((uint8_t)result))
        flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x0010u) != 0u)
        flags |= 0x0010u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->dx = result;
    cpu->flags = flags;
}

static void push16(uint8_t *ram, pr_handler_02e6_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

pr_loader_0366_status pr_resident_loader_0366(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu)
{
    if (ram == NULL || ram_size < PR_HANDLER_02E6_RAM_SIZE || cpu == NULL)
        return PR_LOADER_0366_ARGUMENT;

    /* +0366 JMP +039C; +039C SUB DX,AX; +039E JA +0368. */
    subtract_dx_ax(cpu);
    if ((cpu->flags & (0x0001u | 0x0040u)) == 0u) {
        push16(ram, cpu, cpu->dx); /* +0368 PUSH DX */
        cpu->ip = 0x0369u;
        return PR_LOADER_0366_AT_INTERNAL_CALL;
    }

    cpu->es = read16(ram, cpu->ss, cpu->sp); /* +03A0 POP ES */
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ax = read16(ram, cpu->ds, 0x43F6u); /* +03A1 MOV AX,[43F6] */
    write16(ram, cpu->es, 0x0010u, cpu->ax); /* +03A4 MOV ES:[10],AX */
    push16(ram, cpu, cpu->es); /* +03A8 PUSH ES */
    push16(ram, cpu, cpu->es); /* +03A9 PUSH ES */
    cpu->ip = 0x03AAu;
    return PR_LOADER_0366_AT_FAR_CALL;
}
