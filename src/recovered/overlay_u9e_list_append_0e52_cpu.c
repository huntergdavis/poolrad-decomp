/* GAME.OVR unit 0x9E +0E52, entry 10 (raw CPU, composing SYSTEM GetMem 023F); docs/re/U9E_LIST_APPEND_0E52_CPU.md. */
#include "poolrad/recovered/overlay_u9e_list_append_0e52_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"

#define MASK (PR_U9E_0E52_RAM_SIZE - 1u)

#define COPY_CPU(to, from)                                                                                   \
    do {                                                                                                     \
        (to)->ax = (from)->ax; (to)->bx = (from)->bx; (to)->cx = (from)->cx; (to)->dx = (from)->dx;          \
        (to)->si = (from)->si; (to)->di = (from)->di; (to)->bp = (from)->bp; (to)->sp = (from)->sp;          \
        (to)->cs = (from)->cs; (to)->ip = (from)->ip; (to)->ds = (from)->ds; (to)->es = (from)->es;          \
        (to)->ss = (from)->ss; (to)->flags = (from)->flags;                                                  \
    } while (0)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u9e_0e52_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

/* LES DI,SS:[BP+displacement], the pair read physically contiguous. */
static void les_frame(const uint8_t *ram, pr_u9e_0e52_cpu *cpu, uint16_t displacement)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp + displacement));
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* OR or XOR on words: CF = OF = AF = 0, ZF/SF/PF from the result; the other flags are kept. */
static void logic_flags(pr_u9e_0e52_cpu *cpu, uint16_t result)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
}

/* SUB SP,8: the six arithmetic flags of the subtraction; the other flags are kept. */
static void sub_flags(pr_u9e_0e52_cpu *cpu, uint16_t value, uint16_t operand)
{
    uint16_t result = (uint16_t)(value - operand);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (value < operand) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((value ^ operand ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((value ^ operand) & (value ^ result) & 0x8000u) cpu->flags |= 0x0800u;
}

pr_u9e_0e52_status pr_u9e_list_append_0e52_cpu(uint8_t *ram, size_t ram_size, pr_u9e_0e52_cpu *cpu, unsigned max_heap_steps,
                                               size_t max_links, int *getmem_status)
{
    pr_system_getmem_cpu state;
    pr_system_getmem_status status;
    uint16_t caller_cs, segment;
    size_t at, links;
    if (getmem_status != NULL)
        *getmem_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U9E_0E52_RAM_SIZE || cpu->ip != 0x0E52u)
        return PR_U9E_0E52_ARGUMENT;
    push16(ram, cpu, cpu->bp);                                            /* 0E52 PUSH BP; MOV BP,SP; SUB SP,8 */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 8u);
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    cpu->ax = 0x0009u;                                                    /* 0E58 MOV AX,9; PUSH AX */
    push16(ram, cpu, cpu->ax);
    cpu->ip = 0x0E5Cu;                                                    /* 0E5C LCALL xxxx:023F */
    if (read8(ram, cpu->cs, 0x0E5Cu) != 0x9Au || read16(ram, cpu->cs, 0x0E5Du) != 0x023Fu)
        return PR_U9E_0E52_CODE;
    segment = read16(ram, cpu->cs, 0x0E5Fu);
    caller_cs = cpu->cs;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x0E61u);
    cpu->cs = segment;
    cpu->ip = 0x023Fu;
    COPY_CPU(&state, cpu);
    status = pr_system_getmem_023f(ram, ram_size, &state, max_heap_steps);
    COPY_CPU(cpu, &state);
    if (getmem_status != NULL)
        *getmem_status = (int)status;
    if (status != PR_SYSTEM_GETMEM_RETURNED)
        return PR_U9E_0E52_CALLEE;
    if (cpu->ip != 0x0E61u || cpu->cs != caller_cs)
        return PR_U9E_0E52_TRANSFER;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);             /* 0E61 MOV [BP-4],AX; MOV [BP-2],DX */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    les_frame(ram, cpu, 0x000Eu);                                         /* 0E67 LES DI,[BP+0Eh]; MOV AX,ES:[DI+7Fh]; */
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x7Fu));          /*      OR AX,ES:[DI+81h]; JNE 0E89 */
    cpu->ax |= read16(ram, cpu->es, (uint16_t)(cpu->di + 0x81u));
    logic_flags(cpu, cpu->ax);
    if (cpu->ax == 0u) {
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));         /* 0E75 the head := the node; JMP 0ECC */
        cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        les_frame(ram, cpu, 0x000Eu);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x7Fu), cpu->ax);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x81u), cpu->dx);
    } else {
        les_frame(ram, cpu, 0x000Eu);                                     /* 0E89 the cursor [BP-8] := the head */
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x7Fu));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x81u));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->dx);
        for (links = 0u;; ++links) {
            les_frame(ram, cpu, (uint16_t)0xFFF8u);                       /* 0E9B LES DI,[BP-8]; MOV AX,ES:[DI+5]; */
            cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 5u));     /*      OR AX,ES:[DI+7]; JE 0EBB */
            cpu->ax |= read16(ram, cpu->es, (uint16_t)(cpu->di + 7u));
            logic_flags(cpu, cpu->ax);
            if (cpu->ax == 0u)
                break;
            if (links == max_links) {
                cpu->ip = 0x0EA8u;
                return PR_U9E_0E52_LIMIT;
            }
            les_frame(ram, cpu, (uint16_t)0xFFF8u);                       /* 0EA8 the cursor := its link; JMP 0E9B */
            cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 5u));
            cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 7u));
            write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
            write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->dx);
        }
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));         /* 0EBB the last link := the node */
        cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        les_frame(ram, cpu, (uint16_t)0xFFF8u);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 5u), cpu->ax);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 7u), cpu->dx);
    }
    les_frame(ram, cpu, (uint16_t)0xFFFCu);                               /* 0ECC LES DI,[BP-4]; XOR AX,AX; the link := nil */
    cpu->ax = 0u;
    logic_flags(cpu, 0u);
    write16(ram, cpu->es, (uint16_t)(cpu->di + 5u), cpu->ax);
    write16(ram, cpu->es, (uint16_t)(cpu->di + 7u), cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu)));  /* 0ED9 byte +0 := b */
    les_frame(ram, cpu, (uint16_t)0xFFFCu);
    write8(ram, cpu->es, cpu->di, (uint8_t)cpu->ax);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Au));          /* 0EE2 word +1 := a */
    les_frame(ram, cpu, (uint16_t)0xFFFCu);
    write16(ram, cpu->es, (uint16_t)(cpu->di + 1u), cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));     /* 0EEC byte +4 := d */
    les_frame(ram, cpu, (uint16_t)0xFFFCu);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 4u), (uint8_t)cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));     /* 0EF6 byte +3 := c */
    les_frame(ram, cpu, (uint16_t)0xFFFCu);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 3u), (uint8_t)cpu->ax);
    cpu->sp = cpu->bp;                                                    /* 0F00 MOV SP,BP; POP BP; RETF 0Ch */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 12u);
    return PR_U9E_0E52_OK;
}
