/* GAME.OVR unit 33:+08A4..+0B96; see docs/re/U33_BYTE_FLAGS_08A4.md. */
#include "poolrad/recovered/overlay_u33_byte_flags_08a4.h"

#define RAM_MASK (PR_U33_08A4_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void parity(pr_u33_08a4_cpu *cpu, uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    if (!(value & 1u)) cpu->flags |= 0x0004u;
}

static void logic8(pr_u33_08a4_cpu *cpu, uint8_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS; /* Undefined AF normalized to zero. */
    parity(cpu, value);
    if (!value) cpu->flags |= 0x0040u;
    if (value & 0x80u) cpu->flags |= 0x0080u;
}

static void cmp16(pr_u33_08a4_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    parity(cpu, (uint8_t)result);
    if (left < right) cpu->flags |= 1u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (!result) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if ((left ^ right) & (left ^ result) & 0x8000u) cpu->flags |= 0x800u;
}

static int between(uint8_t value, unsigned first, unsigned last)
{
    return value >= first && value <= last;
}

/* Ordered source branches, not a default-filled lookup table. All CMP flags
 * in this selection are overwritten by +0B6B AND/+0B6D OR before return.
 * No match means no local write, as in the original. */
static int classify(uint8_t value, uint8_t *result)
{
    if (between(value, 1, 2) || between(value, 0x28, 0x29) ||
        between(value, 0x4F, 0x56) || between(value, 0x5B, 0x5C) ||
        value == 0x2B || value == 0xF1) *result = 1;
    else if (between(value, 0xA8, 0xB1)) *result = 0x21;
    else if (value == 0xB9 || between(value, 0xBC, 0xC0)) *result = 0x11;
    else if (between(value, 3, 4) || between(value, 6, 7) ||
        between(value, 8, 9) || between(value, 0x13, 0x15) ||
        between(value, 0x17, 0x19)) *result = 3;
    else if (between(value, 3, 4) || value == 7 || between(value, 0xA, 0x12) ||
        value == 0x16 || between(value, 0x1A, 0x24) ||
        between(value, 0xC9, 0xD0)) *result = 2;
    else if (between(value, 0xD1, 0xD5)) *result = 0x12;
    else if (value == 0x25 || value == 0x39 || value == 0x40 ||
        between(value, 0x9A, 0x9E) || between(value, 0xA4, 0xA5) ||
        value == 0xB4 || value == 0xF3 || between(value, 0xEF, 0xF0) ||
        value == 0xFA || value == 0xFC) *result = 4;
    else if (between(value, 0x27, 0x2C) || value == 0x99 || value == 0x9F)
        *result = 5;
    else if (between(value, 0x2D, 0x2E) || between(value, 0x33, 0x38) ||
        between(value, 0x43, 0x4D) || between(value, 0x5D, 0x61)) *result = 0x44;
    else if (between(value, 0xB5, 0xB8) || value == 0xBA ||
        between(value, 0xC1, 0xC8)) *result = 0x14;
    else if (between(value, 0x2F, 0x32) || between(value, 0x62, 0x65) ||
        value == 0xF2) *result = 0x40;
    else if (value == 0x26) *result = 0x41;
    else if (between(value, 0xB2, 0xB3)) *result = 0x60;
    else if (between(value, 0x3A, 0x3C) || between(value, 0x41, 0x42)) *result = 0x48;
    else if (between(value, 0x3D, 0x3F) || between(value, 0x79, 0x7C) ||
        between(value, 0x87, 0x89) || value == 0x8B || value == 0x8D ||
        between(value, 0x90, 0x93) || value == 0x95 || between(value, 0x97, 0x98) ||
        between(value, 0xA0, 0xA2) || between(value, 0xA6, 0xA7) ||
        between(value, 0xD6, 0xD8) || value == 0xEE) *result = 8;
    else if (between(value, 0x57, 0x5A) || value == 0xA3 || value == 0xE0) *result = 0x88;
    else if (value == 0x7D || value == 0x7C || value == 0x8A || value == 0x8C ||
        value == 0x8E || value == 0x8F || value == 0x94 || value == 0x96) *result = 9;
    else if (between(value, 0x7E, 0x85) || between(value, 0xE5, 0xE7)) *result = 0x28;
    else if (between(value, 0xD9, 0xDF)) *result = 0x18;
    else if (value == 0x4E || value == 0xF5 || between(value, 0xE8, 0xE9)) *result = 0x20;
    else if (value == 0x6D || between(value, 0xF4, 0xF6)) *result = 0x80;
    else if (between(value, 0x6E, 0x71) || between(value, 0x73, 0x78)) *result = 0x81;
    else if (between(value, 0x66, 0x6C) || value == 0x72) *result = 0x90;
    else if (between(value, 0xE1, 0xE4) || between(value, 0xEA, 0xED)) *result = 0xA0;
    else return 0;
    return 1;
}

pr_u33_08a4_status pr_u33_byte_flags_08a4(
    uint8_t *ram, size_t ram_size, pr_u33_08a4_cpu *cpu)
{
    uint8_t value, selected;
    size_t local;
    if (!ram || !cpu || ram_size < PR_U33_08A4_RAM_SIZE)
        return PR_U33_08A4_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    value = ram[physical(cpu->ds, 0x465Au)];
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
    local = physical(cpu->ss, (uint16_t)(cpu->bp - 2u));
    if (classify(value, &selected)) ram[local] = selected;

    /* +0B68..+0B87: preserve the live local and far-pointer read order. */
    value = (uint8_t)(ram[local] & 0x80u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
    logic8(cpu, value);
    if (value) {
        size_t pointer_at = physical(cpu->ds, 0x49D2u);
        uint16_t window;
        cpu->di = read16_at(ram, pointer_at);
        cpu->es = read16_at(ram, (pointer_at + 2u) & RAM_MASK);
        window = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x366u));
        cmp16(cpu, window, 0xFFu);
        if (window == 0xFFu) {
            ram[local] = (uint8_t)(ram[local] - 0x80u);
            value = (uint8_t)(ram[local] | 1u);
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
            logic8(cpu, value);
            ram[local] = value;
        }
    }
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[local]);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = (uint8_t)cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
              ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))]);
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & RAM_MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U33_08A4_OK;
}
