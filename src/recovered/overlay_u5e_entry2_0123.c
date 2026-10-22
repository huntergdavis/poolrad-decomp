/* Original GAME.OVR U5E entry 2: +0123..+0190, 110 bytes. */
#include "poolrad/recovered/overlay_u5e_entry2_0123.h"

#define MASK (PR_U5E_0123_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define LOGIC_FLAGS 0x08C5u

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
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & MASK] << 8));
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
    ram[(a + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u5e_0123_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u ? 0x0004u : 0u;
}

static void sub_flags(pr_u5e_0123_cpu *cpu, uint16_t left,
                      uint16_t right, int byte_width)
{
    uint16_t mask = byte_width ? 0x00FFu : 0xFFFFu;
    uint16_t sign = byte_width ? 0x0080u : 0x8000u;
    uint16_t result = (uint16_t)((left - right) & mask);
    uint16_t flags = even_parity((uint8_t)result);
    if ((left & mask) < (right & mask)) flags |= 0x0001u;
    if (((left ^ right ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & sign) != 0u) flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & sign) != 0u)
        flags |= 0x0800u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | flags);
}

static void add_ax(pr_u5e_0123_cpu *cpu, uint16_t right)
{
    uint16_t left = cpu->ax;
    uint32_t wide = (uint32_t)left + right;
    uint16_t result = (uint16_t)wide;
    uint16_t flags = even_parity((uint8_t)result);
    if (wide > 0xFFFFu) flags |= 0x0001u;
    if (((left ^ right ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (((~(left ^ right)) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->ax = result;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | flags);
}

static void xor_ah_ah(pr_u5e_0123_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
    /* pinned FillFlags: XOR clears AF. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~(LOGIC_FLAGS | 0x10u)) |
                            0x0044u);
}

static void shl_ax_one(pr_u5e_0123_cpu *cpu)
{
    uint16_t before = cpu->ax;
    uint16_t result = (uint16_t)(before << 1);
    uint16_t carry = (before & 0x8000u) != 0u ? 1u : 0u;
    uint16_t flags = even_parity((uint8_t)result) | carry;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (((result >> 15) ^ carry) != 0u) flags |= 0x0800u;
    /* pinned DOSBox-X: SHL sets AF for a non-zero count. */
    cpu->ax = result;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~(LOGIC_FLAGS | 0x10u)) | flags | 0x10u);
}

pr_u5e_0123_status pr_u5e_entry2_0123(
    uint8_t *ram, size_t ram_size, pr_u5e_0123_cpu *cpu,
    const pr_u5e_0123_io *io)
{
    uint16_t source_cs, v;
    uint8_t local;
    if (ram == NULL || cpu == NULL || io == NULL ||
        ram_size != PR_U5E_0123_RAM_SIZE || cpu->ip != 0x0123u)
        return PR_U5E_0123_ARGUMENT;
    source_cs = cpu->cs;

    /* +0123..+0137: character byte and early conditional. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    v = cpu->sp;
    sub_flags(cpu, v, 2u, 0);
    cpu->sp = (uint16_t)(v - 2u);
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->es, (uint16_t)(cpu->di + 0x011Cu)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    local = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x010Eu));
    sub_flags(cpu, local, 0u, 1);
    if (local == 0u) {
        /* +013F..+014D: add a DS-selected object's word and keep AL. */
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                             read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
        xor_ah_ah(cpu);
        cpu->di = read16(ram, cpu->ds, 0x49D6u);
        cpu->es = read16(ram, cpu->ds, 0x49D8u);
        add_ax(cpu, read16(ram, cpu->es,
                           (uint16_t)(cpu->di + 0x06E4u)));
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
    }

    /* +0150..+016C: mark, clamp to [1,96], double and publish low byte. */
    write8(ram, cpu->ds, 0x681Du, 1u);
    local = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    sub_flags(cpu, local, 1u, 1);
    if (local < 1u) {
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);
    } else {
        local = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        sub_flags(cpu, local, 0x60u, 1);
        if (local > 0x60u)
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);
    }
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    xor_ah_ah(cpu);
    shl_ax_one(cpu);
    write8(ram, cpu->ds, 0x681Au, (uint8_t)cpu->ax);

    /* +016F..+0178: character pointer and low byte 0x12 to far callee. */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x0012u);
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    push16(ram, cpu, source_cs);
    push16(ram, cpu, 0x017Du);
    cpu->cs = io->loaded_callee_cs;
    cpu->ip = 0x002Fu;
    if (io->call == NULL) return PR_U5E_0123_CALLEE_REQUIRED;
    if (!io->call(io->context, ram, ram_size, cpu))
        return PR_U5E_0123_CALLEE_FAILED;
    if (cpu->cs != source_cs || cpu->ip != 0x017Du)
        return PR_U5E_0123_TRANSFER;

    /* +017D..+0190: publish return value and outer RETF 4. */
    write8(ram, cpu->ds, 0x681Du, 0u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ds, 0x681Au));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->cs = read16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U5E_0123_RETURNED;
}
