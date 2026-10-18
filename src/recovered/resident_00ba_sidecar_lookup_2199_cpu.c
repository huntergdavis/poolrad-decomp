/* START.EXE resident RES_00BA_2199, raw-CPU full state; docs/re/RESIDENT_SIDECAR_LOOKUP_2199_CPU.md. */
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"

#define MASK (PR_RES_2199_RAM_SIZE - 1u)

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

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

/* LES DI,[segment:offset]: DI from the first word, ES from the next two physical bytes. */
static void les(const uint8_t *ram, pr_res_2199_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    uint16_t di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->di = di;
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* OR r16,r/m16: CF = OF = AF = 0; PF of the low byte, ZF, SF. */
static void or_flags(pr_res_2199_cpu *cpu)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)cpu->ax)) cpu->flags |= 0x0004u;
    if (cpu->ax == 0u) cpu->flags |= 0x0040u;
    if (cpu->ax & 0x8000u) cpu->flags |= 0x0080u;
}

/* CMP r/m8,r8 or imm8. */
static void cmp8(pr_res_2199_cpu *cpu, uint8_t left, uint8_t right)
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

static void set_al(pr_res_2199_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

pr_res_2199_status pr_res_00ba_2199_cpu(uint8_t *ram, size_t ram_size, pr_res_2199_cpu *cpu, unsigned max_loopbacks)
{
    uint16_t entry;
    uint8_t value;
    unsigned loopbacks = 0u;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_RES_2199_RAM_SIZE)
        return PR_RES_2199_ARGUMENT;
    entry = cpu->ip;
    /* 2199..219C: PUSH BP; MOV BP,SP; SUB SP,2 (its flags are overwritten by the OR before any exit). */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0u);                       /* 219F */
    /* 21A3..21B5: C := X->+7F/+81. */
    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x7Fu));
    cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x81u));
    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
    write16(ram, cpu->es, cpu->di, cpu->ax);
    write16(ram, cpu->es, (uint16_t)(cpu->di + 2u), cpu->dx);
    for (;;) {
        /* 21B9..21C3: LES DI,[BP+6]; MOV AX,ES:[DI]; OR AX,ES:[DI+2]; JE exit. */
        les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
        cpu->ax = read16(ram, cpu->es, cpu->di);
        cpu->ax = (uint16_t)(cpu->ax | read16(ram, cpu->es, (uint16_t)(cpu->di + 2u)));
        or_flags(cpu);
        if (cpu->ax == 0u)
            break;
        /* 21C5..21C9: CMP BYTE [BP-2],0; JNE exit. */
        value = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        cmp8(cpu, value, 0u);
        if (value != 0u)
            break;
        /* 21CB..21D7: LES DI,[BP+6]; LES DI,ES:[DI]; MOV AL,ES:[DI]; CMP AL,[BP+0Ah]; JNE miss. */
        les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
        les(ram, cpu, cpu->es, cpu->di);
        set_al(cpu, read8(ram, cpu->es, cpu->di));
        value = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Au));
        cmp8(cpu, (uint8_t)cpu->ax, value);
        if ((uint8_t)cpu->ax == value) {
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);              /* 21D9; JMP 21F7 */
        } else {
            /* 21DF..21F3: C := node->+5/+7. */
            les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
            les(ram, cpu, cpu->es, cpu->di);
            cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 5u));
            cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 7u));
            les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
            write16(ram, cpu->es, cpu->di, cpu->ax);
            write16(ram, cpu->es, (uint16_t)(cpu->di + 2u), cpu->dx);
        }
        /* 21F7: JMP 21B9. */
        if (loopbacks == max_loopbacks) {
            cpu->ip = (uint16_t)(entry + 0x20u);
            return PR_RES_2199_LIMIT;
        }
        ++loopbacks;
    }
    /* 21F9..21FF: MOV AL,[BP-2]; MOV [BP-1],AL; MOV AL,[BP-1]. */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    /* 2202..2205: MOV SP,BP; POP BP; RETF 0Ah. */
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Au);
    return PR_RES_2199_OK;
}
