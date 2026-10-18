/* GAME.OVR unit 0x3C6:+0068..+00E2; docs/re/U3C6_SLOT_OFFSETS_0068.md. */
#include "poolrad/recovered/overlay_u3c6_slot_offsets_0068.h"

#define MASK (PR_U3C6_0068_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void push16(uint8_t *ram, pr_u3c6_0068_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void set_al(pr_u3c6_0068_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u3c6_0068_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u3c6_0068_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

/* LES DI,[6716]; MOV AL,ES:[DI+field]; CBW; MOV DX,AX; MOV AL,[BP-1]; XOR AH,AH; MOV DI,AX; SHL DI,1;
 * SHL DI,1; MOV AL,[DI+table]; CBW; SUB AX,DX; MOV DL,AL; MOV AL,[BP-1]; XOR AH,AH; MOV DI,AX;
 * MOV [DI+target],DL. The XOR/SHL/SUB flags are overwritten by the pass-end CMP before they are observed. */
static void store(uint8_t *ram, pr_u3c6_0068_cpu *cpu, uint16_t field, uint16_t table, uint16_t target)
{
    size_t at = physical(cpu->ds, 0x6716u);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + field)));
    cbw(cpu);
    cpu->dx = cpu->ax;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->ax &= 0x00FFu;
    cpu->di = (uint16_t)(cpu->ax << 2);
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + table)));
    cbw(cpu);
    cpu->ax = (uint16_t)(cpu->ax - cpu->dx);
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)cpu->ax);
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->ax &= 0x00FFu;
    cpu->di = cpu->ax;
    write8(ram, cpu->ds, (uint16_t)(cpu->di + target), (uint8_t)cpu->dx);
}

pr_u3c6_0068_status pr_u3c6_slot_offsets_0068(uint8_t *ram, size_t ram_size, pr_u3c6_0068_cpu *cpu,
                                              unsigned max_loopbacks)
{
    uint16_t entry;
    uint8_t count, limit;
    unsigned loopbacks = 0u;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_U3C6_0068_RAM_SIZE)
        return PR_U3C6_0068_ARGUMENT;
    entry = cpu->ip;
    /* +0068..+0071: frame, two local bytes; [BP-2] := DS:5F2A (SUB's flags are overwritten by the CMP). */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    set_al(cpu, read8(ram, cpu->ds, 0x5F2Au));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
    /* +0074..+0079: MOV AL,1; CMP AL,[BP-2]; JA +00DF (no slot when the count is 0). */
    set_al(cpu, 1u);
    count = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cmp8(cpu, 1u, count);
    if (1u <= count) {
        /* +007B..+007E: [BP-1] := 1; JMP +0083. */
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
        for (;;) {
            store(ram, cpu, 2u, 0x5F27u, 0x604Au);                  /* +0083..+00A9 */
            store(ram, cpu, 3u, 0x5F28u, 0x6092u);                  /* +00AD..+00D3 */
            /* +00D7..+00DD: MOV AL,[BP-1]; CMP AL,[BP-2]; JNE +0080. */
            set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
            limit = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
            cmp8(cpu, (uint8_t)cpu->ax, limit);
            if ((uint8_t)cpu->ax == limit)
                break;
            if (loopbacks == max_loopbacks) {
                cpu->ip = (uint16_t)(entry + 0x18u);                /* host cap before +0080 */
                return PR_U3C6_0068_LIMIT;
            }
            ++loopbacks;
            /* +0080: INC BYTE [BP-1] (its flags are overwritten by the next pass-end CMP). */
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u),
                   (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)) + 1u));
        }
    }
    /* +00DF..+00E2: MOV SP,BP; POP BP; RETF. */
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U3C6_0068_OK;
}
