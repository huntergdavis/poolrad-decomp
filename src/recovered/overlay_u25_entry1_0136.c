/* Original GAME.OVR U25 entry 1: +0136..+01B5, 128 bytes. */
#include "poolrad/recovered/overlay_u25_entry1_0136.h"

#define MASK (PR_U25_0136_RAM_SIZE - 1u)
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

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void push16(uint8_t *ram, pr_u25_0136_cpu *cpu, uint16_t value)
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

static void sub_flags(pr_u25_0136_cpu *cpu, uint16_t left,
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

static void cmp_al(pr_u25_0136_cpu *cpu, uint8_t value)
{
    sub_flags(cpu, (uint8_t)cpu->ax, value, 1);
}

static void xor_ah_ah(pr_u25_0136_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
    /* AF is undefined; retain its incoming bit by convention. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~LOGIC_FLAGS) |
                            0x0044u);
}

static void add_ax_dx(pr_u25_0136_cpu *cpu)
{
    uint16_t left = cpu->ax, right = cpu->dx;
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

static void shl_ax_eight(pr_u25_0136_cpu *cpu)
{
    uint16_t before = cpu->ax;
    uint16_t result = (uint16_t)(before << 8);
    uint16_t flags = even_parity((uint8_t)result);
    if ((before & 0x0100u) != 0u) flags |= 0x0001u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    /* AF/OF are undefined for this eight-bit count; retain inputs. */
    cpu->ax = result;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x00C5u) | flags);
}

static pr_u25_0136_status call_local(
    uint8_t *ram, size_t ram_size, pr_u25_0136_cpu *cpu,
    const pr_u25_0136_io *io, pr_u25_0136_call_kind kind,
    uint16_t target, uint16_t return_ip,
    pr_u25_0136_call_kind *stopped_at)
{
    uint16_t source_cs = cpu->cs;
    push16(ram, cpu, source_cs); /* explicit PUSH CS */
    push16(ram, cpu, return_ip); /* CALL near */
    cpu->ip = target;
    if (stopped_at != NULL) *stopped_at = kind;
    if (io->call == NULL) return PR_U25_0136_CALLEE_REQUIRED;
    if (!io->call(io->context, kind, ram, ram_size, cpu))
        return PR_U25_0136_CALLEE_FAILED;
    if (cpu->cs != source_cs || cpu->ip != return_ip)
        return PR_U25_0136_TRANSFER;
    return PR_U25_0136_RETURNED;
}

pr_u25_0136_status pr_u25_entry1_0136(
    uint8_t *ram, size_t ram_size, pr_u25_0136_cpu *cpu,
    const pr_u25_0136_io *io, pr_u25_0136_call_kind *stopped_at)
{
    uint8_t type;
    uint16_t before;
    pr_u25_0136_status status;
    if (ram == NULL || cpu == NULL || io == NULL ||
        ram_size != PR_U25_0136_RAM_SIZE || cpu->ip != 0x0136u)
        return PR_U25_0136_ARGUMENT;

    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    before = cpu->sp;
    sub_flags(cpu, before, 10u, 0);
    cpu->sp = (uint16_t)(before - 10u);

    /* Three independent MOV AL; CBW; MOV DI,AX table loads. */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    cpu->ax = (uint8_t)cpu->ax >= 0x80u ?
              (uint16_t)(0xFF00u | (uint8_t)cpu->ax) : (uint8_t)cpu->ax;
    cpu->di = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ds,
                               (uint16_t)(cpu->di + 0x7085u)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u), (uint8_t)cpu->ax);

    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    cpu->ax = (uint8_t)cpu->ax >= 0x80u ?
              (uint16_t)(0xFF00u | (uint8_t)cpu->ax) : (uint8_t)cpu->ax;
    cpu->di = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ds,
                               (uint16_t)(cpu->di + 0x7045u)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 10u), (uint8_t)cpu->ax);

    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    cpu->ax = (uint8_t)cpu->ax >= 0x80u ?
              (uint16_t)(0xFF00u | (uint8_t)cpu->ax) : (uint8_t)cpu->ax;
    cpu->di = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ds,
                               (uint16_t)(cpu->di + 0x7005u)));
    type = (uint8_t)cpu->ax;

    cmp_al(cpu, 0u);
    if (type == 0u) {
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                             read8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u)));
        xor_ah_ah(cpu);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);
        goto epilogue;
    }
    cmp_al(cpu, 1u);
    if (type == 1u) goto group_word;
    cmp_al(cpu, 3u);
    if (type == 3u) goto group_word;
    cmp_al(cpu, 0x80u);
    if (type == 0x80u) goto group_word;
    cmp_al(cpu, 2u);
    if (type == 2u) goto group_pair;
    cmp_al(cpu, 0x81u);
    if (type == 0x81u) goto group_pair;
    goto epilogue; /* Original reads an untouched BP-2 word. */

group_word:
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u)));
    xor_ah_ah(cpu);
    cpu->dx = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 10u)));
    xor_ah_ah(cpu);
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 8u);
    shl_ax_eight(cpu);
    add_ax_dx(cpu);
    push16(ram, cpu, cpu->ax);
    status = call_local(ram, ram_size, cpu, io, PR_U25_0136_CALL_0E0C,
                        0x0E0Cu, 0x0191u, stopped_at);
    if (status != PR_U25_0136_RETURNED) return status;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);
    goto epilogue;

group_pair:
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u)));
    push16(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 10u)));
    push16(ram, cpu, cpu->ax);
    status = call_local(ram, ram_size, cpu, io, PR_U25_0136_CALL_06AF,
                        0x06AFu, 0x01AAu, stopped_at);
    if (status != PR_U25_0136_RETURNED) return status;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);

epilogue:
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->cs = read16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_U25_0136_RETURNED;
}
