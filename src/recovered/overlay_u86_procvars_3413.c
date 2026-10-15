/* Unit 86:+3413..+37AF; see docs/re/U86_PROCVARS_3413.md. */
#include "poolrad/recovered/overlay_u86_procvars_3413.h"

#define MASK (PR_U86_3413_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void install(uint8_t *ram, pr_u86_3413_cpu *cpu, uint16_t destination,
                    uint16_t target, uint16_t segment_operand)
{
    cpu->ax = target;
    cpu->dx = read16_at(ram, physical(cpu->cs, segment_operand));
    write16(ram, cpu->ds, destination, cpu->ax);
    write16(ram, cpu->ds, (uint16_t)(destination + 2u), cpu->dx);
}

pr_u86_3413_status pr_u86_procvars_3413(
    uint8_t *ram, size_t ram_size, pr_u86_3413_cpu *cpu)
{
    static const uint16_t targets[67] = {
        0x0057, 0x005C, 0x0061, 0x0066, 0x006B, 0x0070, 0x0070, 0x0075,
        0x007A, 0x007F, 0x006B, 0x0084, 0x0089, 0x008E, 0x0093, 0x0070,
        0x0070, 0x006B, 0x0098, 0x009D, 0x00A2, 0x006B, 0x00A7, 0x00AC,
        0x00B1, 0x00B6, 0x00BB, 0x00C0, 0x006B, 0x00C5, 0x00CA, 0x00CF,
        0x00D4, 0x00D9, 0x00DE, 0x00E3, 0x00E8, 0x00ED, 0x00F2, 0x00F7,
        0x00FC, 0x0101, 0x004D, 0x0106, 0x010B, 0x00FC, 0x0110, 0x0115,
        0x00A7, 0x00C5, 0x011A, 0x0070, 0x0070, 0x0070, 0x011F, 0x0124,
        0x0129, 0x012E, 0x0133, 0x0138, 0x013D, 0x0142, 0x0147, 0x0110,
        0x014C, 0x0151, 0x0156
    };
    size_t at;
    unsigned i;
    if (!ram || !cpu || ram_size < PR_U86_3413_RAM_SIZE)
        return PR_U86_3413_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    install(ram, cpu, 0x6B84u, 0x0034u, 0x341Au);
    ram[physical(cpu->ds, 0x681Fu)] = 0;
    ram[physical(cpu->ds, 0x6DBFu)] = 0;
    cpu->ax = 0; /* +342D XOR AX,AX; all subsequent body instructions are MOV. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08D5u) | 0x0044u);
    write16(ram, cpu->ds, 0x6DB5u, cpu->ax);
    write16(ram, cpu->ds, 0x6DB7u, cpu->ax);
    ram[physical(cpu->ds, 0x6821u)] = 1;
    for (i = 0; i < 67u; ++i)
        install(ram, cpu, (uint16_t)(0x6B88u + 4u * i), targets[i],
                (uint16_t)(0x343Eu + 13u * i));
    install(ram, cpu, 0x6988u, 0x015Bu, 0x37A5u);
    /* No MOV SP,BP in this leaf: POP uses the current SP. */
    cpu->bp = read16_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U86_3413_OK;
}
