/* GAME.OVR unit 0x3C6 +10A7 (raw CPU, composing entry 15 +0C5F folded here, the merged entry 16 +0CB9 and entry 5
 * +03A0 models, and SYSTEM FillChar); docs/re/U3C6_CALLER_10A7_CPU.md. */
#include "poolrad/recovered/overlay_u3c6_caller_10a7_cpu.h"
#include "poolrad/recovered/overlay_u3c6_0cb9_cpu.h"
#include "poolrad/recovered/overlay_u3c6_rebuild_03a0.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#include <string.h>

#define MASK (PR_U3C6_10A7_RAM_SIZE - 1u)
#define ENTRY_IP 0x10A7u

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
    pr_u3c6_10a7_cpu *cpu;
    unsigned loopbacks, max_loopbacks;
    size_t max_slots, max_cells;
    pr_u3c6_10a7_callee *callee;
    int *callee_status;
    pr_u3c6_10a7_status stop;
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

/* SS:[BP+disp] as a byte or word, disp taken modulo 64 KiB. */
static uint8_t local8(const machine *m, uint16_t disp)
{
    return read8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp));
}

static void set_local8(machine *m, uint16_t disp, uint8_t value)
{
    write8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp), value);
}

static uint16_t local16(const machine *m, uint16_t disp)
{
    return read16(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp));
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
    pr_u3c6_10a7_cpu *cpu = m->cpu;
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

/* The six arithmetic flags of left - right (subtract) or left + right in `bits` bits; the other flags are kept. */
static void arith_flags(pr_u3c6_10a7_cpu *cpu, uint32_t left, uint32_t right, unsigned bits, int subtract)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t result = (subtract ? left - right : left + right) & mask;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (subtract ? left < right : left + right > mask) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((subtract ? (left ^ right) & (left ^ result) : (left ^ result) & (right ^ result)) & sign) cpu->flags |= 0x0800u;
}

static void set_al(pr_u3c6_10a7_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void note(machine *m, pr_u3c6_10a7_callee which, int status)
{
    if (m->callee != NULL)
        *m->callee = which;
    if (m->callee_status != NULL)
        *m->callee_status = status;
}

/* MOV AL,[BP-2]; XOR AH,AH; MOV DI,AX; SHL DI,1; SHL DI,1: DI := 4s (flags folded unless `live`: then the last SHL's,
 * with the undefined AF set as the pinned DOSBox-X core sets it after a non-zero shift). */
static void slot_index(machine *m, int live)
{
    pr_u3c6_10a7_cpu *cpu = m->cpu;
    uint16_t before;
    cpu->ax = local8(m, (uint16_t)-2);
    before = (uint16_t)(cpu->ax << 1);
    cpu->di = (uint16_t)(before << 1);
    if (live) {
        cpu->flags &= (uint16_t)~0x08D5u;
        if (before & 0x8000u) cpu->flags |= 0x0001u;
        if (even_parity((uint8_t)cpu->di)) cpu->flags |= 0x0004u;
        cpu->flags |= 0x0010u;
        if (cpu->di == 0u) cpu->flags |= 0x0040u;
        if (cpu->di & 0x8000u) cpu->flags |= 0x0080u;
        if (((cpu->di ^ before) & 0x8000u) != 0u) cpu->flags |= 0x0800u;
    }
}

/* MOV AL,[BP-3]; XOR AH,AH; MOV DX,7; MUL DX; MOV DI,AX: DI := 7k, DX := 0 (flags folded: a later CMP, OR or ADD
 * defines all six before any stop). */
static void entry_index(machine *m)
{
    pr_u3c6_10a7_cpu *cpu = m->cpu;
    cpu->ax = (uint16_t)(local8(m, (uint16_t)-3) * 7u);
    cpu->dx = 0u;
    cpu->di = cpu->ax;
}

/* PUSH CS; CALL +0C5F (Q already pushed): entry 15, folded here in full state from its 32 original instructions; RETF 4.
 * It searches slots 1, 2, ... of the DS:65B9h table for Q until it matches or reaches the count DS:[5F2Ah], and leaves AL
 * = the slot, or 0. */
static int entry15(machine *m, uint16_t return_ip)
{
    pr_u3c6_10a7_cpu *cpu = m->cpu;
    uint16_t caller_cs = cpu->cs;
    uint8_t found;
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->ip = 0x0C5Fu;
    push16(m, cpu->bp);                                                   /* 0C5F PUSH BP; MOV BP,SP; SUB SP,4 (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    set_local8(m, (uint16_t)-2, 0u);                                      /* 0C65 found := 0; result := 0; i := 0 */
    set_local8(m, (uint16_t)-1, 0u);
    set_local8(m, (uint16_t)-3, 0u);
    for (;;) {
        set_local8(m, (uint16_t)-3, (uint8_t)(local8(m, (uint16_t)-3) + 1u)); /* 0C71 INC BYTE [BP-3] (flags folded) */
        cpu->ax = local8(m, (uint16_t)-3);                                /* 0C74 MOV AL,[BP-3]; XOR AH,AH; MOV DI,AX; SHL DI,1 x2 */
        cpu->di = (uint16_t)(cpu->ax << 2);
        cpu->ax = read16(m, cpu->ds, (uint16_t)(cpu->di + 0x65B9u));      /* 0C7F MOV AX,[DI+65B9h]; MOV DX,[DI+65BBh] */
        cpu->dx = read16(m, cpu->ds, (uint16_t)(cpu->di + 0x65BBu));
        if (cpu->dx == local16(m, 8u) && cpu->ax == local16(m, 6u))       /* 0C87 CMP DX,[BP+8]; JNE; CMP AX,[BP+6]; JNE */
            set_local8(m, (uint16_t)-2, 1u);                              /* 0C91 MOV BYTE [BP-2],1 (flags folded) */
        if (local8(m, (uint16_t)-2) != 0u)                                /* 0C95 CMP BYTE [BP-2],0; JNE 0CA4 */
            break;
        set_al(cpu, local8(m, (uint16_t)-3));                             /* 0C9B MOV AL,[BP-3]; CMP AL,[5F2Ah]; JNE 0C71 */
        if ((uint8_t)cpu->ax == read8(m, cpu->ds, 0x5F2Au))
            break;
    }
    found = local8(m, (uint16_t)-2);                                      /* 0CA4 CMP BYTE [BP-2],0; JE 0CB0 */
    arith_flags(cpu, found, 0u, 8u, 1);
    if (found != 0u) {
        set_al(cpu, local8(m, (uint16_t)-3));                             /* 0CAA MOV AL,[BP-3]; MOV [BP-1],AL */
        set_local8(m, (uint16_t)-1, (uint8_t)cpu->ax);
    }
    set_al(cpu, local8(m, (uint16_t)-1));                                 /* 0CB0 MOV AL,[BP-1] */
    leave_retf(m, 4u);                                                    /* 0CB3 MOV SP,BP; POP BP; RETF 4 */
    note(m, PR_U3C6_10A7_ENTRY15, 0);
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U3C6_10A7_TRANSFER;
        return 0;
    }
    return 1;
}

/* PUSH CS; CALL +0CB9 (seven words already pushed): entry 16, the merged raw-CPU model under WRAPPED_SS; RETF 0Eh. */
static int entry16(machine *m, uint16_t return_ip)
{
    pr_u3c6_10a7_cpu *cpu = m->cpu;
    uint16_t caller_cs = cpu->cs;
    pr_u3c6_0cb9_cpu state;
    pr_u3c6_0cb9_cpu_status status;
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->ip = 0x0CB9u;
    COPY_CPU(&state, cpu);
    status = pr_u3c6_0cb9_cpu_run(m->ram, m->size, &state, PR_U3C6_0CB9_RETF_WRAPPED_SS);
    COPY_CPU(cpu, &state);
    note(m, PR_U3C6_10A7_ENTRY16, (int)status);
    if (status != PR_U3C6_0CB9_CPU_RETURNED) {
        m->stop = PR_U3C6_10A7_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U3C6_10A7_TRANSFER;
        return 0;
    }
    return 1;
}

/* PUSH CS; CALL +03A0: entry 5, the merged raw-CPU model, its SYSTEM segment read from its FillChar site operand; RETF. */
static int entry5(machine *m, uint16_t return_ip)
{
    pr_u3c6_10a7_cpu *cpu = m->cpu;
    uint16_t caller_cs = cpu->cs;
    pr_u3c6_03a0_cpu state;
    pr_u3c6_03a0_status status;
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->ip = 0x03A0u;
    COPY_CPU(&state, cpu);
    status = pr_u3c6_rebuild_03a0(m->ram, m->size, &state, read16(m, caller_cs, 0x03B5u), m->max_slots, m->max_cells);
    COPY_CPU(cpu, &state);
    note(m, PR_U3C6_10A7_ENTRY5, (int)status);
    if (status != PR_U3C6_03A0_OK) {
        m->stop = PR_U3C6_10A7_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U3C6_10A7_TRANSFER;
        return 0;
    }
    return 1;
}

/* +11DF LCALL 0AF8:16A8 (arguments already pushed): SYSTEM FillChar. */
static int fillchar(machine *m)
{
    pr_u3c6_10a7_cpu *cpu = m->cpu;
    uint16_t caller_cs = cpu->cs, segment;
    pr_system_leaf_cpu state;
    int status;
    cpu->ip = 0x11DFu;
    if (read8(m, cpu->cs, 0x11DFu) != 0x9Au || read16(m, cpu->cs, 0x11E0u) != 0x16A8u) {
        m->stop = PR_U3C6_10A7_CODE;
        return 0;
    }
    segment = read16(m, cpu->cs, 0x11E2u);
    push16(m, caller_cs);
    push16(m, 0x11E4u);
    cpu->cs = segment;
    cpu->ip = 0x16A8u;
    COPY_CPU(&state, cpu);
    status = (int)pr_system_fillchar_16a8(m->ram, m->size, &state);
    COPY_CPU(cpu, &state);
    note(m, PR_U3C6_10A7_FILLCHAR, status);
    if (cpu->ip != 0x11E4u || cpu->cs != caller_cs) {
        m->stop = PR_U3C6_10A7_TRANSFER;
        return 0;
    }
    return 1;
}

/* +116D..+1284: loop 1 clears every 7-byte entry naming Q (keeping its byte +6 at BP-5 unless 1Fh), loop 2 looks for
 * an entry at (X, Y), and if none was seen the map cell takes BP-5. Returns 0 at a stop. */
static int entries(machine *m)
{
    pr_u3c6_10a7_cpu *cpu = m->cpu;
    uint8_t count, value;
    set_al(cpu, read8(m, cpu->ds, 0x6715u));                              /* +116D MOV AL,[6715h]; MOV [BP-7],AL */
    set_local8(m, (uint16_t)-7, (uint8_t)cpu->ax);
    set_al(cpu, 1u);                                                      /* +1173 MOV AL,1; CMP AL,[BP-7]; JA +11EC */
    count = local8(m, (uint16_t)-7);
    arith_flags(cpu, 1u, count, 8u, 1);
    if (count != 0u) {
        set_local8(m, (uint16_t)-3, 1u);                                  /* +117A MOV [BP-3],AL; JMP +1182 */
        for (;;) {
            entry_index(m);                                               /* +1182 DI := 7k; MOV AX,[DI+66D6h]; MOV DX,[DI+66D8h] */
            cpu->ax = read16(m, cpu->ds, (uint16_t)(cpu->di + 0x66D6u));
            cpu->dx = read16(m, cpu->ds, (uint16_t)(cpu->di + 0x66D8u));
            if (cpu->dx == local16(m, 0x0Eu) && cpu->ax == local16(m, 0x0Cu)) { /* +1196 CMP DX,[BP+0Eh]; JNE; CMP AX,[BP+0Ch]; JNE */
                entry_index(m);                                           /* +11A0 CMP BYTE [DI+66DCh],1Fh; JE +11C6 (flags folded) */
                if (read8(m, cpu->ds, (uint16_t)(cpu->di + 0x66DCu)) != 0x1Fu) {
                    entry_index(m);                                       /* +11B3 MOV AL,[DI+66DCh]; MOV [BP-5],AL */
                    set_al(cpu, read8(m, cpu->ds, (uint16_t)(cpu->di + 0x66DCu)));
                    set_local8(m, (uint16_t)-5, (uint8_t)cpu->ax);
                }
                entry_index(m);                                           /* +11C6 ADD DI,66D6h; PUSH DS; PUSH DI; MOV AX,7; */
                arith_flags(cpu, cpu->di, 0x66D6u, 16u, 0);               /*       PUSH AX; MOV AL,0; PUSH AX */
                cpu->di = (uint16_t)(cpu->di + 0x66D6u);
                push16(m, cpu->ds);
                push16(m, cpu->di);
                cpu->ax = 0x0007u;
                push16(m, cpu->ax);
                cpu->ax = 0x0000u;
                push16(m, cpu->ax);
                if (!fillchar(m))                                         /* +11DF FillChar(DS:66D6h + 7k, 7, 0) */
                    return 0;
            }
            set_al(cpu, local8(m, (uint16_t)-3));                         /* +11E4 MOV AL,[BP-3]; CMP AL,[BP-7]; JNE +117F */
            count = local8(m, (uint16_t)-7);
            arith_flags(cpu, (uint8_t)cpu->ax, count, 8u, 1);
            if ((uint8_t)cpu->ax == count)
                break;
            if (m->loopbacks == m->max_loopbacks) {                       /* the taken JNE +117F, past the cap */
                cpu->ip = 0x117Fu;
                m->stop = PR_U3C6_10A7_LIMIT;
                return 0;
            }
            m->loopbacks++;
            set_local8(m, (uint16_t)-3, (uint8_t)(local8(m, (uint16_t)-3) + 1u)); /* +117F INC BYTE [BP-3] (flags folded) */
        }
    }
    set_local8(m, (uint16_t)-6, 0u);                                      /* +11EC MOV BYTE [BP-6],0 */
    set_al(cpu, read8(m, cpu->ds, 0x6715u));                              /* +11F0 MOV AL,[6715h]; MOV [BP-7],AL */
    set_local8(m, (uint16_t)-7, (uint8_t)cpu->ax);
    set_al(cpu, 1u);                                                      /* +11F6 MOV AL,1; CMP AL,[BP-7]; JA +125F */
    count = local8(m, (uint16_t)-7);
    arith_flags(cpu, 1u, count, 8u, 1);
    if (count != 0u) {
        set_local8(m, (uint16_t)-3, 1u);                                  /* +11FD MOV [BP-3],AL; JMP +1205 */
        for (;;) {
            entry_index(m);                                               /* +1205 MOV AX,[DI+66D6h]; OR AX,[DI+66D8h]; JE +1257 */
            cpu->ax = read16(m, cpu->ds, (uint16_t)(cpu->di + 0x66D6u));  /*       (flags folded) */
            cpu->ax |= read16(m, cpu->ds, (uint16_t)(cpu->di + 0x66D8u));
            if (cpu->ax != 0u) {
                cpu->ax = local8(m, 0x0Au);                               /* +121B MOV AL,[BP+0Ah]; XOR AH,AH; MOV CX,AX */
                cpu->cx = cpu->ax;
                entry_index(m);                                           /* +1222 MOV AL,[DI+66DAh]; CBW; CMP AX,CX; JNE +1257 */
                cpu->ax = (uint16_t)(int16_t)(int8_t)read8(m, cpu->ds, (uint16_t)(cpu->di + 0x66DAu));
                if (cpu->ax == cpu->cx) {
                    cpu->ax = local8(m, 8u);                              /* +1237 MOV AL,[BP+8]; XOR AH,AH; MOV CX,AX */
                    cpu->cx = cpu->ax;
                    entry_index(m);                                       /* +123E MOV AL,[DI+66DBh]; CBW; CMP AX,CX; JNE +1257 */
                    cpu->ax = (uint16_t)(int16_t)(int8_t)read8(m, cpu->ds, (uint16_t)(cpu->di + 0x66DBu));
                    if (cpu->ax == cpu->cx)
                        set_local8(m, (uint16_t)-6, 1u);                  /* +1253 MOV BYTE [BP-6],1 */
                }
            }
            set_al(cpu, local8(m, (uint16_t)-3));                         /* +1257 MOV AL,[BP-3]; CMP AL,[BP-7]; JNE +1202 */
            count = local8(m, (uint16_t)-7);
            arith_flags(cpu, (uint8_t)cpu->ax, count, 8u, 1);
            if ((uint8_t)cpu->ax == count)
                break;
            set_local8(m, (uint16_t)-3, (uint8_t)(local8(m, (uint16_t)-3) + 1u)); /* +1202 INC BYTE [BP-3] (flags folded) */
        }
    }
    value = local8(m, (uint16_t)-6);                                      /* +125F CMP BYTE [BP-6],0; JNE +1285 */
    arith_flags(cpu, value, 0u, 8u, 1);
    if (value == 0u) {
        cpu->bx = (uint16_t)((cpu->bx & 0xFF00u) | local8(m, (uint16_t)-5)); /* +1265 MOV BL,[BP-5] */
        cpu->ax = local8(m, 0x0Au);                                       /* +1268 MOV AL,[BP+0Ah]; XOR AH,AH; MOV CX,AX */
        cpu->cx = cpu->ax;
        cpu->ax = (uint16_t)(local8(m, 8u) * 0x32u);                      /* +126F MOV AL,[BP+8]; XOR AH,AH; MOV DX,32h; MUL DX */
        cpu->dx = 0u;                                                     /*       (flags folded) */
        les(m, cpu->ds, 0x6716u);                                         /* +1279 LES DI,[6716h]; ADD DI,AX; ADD DI,CX */
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        arith_flags(cpu, cpu->di, cpu->cx, 16u, 0);
        cpu->di = (uint16_t)(cpu->di + cpu->cx);
        write8(m, cpu->es, (uint16_t)(cpu->di + 7u), (uint8_t)cpu->bx);   /* +1281 MOV ES:[DI+7],BL */
    }
    return 1;
}

pr_u3c6_10a7_status pr_u3c6_caller_10a7_cpu(uint8_t *ram, size_t ram_size, pr_u3c6_10a7_cpu *cpu, unsigned max_loopbacks,
                                            size_t max_slots, size_t max_cells, pr_u3c6_10a7_callee *callee, int *callee_status)
{
    machine m;
    uint8_t value;
    int fail;
    if (callee != NULL)
        *callee = PR_U3C6_10A7_NO_CALLEE;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U3C6_10A7_RAM_SIZE || cpu->ip != ENTRY_IP || cpu->sp < 0x0100u
        || cpu->sp > 0xFFF0u || max_slots == 0u || max_cells == 0u)
        return PR_U3C6_10A7_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.max_loopbacks = max_loopbacks; m.max_slots = max_slots;
    m.max_cells = max_cells; m.callee = callee; m.callee_status = callee_status; m.stop = PR_U3C6_10A7_OK;
    push16(&m, cpu->bp);                                                  /* +10A7 PUSH BP; MOV BP,SP; SUB SP,8 (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    value = read8(&m, cpu->ds, 0x49F3u);                                  /* +10AD CMP BYTE [49F3h],5; JE +10B7 */
    arith_flags(cpu, value, 5u, 8u, 1);
    if (value != 5u) {
        set_local8(&m, (uint16_t)-1, 1u);                                 /* +10B4 JMP +128B; MOV BYTE [BP-1],1 */
    } else {
        set_local8(&m, (uint16_t)-1, 0u);                                 /* +10B7 MOV BYTE [BP-1],0 */
        push16(&m, local16(&m, 0x0Eu));                                   /* +10BB PUSH WORD [BP+0Eh]; PUSH WORD [BP+0Ch] */
        push16(&m, local16(&m, 0x0Cu));
        if (!entry15(&m, 0x10C5u))                                        /* +10C1 PUSH CS; CALL +0C5F */
            return m.stop;
        set_local8(&m, (uint16_t)-2, (uint8_t)cpu->ax);                   /* +10C5 MOV [BP-2],AL */
        les(&m, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));                    /* +10C8 LES DI,[BP+0Ch]; MOV AL,ES:[DI+6Ch]; AND AL,7Fh; */
        set_al(cpu, (uint8_t)(read8(&m, cpu->es, (uint16_t)(cpu->di + 0x6Cu)) & 0x7Fu)); /* MOV DL,AL (flags folded) */
        cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (cpu->ax & 0x00FFu));
        slot_index(&m, 0);                                                /* +10D3 the slot's type */
        write8(&m, cpu->ds, (uint16_t)(cpu->di + 0x5F2Au), (uint8_t)cpu->dx);
        cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | local8(&m, 0x0Au));    /* +10E2 MOV DL,[BP+0Ah]: X */
        slot_index(&m, 0);
        write8(&m, cpu->ds, (uint16_t)(cpu->di + 0x5F27u), (uint8_t)cpu->dx);
        cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | local8(&m, 8u));       /* +10F4 MOV DL,[BP+8]: Y */
        slot_index(&m, 1);                                                /* (these SHL flags reach +0CB9's entry) */
        write8(&m, cpu->ds, (uint16_t)(cpu->di + 0x5F28u), (uint8_t)cpu->dx);
        push16(&m, local16(&m, 0x0Eu));                                   /* +1106 PUSH Q; MOV AL,8; PUSH AX; LEA DI,[BP-4]; PUSH SS; */
        push16(&m, local16(&m, 0x0Cu));                                   /*       PUSH DI; LEA DI,[BP-5]; PUSH SS; PUSH DI */
        set_al(cpu, 8u);
        push16(&m, cpu->ax);
        cpu->di = (uint16_t)(cpu->bp - 4u);
        push16(&m, cpu->ss);
        push16(&m, cpu->di);
        cpu->di = (uint16_t)(cpu->bp - 5u);
        push16(&m, cpu->ss);
        push16(&m, cpu->di);
        if (!entry16(&m, 0x111Du))                                        /* +1119 PUSH CS; CALL +0CB9 */
            return m.stop;
        fail = local8(&m, (uint16_t)-4) != 0u;                            /* +111D CMP BYTE [BP-4],0; JNE +113B (flags folded) */
        if (!fail)
            fail = local8(&m, (uint16_t)-5) == 0u;                        /* +1123 CMP BYTE [BP-5],0; JE +113B (flags folded) */
        if (!fail) {
            cpu->ax = local8(&m, (uint16_t)-5);                           /* +1129 MOV AL,[BP-5]; XOR AH,AH; MOV DI,AX; SHL DI,1 x2 */
            cpu->di = (uint16_t)(cpu->ax << 2);
            fail = read8(&m, cpu->ds, (uint16_t)(cpu->di + 0x2744u)) == 0xFFu; /* +1134 CMP BYTE [DI+2744h],0FFh; JNE +114E */
        }
        if (fail) {
            slot_index(&m, 1);                                            /* +113B the slot's type := 0; JMP +1289; JMP +128F */
            write8(&m, cpu->ds, (uint16_t)(cpu->di + 0x5F2Au), 0u);       /*       (its SHL flags are the final flags) */
        } else {
            set_local8(&m, (uint16_t)-1, 1u);                             /* +114E MOV BYTE [BP-1],1 */
            value = local8(&m, 6u);                                       /* +1152 CMP BYTE [BP+6],0; JNE +115B; JMP +1285 */
            arith_flags(cpu, value, 0u, 8u, 1);
            if (value != 0u) {
                les(&m, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));            /* +115B LES DI,[BP+0Ch]; LES DI,ES:[DI+108h] */
                les(&m, cpu->es, (uint16_t)(cpu->di + 0x0108u));
                value = read8(&m, cpu->es, (uint16_t)(cpu->di + 0x13u));  /* +1163 CMP BYTE ES:[DI+13h],0; JE +116D; JMP +1285 */
                arith_flags(cpu, value, 0u, 8u, 1);
                if (value == 0u && !entries(&m))
                    return m.stop;
            }
            if (!entry5(&m, 0x1289u))                                     /* +1285 PUSH CS; CALL +03A0; JMP +128F */
                return m.stop;
        }
    }
    set_al(cpu, local8(&m, (uint16_t)-1));                                /* +128F MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 0Ah */
    leave_retf(&m, 0x0Au);
    return PR_U3C6_10A7_OK;
}
