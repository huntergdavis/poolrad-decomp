/* GAME.OVR unit 0x1C:+0A9F..+0AEC, complete 78-byte body. */
#include "poolrad/recovered/overlay_u1c_return_0a9f.h"

#define RAM_MASK (PR_U1C_0A9F_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */
#define INC_FLAGS 0x08D4u   /* OF SF ZF AF PF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u1c_0a9f_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void parity(pr_u1c_0a9f_cpu *cpu, uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    if (!(value & 1u)) cpu->flags |= 0x0004u;
}

static void inc_pc(pr_u1c_0a9f_cpu *cpu, uint16_t before,
                   uint16_t result)
{
    cpu->flags &= (uint16_t)~INC_FLAGS;
    parity(cpu, (uint8_t)result);
    if ((before & 0xFu) == 0xFu) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (before == 0x7FFFu) cpu->flags |= 0x0800u;
}

static void or_head_flags(pr_u1c_0a9f_cpu *cpu, uint16_t result)
{
    /* AF is undefined after OR and excluded by the differential audit. */
    cpu->flags &= (uint16_t)~(ARITH_FLAGS & ~0x0010u);
    parity(cpu, (uint8_t)result);
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
}

static void les_head(const uint8_t *ram, pr_u1c_0a9f_cpu *cpu)
{
    cpu->di = read16(ram, cpu->ds, 0x6F70u);
    cpu->es = read16(ram, cpu->ds, 0x6F72u);
}

pr_u1c_0a9f_status pr_u1c_return_0a9f(
    uint8_t *ram, size_t ram_size, pr_u1c_0a9f_cpu *cpu,
    pr_u1c_0a9f_free free_node, void *context)
{
    uint16_t pc, segment;
    if (ram == NULL || cpu == NULL || ram_size < PR_U1C_0A9F_RAM_SIZE)
        return PR_U1C_0A9F_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    pc = read16(ram, cpu->ds, 0x49EDu);
    write16(ram, cpu->ds, 0x49EDu, (uint16_t)(pc + 1u));
    inc_pc(cpu, pc, (uint16_t)(pc + 1u));
    cpu->ax = read16(ram, cpu->ds, 0x6F70u);
    segment = read16(ram, cpu->ds, 0x6F72u);
    cpu->ax = (uint16_t)(cpu->ax | segment);
    or_head_flags(cpu, cpu->ax);
    if (cpu->ax != 0u) {
        les_head(ram, cpu);
        cpu->ax = read16(ram, cpu->es, cpu->di);
        write16(ram, cpu->ds, 0x49EDu, cpu->ax);
        les_head(ram, cpu);
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 2u));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 4u));
        write16(ram, cpu->ds, 0x6F74u, cpu->ax);
        write16(ram, cpu->ds, 0x6F76u, cpu->dx);
        push16(ram, cpu, read16(ram, cpu->ds, 0x6F72u));
        push16(ram, cpu, read16(ram, cpu->ds, 0x6F70u));
        cpu->ax = 6u;
        push16(ram, cpu, cpu->ax);
        cpu->ip = 0x0AD8u;
        if (free_node == NULL || !free_node(context, ram, ram_size, cpu))
            return PR_U1C_0A9F_CALL_ABORT;
        cpu->sp = (uint16_t)(cpu->sp + 6u);
        cpu->ip = 0x0ADDu;
        cpu->ax = read16(ram, cpu->ds, 0x6F74u);
        cpu->dx = read16(ram, cpu->ds, 0x6F76u);
        write16(ram, cpu->ds, 0x6F70u, cpu->ax);
        write16(ram, cpu->ds, 0x6F72u, cpu->dx);
    }
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->cs = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U1C_0A9F_OK;
}
