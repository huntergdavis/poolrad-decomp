/* GAME.OVR unit 0x3B4 +00C7, +00FB and +012F; docs/re/U3B4_CASE_MAPS.md. */
#include "poolrad/recovered/overlay_u3b4_case_maps.h"

#define MASK (PR_U3B4_CASE_RAM_SIZE - 1u)

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

static uint8_t al(const pr_u3b4_case_cpu *cpu)
{
    return (uint8_t)cpu->ax;
}

static void set_al(pr_u3b4_case_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* CMP AL,imm8: CF PF AF ZF SF OF from AL - imm8; the other FLAGS bits are kept. Returns ZF, so a JNE falls
 * through exactly when it is nonzero. */
static int cmp_al(pr_u3b4_case_cpu *cpu, uint8_t right)
{
    uint8_t left = al(cpu), result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
    return result == 0u;
}

static int valid(const uint8_t *ram, size_t ram_size, const pr_u3b4_case_cpu *cpu)
{
    return ram != NULL && cpu != NULL && ram_size == PR_U3B4_CASE_RAM_SIZE;
}

/* PUSH BP; MOV BP,SP; SUB SP,2 (its flags are overwritten by the first CMP); MOV AL,[BP+8]. */
static void enter(uint8_t *ram, pr_u3b4_case_cpu *cpu)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & MASK] = (uint8_t)(cpu->bp >> 8);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
}

/* MOV BYTE [BP-1],imm8; each arm then jumps to the shared exit. */
static void keep(uint8_t *ram, const pr_u3b4_case_cpu *cpu, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), value);
}

/* MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 4. */
static pr_u3b4_case_status leave(uint8_t *ram, pr_u3b4_case_cpu *cpu)
{
    size_t at;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U3B4_CASE_OK;
}

pr_u3b4_case_status pr_u3b4_case_map_00c7(uint8_t *ram, size_t ram_size, pr_u3b4_case_cpu *cpu)
{
    if (!valid(ram, ram_size, cpu))
        return PR_U3B4_CASE_ARGUMENT;
    enter(ram, cpu);                                    /* +00C7..+00CD */
    if (cmp_al(cpu, 0x08u))                             /* +00D0 CMP AL,08h; JNE +00DA */
        keep(ram, cpu, 0x00u);                          /* +00D4 */
    else if (cmp_al(cpu, 0x41u))                        /* +00DA CMP AL,41h; JNE +00E4 */
        keep(ram, cpu, 0x00u);                          /* +00DE */
    else if (cmp_al(cpu, 0xE8u))                        /* +00E4 CMP AL,E8h; JNE +00EE */
        keep(ram, cpu, 0x0Fu);                          /* +00E8 */
    else
        keep(ram, cpu, 0x0Fu);                          /* +00EE */
    return leave(ram, cpu);                             /* +00F2..+00F8 */
}

pr_u3b4_case_status pr_u3b4_case_map_00fb(uint8_t *ram, size_t ram_size, pr_u3b4_case_cpu *cpu)
{
    if (!valid(ram, ram_size, cpu))
        return PR_U3B4_CASE_ARGUMENT;
    enter(ram, cpu);                                    /* +00FB..+0101 */
    if (cmp_al(cpu, 0x08u))                             /* +0104 CMP AL,08h; JNE +010E */
        keep(ram, cpu, 0x08u);                          /* +0108 */
    else if (cmp_al(cpu, 0x41u))                        /* +010E CMP AL,41h; JNE +0118 */
        keep(ram, cpu, 0x07u);                          /* +0112 */
    else if (cmp_al(cpu, 0xE8u))                        /* +0118 CMP AL,E8h; JNE +0122 */
        keep(ram, cpu, 0x07u);                          /* +011C */
    else
        keep(ram, cpu, 0x07u);                          /* +0122 */
    return leave(ram, cpu);                             /* +0126..+012C */
}

pr_u3b4_case_status pr_u3b4_case_map_012f(uint8_t *ram, size_t ram_size, pr_u3b4_case_cpu *cpu)
{
    if (!valid(ram, ram_size, cpu))
        return PR_U3B4_CASE_ARGUMENT;
    enter(ram, cpu);                                    /* +012F..+0135 */
    if (cmp_al(cpu, 0x0Bu))                             /* +0138 CMP AL,0Bh; JNE +0142 */
        keep(ram, cpu, 0x08u);                          /* +013C */
    else if (cmp_al(cpu, 0x09u))                        /* +0142 CMP AL,09h; JNE +014C */
        keep(ram, cpu, 0x06u);                          /* +0146 */
    else if (cmp_al(cpu, 0xDBu))                        /* +014C CMP AL,DBh; JNE +0156 */
        keep(ram, cpu, 0x06u);                          /* +0150 */
    else if (cmp_al(cpu, 0x06u))                        /* +0156 CMP AL,06h; JNE +0160 */
        keep(ram, cpu, 0x01u);                          /* +015A */
    else
        keep(ram, cpu, 0x06u);                          /* +0160 */
    return leave(ram, cpu);                             /* +0164..+016A */
}
