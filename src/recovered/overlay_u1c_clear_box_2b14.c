/* GAME.OVR unit 0x1C:+2B14..+2B32, complete 31-byte body. */
#include "poolrad/recovered/overlay_u1c_clear_box_2b14.h"

#define RAM_MASK (PR_U1C_2B14_RAM_SIZE - 1u)
#define INC_FLAGS 0x08D4u /* OF SF ZF AF PF; CF is unchanged */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u1c_2b14_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u1c_2b14_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void inc_word_flags(pr_u1c_2b14_cpu *cpu, uint16_t before,
                           uint16_t result)
{
    uint8_t bits = (uint8_t)result;
    cpu->flags &= (uint16_t)~INC_FLAGS;
    bits ^= (uint8_t)(bits >> 4);
    bits ^= (uint8_t)(bits >> 2);
    bits ^= (uint8_t)(bits >> 1);
    if (!(bits & 1u)) cpu->flags |= 0x0004u;
    if ((before & 0xFu) == 0xFu) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (before == 0x7FFFu) cpu->flags |= 0x0800u;
}

pr_u1c_2b14_status pr_u1c_clear_box_2b14(
    uint8_t *ram, size_t ram_size, pr_u1c_2b14_cpu *cpu,
    pr_u1c_2b14_display display, void *context)
{
    uint16_t pc;
    if (ram == NULL || cpu == NULL || ram_size < PR_U1C_2B14_RAM_SIZE)
        return PR_U1C_2B14_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    pc = read16(ram, cpu->ds, 0x49EDu);
    write16(ram, cpu->ds, 0x49EDu, (uint16_t)(pc + 1u));
    inc_word_flags(cpu, pc, (uint16_t)(pc + 1u));
    set_al(cpu, 1u);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x0Fu);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x0Fu);
    push16(ram, cpu, cpu->ax);
    cpu->ip = 0x2B27u;
    if (display == NULL || !display(context, ram, ram_size, cpu))
        return PR_U1C_2B14_CALL_ABORT;
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    cpu->ip = 0x2B2Cu;
    write8(ram, cpu->ds, 0x84DEu, 1u);
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->cs = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U1C_2B14_OK;
}
