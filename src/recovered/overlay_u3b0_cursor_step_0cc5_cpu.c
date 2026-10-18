/* GAME.OVR unit 0x3B0 +0CC5 (raw CPU, composing the strict U3B0 +0B03); docs/re/U3B0_CURSOR_STEP_0CC5.md. */
#include "poolrad/recovered/overlay_u3b0_cursor_step_0cc5_cpu.h"
#include "poolrad/recovered/overlay_u3b0_step_scan_0b03_cpu.h"

#include <string.h>

#define MASK (PR_U3B0_0CC5_RAM_SIZE - 1u)
#define ENTRY_IP 0x0CC5u

_Static_assert(sizeof(pr_u3b0_0cc5_cpu) == sizeof(pr_u3b0_0b03_cpu), "U3B0 +0B03 CPU layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
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

static void push16(uint8_t *ram, pr_u3b0_0cc5_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t here(uint16_t entry, uint16_t offset)
{
    return (uint16_t)(entry + (uint16_t)(offset - ENTRY_IP));
}

/* MOV DI,[BP+6] (the static link L). */
static uint16_t link(const uint8_t *ram, pr_u3b0_0cc5_cpu *cpu)
{
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    return cpu->di;
}

/* MOV DI,[BP+6]; LES DI,SS:[DI+0Ah]: ES:DI := X. */
static void index_pointer(const uint8_t *ram, pr_u3b0_0cc5_cpu *cpu)
{
    size_t at = physical(cpu->ss, (uint16_t)(link(ram, cpu) + 0x0Au));
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
}

/* index_pointer; MOV ES:[DI],AX */
static void store_index(uint8_t *ram, pr_u3b0_0cc5_cpu *cpu)
{
    index_pointer(ram, cpu);
    write16(ram, cpu->es, cpu->di, cpu->ax);
}

/* index_pointer; CMP AX,ES:[DI]: returns AX >= X^ (signed JGE). */
static int ax_at_least_index(const uint8_t *ram, pr_u3b0_0cc5_cpu *cpu)
{
    index_pointer(ram, cpu);
    return (int16_t)cpu->ax >= (int16_t)read16(ram, cpu->es, cpu->di);
}

/* MOV AX,[6B29h]; MOV DI,[BP+6]; ADD AX,SS:[DI-59h]; DEC AX: AX := G + lim - 1. */
static void upper_bound(const uint8_t *ram, pr_u3b0_0cc5_cpu *cpu)
{
    cpu->ax = read16(ram, cpu->ds, 0x6B29u);
    cpu->ax = (uint16_t)(cpu->ax + read16(ram, cpu->ss, (uint16_t)(link(ram, cpu) - 0x59u)) - 1u);
}

/* MOV DI,[BP+6]; MOV AX,SS:[DI-56h]; DEC AX: AX := B - 1. */
static void second_bound(const uint8_t *ram, pr_u3b0_0cc5_cpu *cpu)
{
    cpu->ax = (uint16_t)(read16(ram, cpu->ss, (uint16_t)(link(ram, cpu) - 0x56u)) - 1u);
}

pr_u3b0_0cc5_status pr_u3b0_cursor_step_0cc5_cpu(uint8_t *ram, size_t ram_size, pr_u3b0_0cc5_cpu *cpu, unsigned max_loopbacks,
                                                 int *callee_status, int *leaf_status)
{
    uint16_t entry, caller_cs, return_ip;
    pr_u3b0_0b03_cpu step;
    pr_u3b0_0b03_status status;
    int leaf = -1;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (leaf_status != NULL)
        *leaf_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U3B0_0CC5_RAM_SIZE)
        return PR_U3B0_0CC5_ARGUMENT;
    entry = cpu->ip;
    push16(ram, cpu, cpu->bp);                                            /* +0CC5 PUSH BP; MOV BP,SP; SUB SP,8 (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    if (ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))] != 0u) {         /* +0CCB CMP BYTE [BP+8],0; JE +0D22 */
        index_pointer(ram, cpu);                                          /* +0CD1 INC WORD ES:[DI] */
        write16(ram, cpu->es, cpu->di, (uint16_t)(read16(ram, cpu->es, cpu->di) + 1u));
        upper_bound(ram, cpu);                                            /* +0CDB */
        if (!ax_at_least_index(ram, cpu)) {                               /* +0CE6 CMP AX,ES:[DI]; JGE +0CFF */
            cpu->ax = read16(ram, cpu->ds, 0x6B29u);                      /* +0CF2 */
            store_index(ram, cpu);
        }
        second_bound(ram, cpu);                                           /* +0CFF */
        if (!ax_at_least_index(ram, cpu)) {                               /* +0D07 CMP AX,ES:[DI]; JGE +0D20 */
            cpu->ax = read16(ram, cpu->ds, 0x6B29u);                      /* +0D13 */
            store_index(ram, cpu);
        }
    } else {
        index_pointer(ram, cpu);                                          /* +0D22 DEC WORD ES:[DI] */
        write16(ram, cpu->es, cpu->di, (uint16_t)(read16(ram, cpu->es, cpu->di) - 1u));
        index_pointer(ram, cpu);                                          /* +0D2C MOV AX,ES:[DI]; CMP AX,[6B29h]; JGE +0D51 */
        cpu->ax = read16(ram, cpu->es, cpu->di);
        if ((int16_t)cpu->ax < (int16_t)read16(ram, cpu->ds, 0x6B29u)) {
            upper_bound(ram, cpu);                                        /* +0D3C */
            store_index(ram, cpu);
        }
        second_bound(ram, cpu);                                           /* +0D51 */
        if (!ax_at_least_index(ram, cpu)) {                               /* +0D59 CMP AX,ES:[DI]; JGE +0D77 */
            second_bound(ram, cpu);                                       /* +0D65 */
            store_index(ram, cpu);
        }
    }
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))]); /* +0D77 MOV AL,[BP+8]; PUSH AX */
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));     /* PUSH WORD [BP+6] */
    caller_cs = cpu->cs;                                                  /* PUSH CS; CALL +0B03 */
    return_ip = here(entry, 0x0D82u);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = here(entry, 0x0B03u);
    memcpy(&step, cpu, sizeof step);
    status = pr_u3b0_step_scan_0b03_cpu(ram, ram_size, &step, max_loopbacks, &leaf);
    memcpy(cpu, &step, sizeof step);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (leaf_status != NULL)
        *leaf_status = leaf;
    if (status != PR_U3B0_0B03_OK)
        return PR_U3B0_0CC5_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U3B0_0CC5_TRANSFER;
    cpu->sp = cpu->bp;                                                    /* +0D82 MOV SP,BP; POP BP; RETF 4 */
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 4u);
    return PR_U3B0_0CC5_OK;
}
