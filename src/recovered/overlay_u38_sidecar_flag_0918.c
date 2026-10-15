/* GAME.OVR unit 0x38 entry 22, +0918..+0944 (47 original bytes). */
#include "poolrad/recovered/overlay_u38_sidecar_flag_0918.h"

#define RAM_MASK (PR_U38_0918_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u38_0918_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u38_0918_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void sub16_sp(pr_u38_0918_cpu *cpu, uint16_t value)
{
    uint16_t before = cpu->sp;
    uint16_t result = (uint16_t)(before - value);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < value) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ value ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((before ^ value) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    cpu->sp = result;
}

static void or_al_al(pr_u38_0918_cpu *cpu)
{
    uint8_t value = (uint8_t)cpu->ax;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity(value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x80u) cpu->flags |= 0x80u;
}

static void sub_byte(uint8_t *ram, pr_u38_0918_cpu *cpu,
                     uint16_t offset, uint8_t value)
{
    uint8_t before = read8(ram, cpu->ds, offset);
    uint8_t result = (uint8_t)(before - value);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < value) cpu->flags |= 1u;
    if (parity(result)) cpu->flags |= 4u;
    if ((before ^ value ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x80u) cpu->flags |= 0x80u;
    if (((before ^ value) & (before ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x800u;
    write8(ram, cpu->ds, offset, result);
}

pr_u38_0918_status pr_u38_sidecar_flag_0918(
    uint8_t *ram, size_t ram_size, pr_u38_0918_cpu *cpu,
    pr_u38_0918_lookup lookup, void *context)
{
    if (ram == NULL || cpu == NULL || lookup == NULL ||
        ram_size < PR_U38_0918_RAM_SIZE || cpu->ip != 0x0918u)
        return PR_U38_0918_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub16_sp(cpu, 4u);
    push16(ram, cpu, read16(ram, cpu->ds, 0x5D94u));
    push16(ram, cpu, read16(ram, cpu->ds, 0x5D92u));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x18u);
    push16(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 4u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->ip = 0x092Eu;
    if (!lookup(context, ram, ram_size, cpu)) return PR_U38_0918_CALL_ABORT;
    cpu->sp = (uint16_t)(cpu->sp + 10u); /* BA:2199 RETF 10 */
    cpu->ip = 0x0933u;
    or_al_al(cpu);
    if ((cpu->ax & 0xFFu) == 0u) write8(ram, cpu->ds, 0x681Eu, 1u);
    sub_byte(ram, cpu, 0x6822u, 4u);
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_0918_OK;
}
