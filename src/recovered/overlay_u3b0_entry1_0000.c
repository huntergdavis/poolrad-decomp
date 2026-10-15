/* GAME.OVR U3B0 public entry 1, +0000..+0066 (103 original bytes). */
#include "poolrad/recovered/overlay_u3b0_entry1_0000.h"

#define RAM_MASK (PR_U3B0_0000_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void les_local(const uint8_t *ram, pr_u3b0_0000_cpu *cpu)
{
    size_t a = physical(cpu->ss, (uint16_t)(cpu->bp - 8u));
    cpu->di = (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
    cpu->es = (uint16_t)(ram[(a + 2u) & RAM_MASK] |
                         ((uint16_t)ram[(a + 3u) & RAM_MASK] << 8));
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void cmp16(pr_u3b0_0000_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & 0x8000u) != 0u) cpu->flags |= 0x80u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
}

static void xor_ax_ax(pr_u3b0_0000_cpu *cpu)
{
    cpu->ax = 0u;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= 0x0044u; /* ZF, PF */
}

static uint16_t pop16(const uint8_t *ram, pr_u3b0_0000_cpu *cpu)
{
    uint16_t result = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return result;
}

pr_u3b0_0000_status pr_u3b0_entry1_0000(
    uint8_t *ram, size_t ram_size, pr_u3b0_0000_cpu *cpu)
{
    uint16_t head_off, head_seg, count, target, next_off, next_seg;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0000_RAM_SIZE ||
        cpu->ip != 0u)
        return PR_U3B0_0000_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u); /* PUSH BP */
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 10u);
    xor_ax_ax(cpu);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 10u), cpu->ax);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->dx);

    for (;;) {
        head_off = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
        head_seg = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
        cpu->ax = (uint16_t)(head_off | head_seg);
        if (cpu->ax == 0u)
            break;
        count = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 10u));
        cpu->ax = count;
        target = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 10u));
        if (count == target)
            break;
        les_local(ram, cpu);
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
        next_off = cpu->ax;
        next_seg = cpu->dx;
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), next_off);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), next_seg);
        count = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 10u));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 10u),
                (uint16_t)(count + 1u));
    }

    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 10u));
    target = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 10u));
    cmp16(cpu, cpu->ax, target);
    if (cpu->ax == target) {
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
        cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    } else {
        xor_ax_ax(cpu);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);
    }
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_U3B0_0000_OK;
}
