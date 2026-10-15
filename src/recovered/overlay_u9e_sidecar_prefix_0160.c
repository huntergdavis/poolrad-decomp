/* GAME.OVR unit 0x9E:+0160..+018A, before resident sidecar lookup. */
#include "poolrad/recovered/overlay_u9e_sidecar_prefix_0160.h"

#define RAM_MASK (PR_U9E_0160_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

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

static void push16(uint8_t *ram, pr_u9e_0160_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void sub_sp_e4(pr_u9e_0160_cpu *cpu)
{
    uint16_t before = cpu->sp;
    uint16_t result = (uint16_t)(before - 0x00E4u);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < 0x00E4u) cpu->flags |= 1u;
    if (even_parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ 0x00E4u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((before ^ 0x00E4u) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    cpu->sp = result;
}

pr_u9e_0160_status pr_u9e_sidecar_prefix_0160(
    uint8_t *ram, size_t ram_size, pr_u9e_0160_cpu *cpu)
{
    uint16_t value;
    if (ram == NULL || cpu == NULL || ram_size < PR_U9E_0160_RAM_SIZE ||
        cpu->ip != 0x0160u)
        return PR_U9E_0160_ARGUMENT;

    push16(ram, cpu, cpu->bp);                  /* +0160 */
    cpu->bp = cpu->sp;                         /* +0161 */
    sub_sp_e4(cpu);                            /* +0163 */
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x00E1u))] = 0u; /* +0167 */
    cpu->ax = read16(ram, cpu->ds, 0x5D96u);  /* +016C */
    cpu->dx = read16(ram, cpu->ds, 0x5D98u);  /* +016F */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x00DCu), cpu->ax); /* +0173 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x00DAu), cpu->dx); /* +0177 */
    value = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x000Au));
    push16(ram, cpu, value);                   /* +017B */
    value = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0008u));
    push16(ram, cpu, value);                   /* +017E */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))]); /* +0181 */
    push16(ram, cpu, cpu->ax);                 /* +0184 */
    cpu->di = (uint16_t)(cpu->bp - 0x00E0u);  /* +0185 */
    push16(ram, cpu, cpu->ss);                 /* +0189 */
    push16(ram, cpu, cpu->di);                 /* +018A */
    cpu->ip = 0x018Bu;
    return PR_U9E_0160_PREFIX_DONE;
}
