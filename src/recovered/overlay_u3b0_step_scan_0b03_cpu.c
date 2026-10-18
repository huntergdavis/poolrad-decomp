/* GAME.OVR unit 0x3B0 +0B03 (raw CPU, composing the strict U3B0 +0000); docs/re/U3B0_STEP_SCAN_0B03.md. */
#include "poolrad/recovered/overlay_u3b0_step_scan_0b03_cpu.h"
#include "poolrad/recovered/overlay_u3b0_entry1_0000.h"

#include <string.h>

#define MASK (PR_U3B0_0B03_RAM_SIZE - 1u)
#define ENTRY_IP 0x0B03u

_Static_assert(sizeof(pr_u3b0_0b03_cpu) == sizeof(pr_u3b0_0000_cpu), "U3B0 +0000 CPU layout differs");

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u3b0_0b03_cpu *cpu;
    uint16_t entry;
    int *callee_status;
    pr_u3b0_0b03_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
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

static uint16_t here(const machine *m, uint16_t offset)
{
    return (uint16_t)(m->entry + (uint16_t)(offset - ENTRY_IP));
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp_flags(pr_u3b0_0b03_cpu *cpu, uint16_t left, uint16_t right, unsigned bits)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t result = ((uint32_t)left - right) & mask;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & sign) cpu->flags |= 0x0800u;
}

/* MOV DI,[BP+6] (the static link L). */
static uint16_t link(machine *m)
{
    m->cpu->di = read16(m, m->cpu->ss, (uint16_t)(m->cpu->bp + 6u));
    return m->cpu->di;
}

/* MOV DI,[BP+6]; LES DI,SS:[DI+0Ah]: ES:DI := X. */
static void index_pointer(machine *m)
{
    size_t at = physical(m->cpu->ss, (uint16_t)(link(m) + 0x0Au));
    m->cpu->di = read16_at(m->ram, at);
    m->cpu->es = read16_at(m->ram, (at + 2u) & MASK);
}

/* MOV AX,[BP-2]; MOV DI,[BP+6]; CMP AX,SS:[DI-59h]: returns count - limit's signed order (< 0 when count < limit). */
static int count_below_limit(machine *m)
{
    pr_u3b0_0b03_cpu *cpu = m->cpu;
    uint16_t limit;
    cpu->ax = read16(m, cpu->ss, (uint16_t)(cpu->bp - 2u));
    limit = read16(m, cpu->ss, (uint16_t)(link(m) - 0x59u));
    cmp_flags(cpu, cpu->ax, limit, 16u);
    return (int16_t)cpu->ax < (int16_t)limit;
}

/* The node fetch and flag test (+0B23..+0B45 or +0BAE..+0BD0); returns 1 to continue, 0 on a zero flag or a stop. */
static int fetch_node(machine *m, uint16_t return_offset)
{
    pr_u3b0_0b03_cpu *cpu = m->cpu;
    uint16_t caller_cs, return_ip = here(m, return_offset);
    pr_u3b0_0000_cpu leaf;
    pr_u3b0_0000_status status;
    uint8_t flag;
    index_pointer(m);                                                     /* PUSH WORD ES:[DI] */
    push16(m, read16(m, cpu->es, cpu->di));
    push16(m, read16(m, cpu->ss, (uint16_t)(link(m) + 0x16u)));           /* MOV DI,[BP+6]; PUSH SS:[DI+16h]; PUSH SS:[DI+14h] */
    push16(m, read16(m, cpu->ss, (uint16_t)(cpu->di + 0x14u)));
    caller_cs = cpu->cs;                                                  /* PUSH CS; CALL +0000 */
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->ip = here(m, 0x0000u);
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_u3b0_entry1_0000(m->ram, m->size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (m->callee_status != NULL)
        *m->callee_status = (int)status;
    if (status != PR_U3B0_0000_OK) {
        m->stop = PR_U3B0_0B03_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U3B0_0B03_TRANSFER;
        return 0;
    }
    cpu->di = cpu->ax;                                                    /* MOV DI,AX; MOV ES,DX; CMP BYTE ES:[DI+29h],0; JE */
    cpu->es = cpu->dx;
    flag = m->ram[physical(cpu->es, (uint16_t)(cpu->di + 0x29u))];
    cmp_flags(cpu, flag, 0u, 8u);
    if (flag == 0u)
        return 0;
    write16(m, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint16_t)(read16(m, cpu->ss, (uint16_t)(cpu->bp - 2u)) + 1u)); /* INC WORD [BP-2] */
    return 1;
}

/* AX := DS:[6B29] + L[-59h] - 1 (MOV AX,[6B29h]; MOV DI,[BP+6]; ADD AX,SS:[DI-59h]; DEC AX; flags folded). */
static void upper_bound(machine *m)
{
    pr_u3b0_0b03_cpu *cpu = m->cpu;
    cpu->ax = read16(m, cpu->ds, 0x6B29u);
    cpu->ax = (uint16_t)(cpu->ax + read16(m, cpu->ss, (uint16_t)(link(m) - 0x59u)) - 1u);
}

/* MOV DI,[BP+6]; MOV AX,SS:[DI-56h]; DEC AX (flags folded). */
static void second_bound(machine *m)
{
    pr_u3b0_0b03_cpu *cpu = m->cpu;
    cpu->ax = (uint16_t)(read16(m, cpu->ss, (uint16_t)(link(m) - 0x56u)) - 1u);
}

/* MOV DI,[BP+6]; LES DI,SS:[DI+0Ah]; MOV ES:[DI],AX */
static void store_index(machine *m)
{
    index_pointer(m);
    write16(m, m->cpu->es, m->cpu->di, m->cpu->ax);
}

/* CMP AX,ES:[DI] after index_pointer; returns AX >= X^ (signed JGE). */
static int ax_at_least_index(machine *m)
{
    pr_u3b0_0b03_cpu *cpu = m->cpu;
    uint16_t value;
    index_pointer(m);
    value = read16(m, cpu->es, cpu->di);
    cmp_flags(cpu, cpu->ax, value, 16u);
    return (int16_t)cpu->ax >= (int16_t)value;
}

static int loop_back(machine *m, unsigned *loopbacks, unsigned max_loopbacks, uint16_t head)
{
    if ((*loopbacks)++ == max_loopbacks) {
        m->cpu->ip = here(m, head);
        m->stop = PR_U3B0_0B03_LIMIT;
        return 0;
    }
    return 1;
}

pr_u3b0_0b03_status pr_u3b0_step_scan_0b03_cpu(uint8_t *ram, size_t ram_size, pr_u3b0_0b03_cpu *cpu, unsigned max_loopbacks,
                                               int *callee_status)
{
    machine m;
    unsigned loopbacks = 0u;
    uint8_t direction;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U3B0_0B03_RAM_SIZE)
        return PR_U3B0_0B03_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.entry = cpu->ip; m.callee_status = callee_status; m.stop = PR_U3B0_0B03_OK;
    push16(&m, cpu->bp);                                                  /* +0B03 PUSH BP; MOV BP,SP; SUB SP,2 (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    cpu->ax = 0u;                                                         /* +0B09 XOR AX,AX; MOV [BP-2],AX */
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);
    direction = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))];         /* +0B0E CMP BYTE [BP+8],0; JNE +0B17 */
    cmp_flags(cpu, direction, 0u, 8u);
    if (direction != 0u) {
        for (;;) {                                                        /* +0B17 */
            if (!count_below_limit(&m))
                break;                                                    /* JGE +0B9C -> +0C2D */
            if (!fetch_node(&m, 0x0B3Cu)) {
                if (m.stop != PR_U3B0_0B03_OK)
                    return m.stop;
                break;                                                    /* JE +0B9C */
            }
            index_pointer(&m);                                            /* +0B4A INC WORD ES:[DI] */
            write16(&m, cpu->es, cpu->di, (uint16_t)(read16(&m, cpu->es, cpu->di) + 1u));
            upper_bound(&m);                                              /* +0B54 */
            if (!ax_at_least_index(&m)) {                                 /* +0B5F CMP AX,ES:[DI]; JGE +0B78 */
                cpu->ax = read16(&m, cpu->ds, 0x6B29u);                   /* +0B6B */
                store_index(&m);
            }
            second_bound(&m);                                             /* +0B78 */
            if (!ax_at_least_index(&m)) {                                 /* +0B83 CMP AX,ES:[DI]; JGE +0B99 */
                cpu->ax = read16(&m, cpu->ds, 0x6B29u);                   /* +0B8C */
                store_index(&m);
            }
            if (!loop_back(&m, &loopbacks, max_loopbacks, 0x0B17u))      /* +0B99 JMP +0B17 */
                return m.stop;
        }
    } else {
        for (;;) {                                                        /* +0B9F */
            if (!count_below_limit(&m))
                break;                                                    /* JL +0BAE, else JMP +0C2D */
            if (!fetch_node(&m, 0x0BC7u)) {
                if (m.stop != PR_U3B0_0B03_OK)
                    return m.stop;
                break;                                                    /* JE +0C2D */
            }
            index_pointer(&m);                                            /* +0BD5 DEC WORD ES:[DI] */
            write16(&m, cpu->es, cpu->di, (uint16_t)(read16(&m, cpu->es, cpu->di) - 1u));
            index_pointer(&m);                                            /* +0BDF MOV AX,ES:[DI]; CMP AX,[6B29h]; JGE +0C04 */
            cpu->ax = read16(&m, cpu->es, cpu->di);
            {
                uint16_t floor = read16(&m, cpu->ds, 0x6B29u);
                cmp_flags(cpu, cpu->ax, floor, 16u);
                if ((int16_t)cpu->ax < (int16_t)floor) {
                    upper_bound(&m);                                      /* +0BEF */
                    store_index(&m);
                }
            }
            second_bound(&m);                                             /* +0C04 */
            if (!ax_at_least_index(&m)) {                                 /* +0C0F CMP AX,ES:[DI]; JGE +0C2A */
                second_bound(&m);                                         /* +0C18 */
                store_index(&m);
            }
            if (!loop_back(&m, &loopbacks, max_loopbacks, 0x0B9Fu))      /* +0C2A JMP +0B9F */
                return m.stop;
        }
    }
    cpu->sp = cpu->bp;                                                    /* +0C2D MOV SP,BP; POP BP; RETF 4 */
    cpu->bp = read16(&m, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 4u);
    return PR_U3B0_0B03_OK;
}
