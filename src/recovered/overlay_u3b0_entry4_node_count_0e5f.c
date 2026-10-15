/* DOS v1.3 GAME.OVR U3B0 entry4 +0E5F..+0E96, 56 original bytes. */
#include "poolrad/recovered/overlay_u3b0_entry4_node_count_0e5f.h"

#define MASK (PR_U3B0_0DEA_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

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

static uint8_t even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void logic16_flags(pr_u3b0_0dea_cpu *cpu, uint16_t value)
{
    uint16_t flags = cpu->flags & (uint16_t)~ARITH_FLAGS;
    if (even_parity((uint8_t)value)) flags |= 0x0004u;
    if (value == 0u) flags |= 0x0040u;
    if ((value & 0x8000u) != 0u) flags |= 0x0080u;
    cpu->flags = flags;
}

static void inc16_flags(pr_u3b0_0dea_cpu *cpu, uint16_t before,
                        uint16_t result)
{
    uint16_t flags = cpu->flags &
                     (uint16_t)~(ARITH_FLAGS & (uint16_t)~0x0001u);
    if (even_parity((uint8_t)result)) flags |= 0x0004u;
    if (((before ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (before == 0x7FFFu) flags |= 0x0800u;
    cpu->flags = flags;
}

pr_u3b0_0e5f_status pr_u3b0_entry4_node_count_0e5f(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dea_cpu *cpu,
    size_t max_traversals)
{
    size_t traversals = 0u;
    uint16_t bp, before, next;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0DEA_RAM_SIZE ||
        cpu->ip != 0x0E5Fu || max_traversals == 0u)
        return PR_U3B0_0E5F_ARGUMENT;

    bp = cpu->bp;
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(bp + 0x14u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(bp + 0x16u));
    write16(ram, cpu->ss, (uint16_t)(bp - 0x89u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(bp - 0x87u), cpu->dx);
    cpu->ax = 0u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x0044u);
    write16(ram, cpu->ss, (uint16_t)(bp - 0x8Du), cpu->ax);

    for (;;) {
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(bp - 0x89u));
        cpu->ax = (uint16_t)(cpu->ax |
                             read16(ram, cpu->ss, (uint16_t)(bp - 0x87u)));
        logic16_flags(cpu, cpu->ax);
        if (cpu->ax == 0u) {
            cpu->ip = 0x0E97u;
            return PR_U3B0_0E5F_OK;
        }
        if (traversals >= max_traversals) {
            cpu->ip = 0x0E7Du;
            return PR_U3B0_0E5F_LIMIT;
        }

        cpu->di = read16(ram, cpu->ss, (uint16_t)(bp - 0x89u));
        cpu->es = read16(ram, cpu->ss, (uint16_t)(bp - 0x87u));
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
        write16(ram, cpu->ss, (uint16_t)(bp - 0x89u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(bp - 0x87u), cpu->dx);
        before = read16(ram, cpu->ss, (uint16_t)(bp - 0x8Du));
        next = (uint16_t)(before + 1u);
        write16(ram, cpu->ss, (uint16_t)(bp - 0x8Du), next);
        inc16_flags(cpu, before, next);
        cpu->ip = 0x0E73u;
        ++traversals;
    }
}
