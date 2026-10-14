/* GAME.OVR unit 0x1C:+2AA2..+2AC0, complete 31-byte body. */
#include "poolrad/recovered/overlay_u1c_sprite_off_2aa2.h"

#define RAM_MASK (PR_U1C_2AA2_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
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

static void cmp_zero(pr_u1c_2aa2_cpu *cpu, uint8_t value)
{
    uint8_t bits = value;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    bits ^= (uint8_t)(bits >> 4);
    bits ^= (uint8_t)(bits >> 2);
    bits ^= (uint8_t)(bits >> 1);
    if (!(bits & 1u)) cpu->flags |= 0x0004u;
    if (value == 0u) cpu->flags |= 0x0040u;
    if (value & 0x80u) cpu->flags |= 0x0080u;
}

pr_u1c_2aa2_status pr_u1c_sprite_off_2aa2(
    uint8_t *ram, size_t ram_size, pr_u1c_2aa2_cpu *cpu,
    pr_u1c_2aa2_view view, void *context)
{
    uint16_t pc;
    uint8_t dirty;
    if (ram == NULL || cpu == NULL || ram_size < PR_U1C_2AA2_RAM_SIZE)
        return PR_U1C_2AA2_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u); /* push bp */
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    pc = read16(ram, cpu->ds, 0x49EDu);
    write16(ram, cpu->ds, 0x49EDu, (uint16_t)(pc + 1u));
    dirty = read8(ram, cpu->ds, 0x84E3u);
    cmp_zero(cpu, dirty);
    if (dirty != 0u) {
        cpu->ip = 0x2AB0u;
        if (view == NULL || !view(context, ram, ram_size, cpu))
            return PR_U1C_2AA2_CALL_ABORT;
        cpu->ip = 0x2AB5u;
        write8(ram, cpu->ds, 0x84E3u, 0u);
        write8(ram, cpu->ds, 0x84E0u, 0u);
    }
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->cs = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U1C_2AA2_OK;
}
