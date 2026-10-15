/* DOS v1.3 GAME.OVR U3B0 entry4 +0DB4..+0DC1, 14 original bytes. */
#include "poolrad/recovered/overlay_u3b0_entry4_successor_0db4.h"

#define MASK (PR_U3B0_0DB4_RAM_SIZE - 1u)

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

static void push16(uint8_t *ram, pr_u3b0_0db4_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

pr_u3b0_0db4_status pr_u3b0_entry4_successor_0db4(
    uint8_t *ram, size_t ram_size, pr_u3b0_0db4_cpu *cpu)
{
    uint16_t source_offset;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0DB4_RAM_SIZE ||
        cpu->ip != 0x0DB4u)
        return PR_U3B0_0DB4_ARGUMENT;

    source_offset = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x26u));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x28u));
    cpu->di = source_offset;                    /* +0DB4 LES DI,[BP+26] */
    push16(ram, cpu, cpu->es);                  /* +0DB7 PUSH ES */
    push16(ram, cpu, cpu->di);                  /* +0DB8 PUSH DI */
    cpu->di = (uint16_t)(cpu->bp - 0x53u);      /* +0DB9 LEA DI,[BP-53] */
    push16(ram, cpu, cpu->ss);                  /* +0DBC PUSH SS */
    push16(ram, cpu, cpu->di);                  /* +0DBD PUSH DI */
    cpu->ax = 0x0028u;                         /* +0DBE MOV AX,28 */
    push16(ram, cpu, cpu->ax);                  /* +0DC1 PUSH AX */
    cpu->ip = 0x0DC2u;
    return PR_U3B0_0DB4_OK;
}
