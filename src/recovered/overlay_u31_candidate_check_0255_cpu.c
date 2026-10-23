#include "overlay_loaded_contracts.h"
/* GAME.OVR unit 0x31 +0255 (raw CPU, composing U3C2 entry 6 through the loaded stub 03DB:003E and the resident zero
 * test 00BA:23B7); docs/re/U31_CANDIDATE_CHECK_0255_CPU.md. */
#include "poolrad/recovered/overlay_u31_candidate_check_0255_cpu.h"
#include "poolrad/recovered/overlay_u3c2_caller_0909.h"

#include <string.h>

#define MASK (PR_U31_0255_RAM_SIZE - 1u)
#define ENTRY_IP 0x0255u
#define STUB_OFFSET 0x003Eu
#define U3C2_ENTRY 0x0909u
#define RESIDENT_ENTRY 0x23B7u

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
    pr_u31_0255_cpu *cpu;
    uint16_t entry, pair_cs;
    size_t max_caller_iterations, max_line_iterations;
    unsigned max_sort_loopbacks;
    pr_u31_0255_callee *callee;
    int *callee_status;
    pr_u31_0255_status stop;
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

static void write8(machine *m, uint16_t segment, uint16_t offset, uint8_t value)
{
    m->ram[physical(segment, offset)] = value;
}

static uint16_t read16(const machine *m, uint16_t segment, uint16_t offset)
{
    return read16_at(m->ram, physical(segment, offset));
}

static void push16(machine *m, uint16_t value)
{
    size_t at;
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    at = physical(m->cpu->ss, m->cpu->sp);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t here(const machine *m, uint16_t offset)
{
    return (uint16_t)(m->entry + (uint16_t)(offset - ENTRY_IP));
}

/* LES DI,segment:[offset], the pair read physically contiguous. */
static void les(machine *m, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    m->cpu->di = read16_at(m->ram, at);
    m->cpu->es = read16_at(m->ram, (at + 2u) & MASK);
}

/* MOV SP,BP; POP BP; RETF n, the POP and the frame read physically contiguous. */
static void leave_retf(machine *m, uint16_t n)
{
    pr_u31_0255_cpu *cpu = m->cpu;
    size_t at;
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(m->ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(m->ram, at);
    cpu->cs = read16_at(m->ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + n);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* CMP left,right on bytes: the six arithmetic flags; the other flags are kept. */
static void cmp8_flags(pr_u31_0255_cpu *cpu, uint8_t left, uint8_t right)
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

/* INC BYTE giving `result`: OF SF ZF AF PF; CF is kept. */
static void inc8_flags(pr_u31_0255_cpu *cpu, uint8_t result)
{
    cpu->flags &= (uint16_t)~0x08D4u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((result & 0x0Fu) == 0u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if (result == 0x80u) cpu->flags |= 0x0800u;
}

static void set_al(pr_u31_0255_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void note(machine *m, pr_u31_0255_callee which, int status)
{
    if (m->callee != NULL)
        *m->callee = which;
    if (m->callee_status != NULL)
        *m->callee_status = status;
}

/* +0287 LCALL 03DB:003E (seven words pushed); the loaded stub's EA jumps to U3C2 +0909. */
static int u3c2_call(machine *m)
{
    pr_u31_0255_cpu *cpu = m->cpu;
    uint16_t at = here(m, 0x0287u), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs, stub;
    pr_u3c2_0909_cpu state;
    pr_u3c2_0909_status status;
    cpu->ip = at;
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != STUB_OFFSET) {
        m->stop = PR_U31_0255_CODE;
        return 0;
    }
    stub = read16(m, cpu->cs, (uint16_t)(at + 3u));
    push16(m, caller_cs);
    push16(m, return_ip);
    cpu->cs = stub;
    cpu->ip = STUB_OFFSET;
    if (read8(m, stub, STUB_OFFSET) != 0xEAu || read16(m, stub, STUB_OFFSET + 1u) != U3C2_ENTRY) {
        m->stop = PR_U31_0255_CODE;
        return 0;
    }
    cpu->cs = read16(m, stub, STUB_OFFSET + 3u);
    cpu->ip = U3C2_ENTRY;
    COPY_CPU(&state, cpu);
    status = pr_u3c2_caller_0909(m->ram, m->size, &state, m->pair_cs, m->max_caller_iterations, m->max_line_iterations,
                                 m->max_sort_loopbacks);
    COPY_CPU(cpu, &state);
    note(m, PR_U31_0255_U3C2, (int)status);
    if (status != PR_U3C2_0909_OK) {
        m->stop = PR_U31_0255_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U31_0255_TRANSFER;
        return 0;
    }
    return 1;
}

/* +02A9 LCALL 00BA:23B7 (the active character pushed): the resident zero test, translated here in full state. */
static int resident_call(machine *m)
{
    pr_u31_0255_cpu *cpu = m->cpu;
    uint16_t at = here(m, 0x02A9u), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs, segment;
    uint8_t value;
    cpu->ip = at;
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != RESIDENT_ENTRY) {
        m->stop = PR_U31_0255_CODE;
        return 0;
    }
    segment = read16(m, cpu->cs, (uint16_t)(at + 3u));
    push16(m, caller_cs);
    push16(m, return_ip);
    cpu->cs = segment;
    cpu->ip = RESIDENT_ENTRY;
    push16(m, cpu->bp);                                                   /* 23B7 PUSH BP; MOV BP,SP; SUB SP,2 (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    les(m, cpu->ss, (uint16_t)(cpu->bp + 6u));                            /* 23BD LES DI,[BP+6]; CMP BYTE ES:[DI+10Eh],0; JNE 23CE */
    value = read8(m, cpu->es, (uint16_t)(cpu->di + 0x010Eu));
    cmp8_flags(cpu, value, 0u);
    write8(m, cpu->ss, (uint16_t)(cpu->bp - 1u), value == 0u ? 1u : 0u);  /* 23C8 MOV BYTE [BP-1],1 or 23CE MOV BYTE [BP-1],0 */
    set_al(cpu, read8(m, cpu->ss, (uint16_t)(cpu->bp - 1u)));             /* 23D2 MOV AL,[BP-1] */
    leave_retf(m, 4u);                                                    /* 23D5 MOV SP,BP; POP BP; RETF 4 */
    note(m, PR_U31_0255_RESIDENT, 0);
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U31_0255_TRANSFER;
        return 0;
    }
    return 1;
}

pr_u31_0255_status pr_u31_candidate_check_0255_cpu(uint8_t *ram, size_t ram_size, pr_u31_0255_cpu *cpu, uint16_t pair_cs,
                                                   size_t max_caller_iterations, size_t max_line_iterations,
                                                   unsigned max_sort_loopbacks, pr_u31_0255_callee *callee, int *callee_status)
{
    machine m;
    uint8_t count, value;
    if (callee != NULL)
        *callee = PR_U31_0255_NO_CALLEE;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U31_0255_RAM_SIZE || max_caller_iterations == 0u || max_line_iterations == 0u
        || max_sort_loopbacks == 0u || (uint16_t)(cpu->sp - 0x18u) < 0x008Eu || (uint16_t)(cpu->sp - 0x18u) > 0xFFEEu)
        return PR_U31_0255_ARGUMENT;
    {
        uint16_t chain_cs;
        /* Preserve this caller's existing CODE stop for its own malformed binding.
         * A valid own 0xEA jump must lead to a chain whose dependencies are loaded. */
        if (pr_overlay_stub_target(ram, cpu->cs, (uint16_t)(cpu->ip + 0x32u),
                                   STUB_OFFSET, U3C2_ENTRY, &chain_cs) &&
            !pr_u3c2_0909_stubs_loaded(ram, chain_cs, pair_cs))
            return PR_U31_0255_ARGUMENT;
    }
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.entry = cpu->ip; m.pair_cs = pair_cs;
    m.max_caller_iterations = max_caller_iterations; m.max_line_iterations = max_line_iterations;
    m.max_sort_loopbacks = max_sort_loopbacks; m.callee = callee; m.callee_status = callee_status; m.stop = PR_U31_0255_OK;
    push16(&m, cpu->bp);                                                  /* +0255 PUSH BP; MOV BP,SP; SUB SP,4 (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    write8(&m, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);                    /* +025B MOV BYTE [BP-1],0 */
    set_al(cpu, read8(&m, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu)));         /* +025F MOV AL,[BP+0Ch]; PUSH AX */
    push16(&m, cpu->ax);
    set_al(cpu, read8(&m, cpu->ss, (uint16_t)(cpu->bp + 0x0Au)));         /* +0263 MOV AL,[BP+0Ah]; PUSH AX */
    push16(&m, cpu->ax);
    cpu->ax = read8(&m, cpu->ss, (uint16_t)(cpu->bp + 8u));               /* +0267 MOV AL,[BP+8]; XOR AH,AH; MOV DI,AX; MOV CL,4; */
    cpu->di = (uint16_t)(cpu->ax << 4);                                   /*       SHL DI,CL (flags folded) */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    cpu->ax = read8(&m, cpu->ds, (uint16_t)(cpu->di + 0x3209u));          /* +0272 MOV AL,[DI+3209h]; XOR AH,AH; PUSH AX */
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
    push16(&m, cpu->ax);
    set_al(cpu, 0xFFu);                                                   /* +0279 MOV AL,0FFh; PUSH AX; MOV AL,1; PUSH AX */
    push16(&m, cpu->ax);
    set_al(cpu, 0x01u);
    push16(&m, cpu->ax);
    push16(&m, read16(&m, cpu->ds, 0x6718u));                             /* +027F PUSH WORD [6718h]; PUSH WORD [6716h] */
    push16(&m, read16(&m, cpu->ds, 0x6716u));
    if (!u3c2_call(&m))                                                   /* +0287 */
        return m.stop;
    set_al(cpu, read8(&m, cpu->ds, 0x671Au));                             /* +028C MOV AL,[671Ah]; MOV [BP-3],AL */
    write8(&m, cpu->ss, (uint16_t)(cpu->bp - 3u), (uint8_t)cpu->ax);
    set_al(cpu, 1u);                                                      /* +0292 MOV AL,1; CMP AL,[BP-3]; JA +02E2 */
    count = read8(&m, cpu->ss, (uint16_t)(cpu->bp - 3u));
    cmp8_flags(cpu, 1u, count);
    if (count != 0u) {
        write8(&m, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);                /* +0299 MOV [BP-2],AL; JMP +02A1 */
        for (;;) {
            push16(&m, read16(&m, cpu->ds, 0x5D94u));                     /* +02A1 PUSH WORD [5D94h]; PUSH WORD [5D92h] */
            push16(&m, read16(&m, cpu->ds, 0x5D92u));
            if (!resident_call(&m))                                       /* +02A9 */
                return m.stop;
            cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (cpu->ax & 0x00FFu)); /* +02AE MOV DL,AL */
            cpu->ax = read8(&m, cpu->ss, (uint16_t)(cpu->bp - 2u));       /* +02B0 MOV AL,[BP-2]; XOR AH,AH; MOV DI,AX; MOV SI,DI; */
            cpu->si = cpu->ax;                                            /*       SHL DI,1; ADD DI,SI (flags folded) */
            cpu->di = (uint16_t)(cpu->ax * 3u);
            cpu->ax = read8(&m, cpu->ds, (uint16_t)(cpu->di + 0x6718u));  /* +02BD MOV AL,[DI+6718h]; XOR AH,AH; MOV DI,AX; */
            cpu->di = (uint16_t)(cpu->ax << 2);                           /*       SHL DI,1; SHL DI,1 (flags folded) */
            les(&m, cpu->ds, (uint16_t)(cpu->di + 0x65B9u));              /* +02C9 LES DI,[DI+65B9h]; MOV AL,ES:[DI+10Eh] */
            set_al(cpu, read8(&m, cpu->es, (uint16_t)(cpu->di + 0x010Eu)));
            if ((uint8_t)cpu->ax != (uint8_t)cpu->dx)                     /* +02D2 CMP AL,DL; JE +02DA; MOV BYTE [BP-1],1 */
                write8(&m, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);        /*       (flags folded) */
            set_al(cpu, read8(&m, cpu->ss, (uint16_t)(cpu->bp - 2u)));    /* +02DA MOV AL,[BP-2]; CMP AL,[BP-3]; JNE +029E */
            count = read8(&m, cpu->ss, (uint16_t)(cpu->bp - 3u));
            cmp8_flags(cpu, (uint8_t)cpu->ax, count);
            if ((uint8_t)cpu->ax == count)
                break;
            value = (uint8_t)(read8(&m, cpu->ss, (uint16_t)(cpu->bp - 2u)) + 1u); /* +029E INC BYTE [BP-2] */
            write8(&m, cpu->ss, (uint16_t)(cpu->bp - 2u), value);
            inc8_flags(cpu, value);
        }
    }
    set_al(cpu, read8(&m, cpu->ss, (uint16_t)(cpu->bp - 1u)));            /* +02E2 MOV AL,[BP-1] */
    leave_retf(&m, 8u);                                                   /* +02E5 MOV SP,BP; POP BP; RETF 8 */
    return PR_U31_0255_OK;
}
