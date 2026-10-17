/* GAME.OVR unit 0x6A:+019C..+0213; docs/re/U6A_LINK_HIGH_CLEAR_019C.md. */
#include "poolrad/recovered/overlay_u6a_link_high_clear_019c.h"
#include "poolrad/recovered/overlay_u86_record_predicate_323d.h"

#include <string.h>

#define MASK (PR_U6A_019C_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */
#define CALLEE_IP 0x323Du   /* unit 0x86 entry 6 behind stub 0096:003E */
#define RETURN_DELTA 0x2Cu /* the far call site plus five, from the entry offset */

_Static_assert(sizeof(pr_u6a_019c_cpu) == sizeof(pr_u86_323d_cpu),
               "u86 +323D CPU state layout differs");

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

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

/* LES DI,SS:[offset]: both words are physically contiguous. */
static void les(const uint8_t *ram, pr_u6a_019c_cpu *cpu, uint16_t offset)
{
    size_t at = physical(cpu->ss, offset);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
}

static void push16(uint8_t *ram, pr_u6a_019c_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u6a_019c_cpu *cpu, uint8_t value)
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

/* OR flags: CF/OF clear, undefined AF represented as zero. */
static void or_flags(pr_u6a_019c_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

/* +01C3: LCALL 0096:003E. The loaded stub jumps to unit 0x86 +323D; the
 * callee sees the pushed CS:+01C8 and consumes it with RETF 4. */
static pr_u6a_019c_status call_323d(uint8_t *ram, size_t ram_size, pr_u6a_019c_cpu *cpu,
                                    uint16_t return_ip)
{
    pr_u86_323d_cpu leaf;
    uint16_t caller_cs = cpu->cs;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = CALLEE_IP;
    memcpy(&leaf, cpu, sizeof leaf);
    if (pr_u86_record_predicate_323d(ram, ram_size, &leaf) != PR_U86_323D_OK)
        return PR_U6A_019C_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U6A_019C_TRANSFER;
    return PR_U6A_019C_OK;
}

pr_u6a_019c_status pr_u6a_link_high_clear_019c(
    uint8_t *ram, size_t ram_size, pr_u6a_019c_cpu *cpu)
{
    uint16_t return_ip;
    if (ram == NULL || cpu == NULL || ram_size < PR_U6A_019C_RAM_SIZE)
        return PR_U6A_019C_ARGUMENT;
    /* The loaded game enters at offset 0 of its unit; the pushed return offset follows the entry offset. */
    return_ip = (uint16_t)(cpu->ip + RETURN_DELTA);

    /* +019C..+019F: frame with six local bytes. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 6u);

    /* +01A2..+01B2: current := record^.link at +C8/+CA. */
    les(ram, cpu, (uint16_t)(cpu->bp + 6u));
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xC8u));
    cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCAu));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), cpu->dx);

    for (;;) {
        pr_u6a_019c_status status;

        /* +01B5..+01BB: both pointer words zero ends the walk. */
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u));
        cpu->ax = (uint16_t)(cpu->ax | read16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u)));
        or_flags(cpu, cpu->ax, 0x8000u);
        if (cpu->ax == 0u)
            break;

        /* +01BD..+01C3: push segment then offset, rereading after each push. */
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u)));
        status = call_323d(ram, ram_size, cpu, return_ip);
        if (status != PR_U6A_019C_OK)
            return status;

        /* +01C8..+01CA: OR AL,AL on the predicate result. */
        or_flags(cpu, (uint8_t)cpu->ax, 0x80u);
        if ((uint8_t)cpu->ax != 0u) {
            /* +01CC: index := 1; +01D2..+01F9: clear bit 7 of node +3B+index. */
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);
            for (;;) {
                set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
                cpu->ax &= 0x00FFu;
                les(ram, cpu, (uint16_t)(cpu->bp - 5u));
                cpu->di = (uint16_t)(cpu->di + cpu->ax);
                set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x3Bu)));
                set_al(cpu, (uint8_t)(cpu->ax & 0x7Fu));
                cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)cpu->ax);
                set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
                cpu->ax &= 0x00FFu;
                les(ram, cpu, (uint16_t)(cpu->bp - 5u));
                cpu->di = (uint16_t)(cpu->di + cpu->ax);
                write8(ram, cpu->es, (uint16_t)(cpu->di + 0x3Bu), (uint8_t)cpu->dx);
                /* +01F5..+01F9: the live index byte decides. */
                if (read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)) == 3u)
                    break;
                write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u),
                       (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)) + 1u));
            }
        }

        /* +01FB..+020C: current := current^.link at +2A/+2C. */
        les(ram, cpu, (uint16_t)(cpu->bp - 5u));
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), cpu->dx);
    }

    /* +020E..+0211: MOV SP,BP; POP BP; RETF 4. */
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U6A_019C_OK;
}
