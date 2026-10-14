/* GAME.OVR unit 0x6A:+0824..+08BE. See docs/re/U6A_SORT_0824.md. */
#include "poolrad/recovered/overlay_u6a_sort_0824.h"

#define RAM_MASK (PR_U6A_0824_RAM_SIZE - 1u)
#define CMP_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

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

/* 8086 word memory operands use contiguous physical bytes at offset FFFF. */
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

static void les_list(uint8_t *ram, pr_u6a_0824_cpu *cpu)
{
    size_t address = physical(cpu->ds, 0x5D92u);
    cpu->di = read16_at(ram, address);
    cpu->es = read16_at(ram, (address + 2u) & RAM_MASK);
}

static void indexed_list(uint8_t *ram, pr_u6a_0824_cpu *cpu)
{
    les_list(ram, cpu);
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
}

static uint8_t local8(const uint8_t *ram, const pr_u6a_0824_cpu *cpu,
                      uint16_t distance)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local8(uint8_t *ram, const pr_u6a_0824_cpu *cpu,
                       uint16_t distance, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static void set_cmp_flags(pr_u6a_0824_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    uint8_t parity = result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    cpu->flags &= (uint16_t)~CMP_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    if (!(parity & 1u)) cpu->flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x0800u;
}

pr_u6a_0824_status pr_u6a_sort_0824(
    uint8_t *ram, size_t ram_size, pr_u6a_0824_cpu *cpu)
{
    if (ram == NULL || cpu == NULL || ram_size < PR_U6A_0824_RAM_SIZE)
        return PR_U6A_0824_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    set_local8(ram, cpu, 1u, 0u);

    for (;;) {
        uint8_t i;
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | local8(ram, cpu, 1u));
        i = (uint8_t)cpu->ax;
        set_cmp_flags(cpu, i, 20u);
        if (i <= 20u) {
            set_local8(ram, cpu, 2u, i);
            for (;;) {
                uint8_t current;
                uint8_t candidate;
                uint8_t j;

                /* +0842..+0853: key at index j; DL receives the key. */
                j = local8(ram, cpu, 2u);
                cpu->ax = j;
                indexed_list(ram, cpu);
                cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                    read8(ram, cpu->es, (uint16_t)(cpu->di + 0x17u)));
                cpu->ax &= 0xFF7Fu;
                candidate = (uint8_t)cpu->ax;
                cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | candidate);

                /* +0855..+0868: compare index i against the candidate. */
                cpu->ax = local8(ram, cpu, 1u);
                indexed_list(ram, cpu);
                cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                    read8(ram, cpu->es, (uint16_t)(cpu->di + 0x17u)));
                cpu->ax &= 0xFF7Fu;
                current = (uint8_t)cpu->ax;
                set_cmp_flags(cpu, current, candidate);

                if (current > candidate) {
                    /* +086A..+08A8: exchange the complete bytes. LES reads
                     * the pointer anew for each of the four accesses. */
                    cpu->ax = local8(ram, cpu, 1u);
                    indexed_list(ram, cpu);
                    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                        read8(ram, cpu->es, (uint16_t)(cpu->di + 0x17u)));
                    set_local8(ram, cpu, 3u, (uint8_t)cpu->ax);

                    cpu->ax = local8(ram, cpu, 2u);
                    indexed_list(ram, cpu);
                    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) |
                        read8(ram, cpu->es, (uint16_t)(cpu->di + 0x17u)));

                    cpu->ax = local8(ram, cpu, 1u);
                    indexed_list(ram, cpu);
                    write8(ram, cpu->es, (uint16_t)(cpu->di + 0x17u),
                           (uint8_t)cpu->dx);
                    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) |
                        local8(ram, cpu, 3u));
                    cpu->ax = local8(ram, cpu, 2u);
                    indexed_list(ram, cpu);
                    write8(ram, cpu->es, (uint16_t)(cpu->di + 0x17u),
                           (uint8_t)cpu->dx);
                }

                /* +08AC..+08B0: test j, then increment its stored byte. */
                j = local8(ram, cpu, 2u);
                set_cmp_flags(cpu, j, 20u);
                if (j == 20u) break;
                set_local8(ram, cpu, 2u, (uint8_t)(j + 1u));
            }
        }

        /* +08B2..+08B8: the final CMP supplies the returned flags. */
        i = local8(ram, cpu, 1u);
        set_cmp_flags(cpu, i, 19u);
        if (i == 19u) break;
        set_local8(ram, cpu, 1u, (uint8_t)(i + 1u));
    }

    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t address = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, address);
        cpu->cs = read16_at(ram, (address + 2u) & RAM_MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U6A_0824_OK;
}
