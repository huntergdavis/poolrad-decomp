/* GAME.OVR unit 0x5E:+0x2879..+0x2907.
 * Evidence: docs/re/U5E_PARTY_VALUE_2879.md. */
#include "poolrad/recovered/overlay_u5e_party_value_2879.h"

#define RAM_MASK (PR_U5E_2879_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)((uint16_t)ram[at] |
                      ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static unsigned even_parity(uint8_t value)
{
    unsigned bits = 0u;
    while (value != 0u) {
        bits ^= value & 1u;
        value >>= 1u;
    }
    return bits == 0u;
}

static void result_flags(pr_u5e_2879_cpu *cpu, uint16_t result)
{
    uint16_t flags = (uint16_t)(cpu->flags & ~ARITH_FLAGS);
    if (even_parity((uint8_t)result))
        flags |= 0x0004u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    cpu->flags = flags;
}

static void add_flags(pr_u5e_2879_cpu *cpu, uint16_t left,
                      uint16_t right, uint16_t result)
{
    result_flags(cpu, result);
    if ((uint32_t)left + right > 0xFFFFu)
        cpu->flags |= 0x0001u;
    if (((left ^ right ^ result) & 0x0010u) != 0u)
        cpu->flags |= 0x0010u;
    if (((~(left ^ right) & (left ^ result)) & 0x8000u) != 0u)
        cpu->flags |= 0x0800u;
}

static void les_local(const uint8_t *ram, pr_u5e_2879_cpu *cpu)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp - 8u));
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & RAM_MASK);
}

static void finish(uint8_t *ram, pr_u5e_2879_cpu *cpu)
{
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->cs = read16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 4u);
}

pr_u5e_2879_status pr_u5e_party_value_2879(
    uint8_t *ram, size_t ram_size, pr_u5e_2879_cpu *cpu, size_t max_nodes)
{
    size_t nodes = 0u;
    uint16_t value;
    if (ram == NULL || ram_size != PR_U5E_2879_RAM_SIZE ||
        cpu == NULL || max_nodes == 0u)
        return PR_U5E_2879_ARGUMENT;

    /* +2879..+2886: far-call frame and eight SS-local bytes. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), 0u);

    /* +2889..+2893: read head words before storing either local word. */
    cpu->ax = read16(ram, cpu->ds, 0x5D96u);
    cpu->dx = read16(ram, cpu->ds, 0x5D98u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->dx);

    for (;;) {
        uint16_t segment_word;
        /* +2896..+289C: reread both SS pointer words every pass. */
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
        segment_word = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
        cpu->ax = (uint16_t)(cpu->ax | segment_word);
        result_flags(cpu, cpu->ax);
        if (cpu->ax == 0u)
            break;
        if (nodes == max_nodes) {
            cpu->ip = 0x289Eu;
            return PR_U5E_2879_LIMIT;
        }
        ++nodes;

        les_local(ram, cpu);
        if (read8(ram, cpu->es, (uint16_t)(cpu->di + 0x10Eu)) == 1u) {
            les_local(ram, cpu);
            if (read8(ram, cpu->es, (uint16_t)(cpu->di + 0x10Du)) != 0u) {
                les_local(ram, cpu);
                value = read8(ram, cpu->es,
                              (uint16_t)(cpu->di + 0x11Bu));
                cpu->ax = (uint16_t)(value + read16(ram, cpu->ss,
                                                 (uint16_t)(cpu->bp - 4u)));
                write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
            }
            les_local(ram, cpu);
            value = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x32u));
            cpu->ax = (uint16_t)(value + read16(ram, cpu->ss,
                                             (uint16_t)(cpu->bp - 2u)));
            write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);
        }
        les_local(ram, cpu);
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->dx);
    }

    /* +28E8..+2903: MUL's high word is explicitly discarded by XOR DX,DX.
     * The nonzero denominator is the branch guard for DIV. */
    value = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    result_flags(cpu, value); /* CMP value, 0 */
    if (value != 0u) {
        uint16_t numerator;
        uint16_t left;
        cpu->ax = 0x0014u;
        numerator = (uint16_t)(20u * read16(ram, cpu->ss,
                                           (uint16_t)(cpu->bp - 4u)));
        cpu->ax = numerator;
        cpu->dx = 0u;
        cpu->si = (uint16_t)(numerator / value);
        cpu->dx = (uint16_t)(numerator % value);
        left = (uint16_t)((uint16_t)(cpu->si << 1u) << 1u);
        cpu->ax = (uint16_t)(left + cpu->si);
        add_flags(cpu, left, cpu->si, cpu->ax);
        ram[physical(cpu->ds, 0x6E2Fu)] = (uint8_t)cpu->ax;
    }
    finish(ram, cpu);
    return PR_U5E_2879_OK;
}
