/* GAME.OVR unit 0x33:+02BA..+0376; docs/re/U33_STAMP_BLOCK_02BA.md. */
#include "poolrad/recovered/overlay_u33_stamp_block_02ba.h"
#include "poolrad/recovered/overlay_u33_clipped_writer_0221.h"

#include <string.h>

#define MASK (PR_U33_02BA_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

_Static_assert(sizeof(pr_u33_02ba_cpu) == sizeof(pr_u33_0221_cpu), "u33 +0221 CPU layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void push16(uint8_t *ram, pr_u33_02ba_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void set_al(pr_u33_02ba_cpu *cpu, uint8_t value)
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

static void cmp8(pr_u33_02ba_cpu *cpu, uint8_t left, uint8_t right)
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
static pr_u33_02ba_status stamp(uint8_t *ram, size_t ram_size, pr_u33_02ba_cpu *cpu, uint16_t entry,
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
    cpu->ip = (uint16_t)(entry - 0x0099u);
    memcpy(&leaf, cpu, sizeof leaf);
    if (pr_u33_clipped_writer_0221(ram, ram_size, &leaf) != PR_U33_0221_OK)
        return PR_U33_02BA_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_02BA_TRANSFER;
    return PR_U33_02BA_OK;
}

/* MOV SP,BP; POP BP; RETF 2 */
static void leave(const uint8_t *ram, pr_u33_02ba_cpu *cpu)
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
static void enter_frame(uint8_t *ram, pr_u33_02ba_cpu *cpu)
{
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
}

static uint8_t local(const uint8_t *ram, const pr_u33_02ba_cpu *cpu, uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static void set_local(uint8_t *ram, const pr_u33_02ba_cpu *cpu, uint16_t back, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - back), value);
}

/* MOV DI,[BP+6]; CMP BYTE SS:[DI-back],value */
static uint8_t selector(pr_u33_02ba_cpu *cpu, const uint8_t *ram, uint16_t back, uint8_t value)
{
    uint8_t byte;
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    byte = read8(ram, cpu->ss, (uint16_t)(cpu->di - back));
    cmp8(cpu, byte, value);
    return byte;
}

#define TRY(call) do { pr_u33_02ba_status status_ = (call); if (status_ != PR_U33_02BA_OK) return status_; } while (0)

pr_u33_02ba_status pr_u33_stamp_block_02ba(uint8_t *ram, size_t ram_size, pr_u33_02ba_cpu *cpu)
{
    uint16_t entry;
    uint8_t value;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_02BA_RAM_SIZE)
        return PR_U33_02BA_ARGUMENT;
    entry = cpu->ip;
    enter_frame(ram, cpu);
    /* +02C0..+02EE: glyph 16h at rows 2..4, columns 0..5. */
    set_local(ram, cpu, 2u, 2u);
    for (;;) {
        set_local(ram, cpu, 1u, 0u);
        for (;;) {
            TRY(stamp(ram, ram_size, cpu, entry, 0x2Au, local(ram, cpu, 1u), 0, local(ram, cpu, 2u), 0x16u));
            value = local(ram, cpu, 1u);
            cmp8(cpu, value, 5u);
            if (value == 5u)
                break;
            set_local(ram, cpu, 1u, (uint8_t)(local(ram, cpu, 1u) + 1u));     /* +02CF */
        }
        value = local(ram, cpu, 2u);
        cmp8(cpu, value, 4u);
        if (value == 4u)
            break;
        set_local(ram, cpu, 2u, (uint8_t)(local(ram, cpu, 2u) + 1u));         /* +02C6 */
    }
    /* +02F0..+0345: selector 1 draws glyphs 4, 3 and 0Dh along rows 2..4. */
    if (selector(cpu, ram, 8u, 1u) == 1u) {
        set_local(ram, cpu, 2u, 2u);
        for (;;) {
            uint8_t row = local(ram, cpu, 2u);
            TRY(stamp(ram, ram_size, cpu, entry, 0x5Eu, (uint16_t)(row - 1u), 1, local(ram, cpu, 2u), 4u));
            TRY(stamp(ram, ram_size, cpu, entry, 0x70u, local(ram, cpu, 2u), 0, local(ram, cpu, 2u), 3u));
            row = local(ram, cpu, 2u);
            TRY(stamp(ram, ram_size, cpu, entry, 0x85u, (uint16_t)(row + 1u), 1, local(ram, cpu, 2u), 0x0Du));
            value = local(ram, cpu, 2u);
            cmp8(cpu, value, 4u);
            if (value == 4u)
                break;
            set_local(ram, cpu, 2u, (uint8_t)(local(ram, cpu, 2u) + 1u));     /* +0300 */
        }
    } else if (selector(cpu, ram, 8u, 3u) == 3u) {
        /* +0351..+036E: selector 3 draws glyph 8 at (2, 1) and 0 at (4, 5). */
        TRY(stamp(ram, ram_size, cpu, entry, 0xA7u, 1u, 0, 2u, 8u));
        TRY(stamp(ram, ram_size, cpu, entry, 0xB7u, 5u, 0, 4u, 0u));
    }
    leave(ram, cpu);
    return PR_U33_02BA_OK;
}
