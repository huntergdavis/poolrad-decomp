/* GAME.OVR unit 0x7F:+05E0..+0639. See docs/re/U7F_PARTY_COUNT_05E0.md. */
#include "poolrad/recovered/overlay_u7f_party_count_05e0.h"

#define RAM_MASK (PR_U7F_05E0_RAM_SIZE - 1u)
#define DEFINED_OR_FLAGS 0x08C5u /* OF SF ZF PF CF; AF is undefined */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
}

static void write8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

/* 8086 word operands cross offset FFFF as contiguous physical bytes. */
static uint16_t read16_at(const uint8_t *ram, size_t address)
{
    return (uint16_t)(ram[address] |
                      ((uint16_t)ram[(address + 1u) & RAM_MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return read16_at(ram, physical(seg, off));
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t address = physical(seg, off);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void set_or_flags(pr_u7f_05e0_cpu *cpu, uint16_t result)
{
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    cpu->flags &= (uint16_t)~DEFINED_OR_FLAGS;
    if (!(parity & 1u)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
}

/* LES uses one contiguous four-byte memory operand, including across FFFF. */
static void les_local(uint8_t *ram, pr_u7f_05e0_cpu *cpu)
{
    size_t address = physical(cpu->ss, (uint16_t)(cpu->bp - 5u));
    cpu->di = read16_at(ram, address);
    cpu->es = read16_at(ram, (address + 2u) & RAM_MASK);
}

pr_u7f_05e0_status pr_u7f_party_count_05e0(
    uint8_t *ram, size_t ram_size, pr_u7f_05e0_cpu *cpu)
{
    if (ram == NULL || cpu == NULL || ram_size < PR_U7F_05E0_RAM_SIZE)
        return PR_U7F_05E0_ARGUMENT;

    /* +05E0..+05F3: frame, party head, and byte counter. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 6u);
    cpu->ax = read16(ram, cpu->ds, 0x5D96u);
    cpu->dx = read16(ram, cpu->ds, 0x5D98u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), cpu->dx);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), 0u);

    for (;;) {
        uint16_t current_segment;
        uint8_t status;
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u));
        current_segment = read16(ram, cpu->ss,
                                 (uint16_t)(cpu->bp - 3u));
        cpu->ax = (uint16_t)(cpu->ax | current_segment);
        set_or_flags(cpu, cpu->ax);
        if (cpu->ax == 0u)
            break;

        les_local(ram, cpu);
        status = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x84u));
        if (status != 0u) {
            les_local(ram, cpu);
            status = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x84u));
        }
        if (status == 0u || status == 0xB3u) {
            uint16_t local = (uint16_t)(cpu->bp - 6u);
            write8(ram, cpu->ss, local,
                   (uint8_t)(read8(ram, cpu->ss, local) + 1u));
        }

        les_local(ram, cpu);
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), cpu->dx);
    }

    /* +062D..+0639: copy count through a scratch byte and far return. */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 6u)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t address = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, address);
        cpu->cs = read16_at(ram, (address + 2u) & RAM_MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U7F_05E0_OK;
}
