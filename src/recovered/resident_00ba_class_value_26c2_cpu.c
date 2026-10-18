/* START.EXE resident RES_00BA_26C2, raw-CPU full state; docs/re/RES_26C2_U86_0729.md. */
#include "poolrad/recovered/resident_00ba_class_value_26c2_cpu.h"

#define MASK (PR_RES_26C2_RAM_SIZE - 1u)

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

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_res_26c2_cpu *cpu, uint8_t left, uint8_t right)
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

/* MOV AL,[BP+6]; XOR AH,AH; MOV DI,AX; MOV CL,4; SHL DI,CL (flags folded). */
static void index16(const uint8_t *ram, pr_res_26c2_cpu *cpu)
{
    cpu->ax = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = (uint16_t)(cpu->ax << 4);
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
}

/* LES DI,[5D92]; MOV AL,ES:[DI+field]; MOV [BP-1],AL. */
static void record_byte(uint8_t *ram, pr_res_26c2_cpu *cpu, uint16_t field)
{
    size_t at = physical(cpu->ds, 0x5D92u);
    uint16_t di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->di = di;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->es, (uint16_t)(cpu->di + field)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
}

pr_res_26c2_status pr_res_00ba_26c2_cpu(uint8_t *ram, size_t ram_size, pr_res_26c2_cpu *cpu)
{
    uint8_t value;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_RES_26C2_RAM_SIZE)
        return PR_RES_26C2_ARGUMENT;
    /* 26C2..26C5: PUSH BP; MOV BP,SP; SUB SP,2. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & MASK] = (uint8_t)(cpu->bp >> 8);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    /* 26C8..26D7: AL := DS:[31FA + 16k]; CMP AL,0/1/2. */
    index16(ram, cpu);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x31FAu)));
    value = (uint8_t)cpu->ax;
    cmp8(cpu, value, 0u);
    if (value == 0u) {
        record_byte(ram, cpu, 0x96u);                                     /* 26DB */
    } else {
        cmp8(cpu, value, 1u);
        if (value == 1u) {
            record_byte(ram, cpu, 0x9Bu);                                 /* 26ED */
        } else {
            cmp8(cpu, value, 2u);
            if (value == 2u)
                write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0x0Cu);    /* 26FF */
        }
    }
    /* 2703..271C: if DS:6DBF and DS:[31FA + 16k] is not 2, [BP-1] := 6. */
    value = read8(ram, cpu->ds, 0x6DBFu);
    cmp8(cpu, value, 0u);
    if (value != 0u) {
        index16(ram, cpu);
        value = read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x31FAu));
        cmp8(cpu, value, 2u);
        if (value != 2u)
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 6u);
    }
    /* 2720..2726: MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 2. */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_RES_26C2_OK;
}
