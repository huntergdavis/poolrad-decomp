/* GAME.OVR unit 0x25 +1951 (raw CPU, composing SYSTEM GetMem 023F and U25 +06AF); docs/re/U25_ALLOC_1951_CPU.md. */
#include "poolrad/recovered/overlay_u25_alloc_1951_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"

#include <string.h>

#define MASK (PR_U25_1951_RAM_SIZE - 1u)

#define COPY_CPU(to, from)                                                                                   \
    do {                                                                                                     \
        (to)->ax = (from)->ax; (to)->bx = (from)->bx; (to)->cx = (from)->cx; (to)->dx = (from)->dx;          \
        (to)->si = (from)->si; (to)->di = (from)->di; (to)->bp = (from)->bp; (to)->sp = (from)->sp;          \
        (to)->cs = (from)->cs; (to)->ip = (from)->ip; (to)->ds = (from)->ds; (to)->es = (from)->es;          \
        (to)->ss = (from)->ss; (to)->flags = (from)->flags;                                                  \
    } while (0)

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u25_1951_cpu *cpu;
    unsigned max_heap_steps;
    pr_u25_1951_callee *callee;
    int *callee_status;
    pr_u25_1951_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t read8(const machine *m, uint16_t segment, uint16_t offset)
{
    return m->ram[physical(segment, offset)];
}

static uint16_t read16(const machine *m, uint16_t segment, uint16_t offset)
{
    return read16_at(m->ram, physical(segment, offset));
}

static void write16(machine *m, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(machine *m, uint16_t value)
{
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    write16(m, m->cpu->ss, m->cpu->sp, value);
}

/* LES DI,segment:[offset], the pair read physically contiguous. */
static void les(machine *m, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    m->cpu->di = read16_at(m->ram, at);
    m->cpu->es = read16_at(m->ram, (at + 2u) & MASK);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* OR or XOR: CF = OF = AF = 0, ZF/SF/PF from the result whose sign bit is `sign`; the other flags are kept. */
static void logic_flags(pr_u25_1951_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void note(machine *m, pr_u25_1951_callee which, int status)
{
    if (m->callee != NULL)
        *m->callee = which;
    if (m->callee_status != NULL)
        *m->callee_status = status;
}

/* MOV AX,6; PUSH AX; LCALL xxxx:023F at `site`; returns 1 when GetMem came back to the next instruction. */
static int getmem(machine *m, uint16_t site)
{
    pr_u25_1951_cpu *cpu = m->cpu;
    uint16_t caller_cs = cpu->cs, return_ip = (uint16_t)(site + 5u), segment;
    pr_system_getmem_cpu state;
    pr_system_getmem_status status;
    cpu->ax = 0x0006u;
    push16(m, cpu->ax);
    cpu->ip = site;
    if (read8(m, cpu->cs, site) != 0x9Au || read16(m, cpu->cs, (uint16_t)(site + 1u)) != 0x023Fu) {
        m->stop = PR_U25_1951_CODE;
        return 0;
    }
    segment = read16(m, cpu->cs, (uint16_t)(site + 3u));
    push16(m, caller_cs);
    push16(m, return_ip);
    cpu->cs = segment;
    cpu->ip = 0x023Fu;
    COPY_CPU(&state, cpu);
    status = pr_system_getmem_023f(m->ram, m->size, &state, m->max_heap_steps);
    COPY_CPU(cpu, &state);
    note(m, PR_U25_1951_GETMEM, (int)status);
    if (status != PR_SYSTEM_GETMEM_RETURNED) {
        m->stop = PR_U25_1951_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U25_1951_TRANSFER;
        return 0;
    }
    return 1;
}

pr_u25_1951_status pr_u25_alloc_1951_cpu(uint8_t *ram, size_t ram_size, pr_u25_1951_cpu *cpu, unsigned max_heap_steps,
                                         pr_u25_1951_callee *callee, int *callee_status)
{
    machine m;
    pr_u25_0000_cpu word;
    pr_u25_0000_status status;
    uint16_t caller_cs;
    size_t at;
    if (callee != NULL)
        *callee = PR_U25_1951_NO_CALLEE;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U25_1951_RAM_SIZE || cpu->ip != 0x1951u)
        return PR_U25_1951_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.max_heap_steps = max_heap_steps; m.callee = callee;
    m.callee_status = callee_status; m.stop = PR_U25_1951_OK;
    push16(&m, cpu->bp);                                                  /* 1951 PUSH BP; MOV BP,SP */
    cpu->bp = cpu->sp;
    cpu->ax = read16(&m, cpu->ds, 0x6F70u);                               /* 1954 MOV AX,[6F70h]; OR AX,[6F72h]; JNE 197D */
    cpu->ax |= read16(&m, cpu->ds, 0x6F72u);
    logic_flags(cpu, cpu->ax, 0x8000u);
    if (cpu->ax == 0u) {
        if (!getmem(&m, 0x1961u))                                         /* 195D MOV AX,6; PUSH AX; LCALL 0AF8:023F */
            return m.stop;
        write16(&m, cpu->ds, 0x6F70u, cpu->ax);                           /* 1966 MOV [6F70h],AX; MOV [6F72h],DX */
        write16(&m, cpu->ds, 0x6F72u, cpu->dx);
        les(&m, cpu->ds, 0x6F70u);                                        /* 196D LES DI,[6F70h]; XOR AX,AX; the link := nil */
        cpu->ax = 0u;
        logic_flags(cpu, 0u, 0x8000u);
        write16(&m, cpu->es, (uint16_t)(cpu->di + 2u), cpu->ax);
        write16(&m, cpu->es, (uint16_t)(cpu->di + 4u), cpu->ax);
    } else {
        cpu->ax = read16(&m, cpu->ds, 0x6F70u);                           /* 197D the old head to [6F74h]/[6F76h] */
        cpu->dx = read16(&m, cpu->ds, 0x6F72u);
        write16(&m, cpu->ds, 0x6F74u, cpu->ax);
        write16(&m, cpu->ds, 0x6F76u, cpu->dx);
        if (!getmem(&m, 0x198Fu))                                         /* 198B MOV AX,6; PUSH AX; LCALL 0AF8:023F */
            return m.stop;
        write16(&m, cpu->ds, 0x6F70u, cpu->ax);                           /* 1994 MOV [6F70h],AX; MOV [6F72h],DX */
        write16(&m, cpu->ds, 0x6F72u, cpu->dx);
        cpu->ax = read16(&m, cpu->ds, 0x6F74u);                           /* 199B the link := the old head */
        cpu->dx = read16(&m, cpu->ds, 0x6F76u);
        les(&m, cpu->ds, 0x6F70u);
        write16(&m, cpu->es, (uint16_t)(cpu->di + 2u), cpu->ax);
        write16(&m, cpu->es, (uint16_t)(cpu->di + 4u), cpu->dx);
    }
    cpu->ax = read16(&m, cpu->ds, 0x49EDu);                               /* 19AE MOV AX,[49EDh]; LES DI,[6F70h]; MOV ES:[DI],AX */
    les(&m, cpu->ds, 0x6F70u);
    write16(&m, cpu->es, cpu->di, cpu->ax);
    cpu->ax = read8(&m, cpu->ss, (uint16_t)(cpu->bp + 6u));               /* 19B8 MOV AL,[BP+6]; XOR AH,AH; MOV DI,AX; */
    cpu->di = cpu->ax;                                                    /*      MOV AL,[DI+7085h]; PUSH AX (flags folded) */
    cpu->ax = read8(&m, cpu->ds, (uint16_t)(cpu->di + 0x7085u));
    push16(&m, cpu->ax);
    cpu->ax = read8(&m, cpu->ss, (uint16_t)(cpu->bp + 6u));               /* 19C4 MOV AL,[BP+6]; XOR AH,AH; MOV DI,AX; */
    logic_flags(cpu, 0u, 0x0080u);                                        /*      MOV AL,[DI+7045h]; PUSH AX */
    cpu->di = cpu->ax;
    cpu->ax = read8(&m, cpu->ds, (uint16_t)(cpu->di + 0x7045u));
    push16(&m, cpu->ax);
    caller_cs = cpu->cs;                                                  /* 19D0 PUSH CS; CALL 06AF */
    push16(&m, cpu->cs);
    push16(&m, 0x19D4u);
    cpu->ip = 0x06AFu;
    COPY_CPU(&word, cpu);
    status = pr_u25_word_06af(ram, ram_size, &word);
    COPY_CPU(cpu, &word);
    note(&m, PR_U25_1951_WORD_06AF, (int)status);
    if (status != PR_U25_0000_OK)
        return PR_U25_1951_CALLEE;
    if (cpu->ip != 0x19D4u || cpu->cs != caller_cs)
        return PR_U25_1951_TRANSFER;
    write16(&m, cpu->ds, 0x49EDu, cpu->ax);                               /* 19D4 MOV [49EDh],AX */
    at = physical(cpu->ss, cpu->sp);                                      /* 19D7 POP BP; RETF 2 */
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_U25_1951_OK;
}
