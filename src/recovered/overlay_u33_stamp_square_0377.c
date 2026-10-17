/* GAME.OVR unit 0x33:+0377..+040E; docs/re/U33_STAMP_SQUARE_0377.md. */
#include "poolrad/recovered/overlay_u33_stamp_square_0377.h"
#include "poolrad/recovered/overlay_u33_clipped_writer_0221.h"

#include <string.h>

#define MASK (PR_U33_0377_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

_Static_assert(sizeof(pr_u33_0377_cpu) == sizeof(pr_u33_0221_cpu), "u33 +0221 CPU layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void push16(uint8_t *ram, pr_u33_0377_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void set_al(pr_u33_0377_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u33_0377_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

/* MOV AL,column; PUSH AX; MOV AL,row; PUSH AX; MOV AL,glyph; PUSH AX; PUSH [BP+6]; PUSH CS;
 * CALL +0221. Each AL load keeps the live AH, so the pushed words carry it. */
static pr_u33_0377_status stamp(uint8_t *ram, size_t ram_size, pr_u33_0377_cpu *cpu, uint16_t entry,
                        uint16_t return_delta, uint16_t column_word, int column_is_word,
                        uint8_t row, uint8_t glyph)
{
    uint16_t caller_cs = cpu->cs, return_ip = (uint16_t)(entry + return_delta);
    pr_u33_0221_cpu leaf;
    if (column_is_word)
        cpu->ax = column_word;
    else
        set_al(cpu, (uint8_t)column_word);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, row);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, glyph);
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = (uint16_t)(entry - 0x0156u);
    memcpy(&leaf, cpu, sizeof leaf);
    if (pr_u33_clipped_writer_0221(ram, ram_size, &leaf) != PR_U33_0221_OK)
        return PR_U33_0377_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0377_TRANSFER;
    return PR_U33_0377_OK;
}

/* MOV SP,BP; POP BP; RETF 2 */
static void leave(const uint8_t *ram, pr_u33_0377_cpu *cpu)
{
    size_t at;
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
}

/* PUSH BP; MOV BP,SP; SUB SP,2 (its flags are overwritten before they are observed) */
static void enter_frame(uint8_t *ram, pr_u33_0377_cpu *cpu)
{
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
}

/* MOV DI,[BP+6]; CMP BYTE SS:[DI-back],value */
static uint8_t selector(pr_u33_0377_cpu *cpu, const uint8_t *ram, uint16_t back, uint8_t value)
{
    uint8_t byte;
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    byte = read8(ram, cpu->ss, (uint16_t)(cpu->di - back));
    cmp8(cpu, byte, value);
    return byte;
}

#define TRY(call) do { pr_u33_0377_status status_ = (call); if (status_ != PR_U33_0377_OK) return status_; } while (0)

pr_u33_0377_status pr_u33_stamp_square_0377(uint8_t *ram, size_t ram_size, pr_u33_0377_cpu *cpu)
{
    uint16_t entry;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_0377_RAM_SIZE)
        return PR_U33_0377_ARGUMENT;
    entry = cpu->ip;
    enter_frame(ram, cpu);
    if (selector(cpu, ram, 7u, 1u) == 1u) {
        /* +0387..+03C7: glyphs 5 on row 0 and 0Ah on row 1 at columns 3 and 4. */
        TRY(stamp(ram, ram_size, cpu, entry, 0x20u, 3u, 0, 0u, 5u));
        TRY(stamp(ram, ram_size, cpu, entry, 0x30u, 4u, 0, 0u, 5u));
        TRY(stamp(ram, ram_size, cpu, entry, 0x40u, 3u, 0, 1u, 0x0Au));
        TRY(stamp(ram, ram_size, cpu, entry, 0x50u, 4u, 0, 1u, 0x0Au));
    } else {
        /* +03C9..+0406: glyph 16h at the same four positions. */
        TRY(stamp(ram, ram_size, cpu, entry, 0x62u, 3u, 0, 0u, 0x16u));
        TRY(stamp(ram, ram_size, cpu, entry, 0x72u, 4u, 0, 0u, 0x16u));
        TRY(stamp(ram, ram_size, cpu, entry, 0x82u, 3u, 0, 1u, 0x16u));
        TRY(stamp(ram, ram_size, cpu, entry, 0x92u, 4u, 0, 1u, 0x16u));
    }
    leave(ram, cpu);
    return PR_U33_0377_OK;
}
