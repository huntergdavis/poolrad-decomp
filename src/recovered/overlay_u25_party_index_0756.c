/* GAME.OVR unit 0x25:+0x0756..+0x07BB. Evidence:
 * docs/re/U25_PARTY_INDEX_0756.md. */
#include "poolrad/recovered/overlay_u25_party_index_0756.h"

#define RAM_MASK (PR_U25_0756_RAM_SIZE - 1u)
#define DEFINED_OR_FLAGS 0x08C5u /* OF SF ZF PF CF; AF is undefined */
#define ARITH_FLAGS 0x08D5u      /* OF SF ZF AF PF CF */

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

/* A 16-bit operand at offset FFFF reads two contiguous physical bytes. */
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

static uint16_t compare16(uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    uint16_t flags = 0u;
    uint8_t low = (uint8_t)result;
    uint8_t parity = low;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (left < right) flags |= 0x0001u;
    if (!(parity & 1u)) flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    return flags;
}

static uint16_t compare8_zero(uint8_t value)
{
    uint8_t parity = value;
    uint16_t flags = 0u;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (!(parity & 1u)) flags |= 0x0004u;
    if (value == 0u) flags |= 0x0040u;
    if ((value & 0x80u) != 0u) flags |= 0x0080u;
    return flags;
}

pr_u25_0756_status pr_u25_party_index_0756(
    uint8_t *ram, size_t ram_size, pr_u25_0756_cpu *cpu)
{
    if (ram == NULL || ram_size < PR_U25_0756_RAM_SIZE || cpu == NULL)
        return PR_U25_0756_ARGUMENT;

    /* +0756..+076D: frame, copy the head, clear found and index locals. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    cpu->ax = read16(ram, cpu->ds, 0x5D96u);
    cpu->dx = read16(ram, cpu->ds, 0x5D98u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->dx);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 7u), 0u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0u);

    for (;;) {
        uint16_t operand;
        uint16_t result;
        uint8_t parity;

        /* +0771..+077D: current pointer is read anew after every store. */
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
        operand = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
        result = (uint16_t)(cpu->ax | operand);
        cpu->ax = result;
        parity = (uint8_t)result;
        parity ^= (uint8_t)(parity >> 4);
        parity ^= (uint8_t)(parity >> 2);
        parity ^= (uint8_t)(parity >> 1);
        cpu->flags &= (uint16_t)~DEFINED_OR_FLAGS;
        if (!(parity & 1u)) cpu->flags |= 0x0004u;
        if (result == 0u) cpu->flags |= 0x0040u;
        if ((result & 0x8000u) != 0u) cpu->flags |= 0x0080u;
        if (result == 0u)
            break;
        operand = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 7u));
        cpu->flags = (uint16_t)((cpu->flags & ~ARITH_FLAGS) |
                                compare8_zero((uint8_t)operand));
        if (operand != 0u)
            break;

        /* +077F..+0793: compare far argument, segment first. */
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
        cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
        cpu->flags = (uint16_t)((cpu->flags & ~ARITH_FLAGS) |
                        compare16(cpu->dx,
                                  read16(ram, cpu->ss,
                                         (uint16_t)(cpu->bp + 8u))));
        if (!(cpu->flags & 0x0040u)) {
            /* +0795..+07AB: count and follow the link at node +0x104. */
            goto next_node;
        }
        cpu->flags = (uint16_t)((cpu->flags & ~ARITH_FLAGS) |
                        compare16(cpu->ax,
                                  read16(ram, cpu->ss,
                                         (uint16_t)(cpu->bp + 6u))));
        if (cpu->flags & 0x0040u) {
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 7u), 1u);
            continue;
        }

next_node:
        operand = (uint16_t)(cpu->bp - 2u);
        write8(ram, cpu->ss, operand,
               (uint8_t)(read8(ram, cpu->ss, operand) + 1u));
        /* LES reads both words contiguously from the local far pointer. */
        {
            size_t address = physical(cpu->ss, (uint16_t)(cpu->bp - 6u));
            cpu->di = read16_at(ram, address);
            cpu->es = read16_at(ram, (address + 2u) & RAM_MASK);
        }
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->dx);
    }

    /* +07AD..+07BB: copy count through a local and RETF 4. */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
              read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
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
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U25_0756_OK;
}
