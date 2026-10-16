/* GAME.OVR U9E entry8 +0E1F..+0E2D; see shared-tail evidence note. */
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_tail_0e1f.h"

#define MASK (PR_U9E_ENTRY8_TAIL_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static int write_aliases_code(const pr_u9e_entry8_tail_cpu *cpu)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp - 1u));
    unsigned offset;
    for (offset = 0x0DE3u; offset <= 0x0E2Du; ++offset)
        if (at == physical(cpu->cs, (uint16_t)offset)) return 1;
    return 0;
}

pr_u9e_entry8_tail_status pr_u9e_entry8_fullstate_tail_0e1f(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_tail_cpu *cpu)
{
    uint8_t al;
    uint16_t bp, sp, return_ip, return_cs;
    if (ram == NULL || cpu == NULL ||
        ram_size != PR_U9E_ENTRY8_TAIL_RAM_SIZE || cpu->ip != 0x0E1Fu ||
        write_aliases_code(cpu))
        return PR_U9E_ENTRY8_TAIL_ARGUMENT;

    bp = cpu->bp;
    /* +0E1F MOV AL,[BP-3]; +0E22 MOV [BP-1],AL;
     * +0E25 MOV AL,[BP-1]. Preserve the live read/write order. */
    al = read8(ram, cpu->ss, (uint16_t)(bp - 3u));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | al);
    ram[physical(cpu->ss, (uint16_t)(bp - 1u))] = al;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(bp - 1u)));

    /* +0E28 MOV SP,BP; +0E2A POP BP; +0E2B RETF 4. */
    sp = bp;
    cpu->bp = read16(ram, cpu->ss, sp);
    sp = (uint16_t)(sp + 2u);
    return_ip = read16(ram, cpu->ss, sp);
    return_cs = read16(ram, cpu->ss, (uint16_t)(sp + 2u));
    cpu->sp = (uint16_t)(sp + 8u);
    cpu->ip = return_ip;
    cpu->cs = return_cs;
    return PR_U9E_ENTRY8_TAIL_RETURNED;
}
