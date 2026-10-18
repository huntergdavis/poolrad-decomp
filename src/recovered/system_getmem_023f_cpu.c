/* START.EXE SYSTEM GetMem 0AF8:023F with its near core 0308, size rounding 0491 and default HeapError 00A9 (raw CPU);
 * docs/re/SYSTEM_GETMEM_023F.md. */
#include "poolrad/recovered/system_getmem_023f_cpu.h"

#include <string.h>

#define MASK (PR_SYSTEM_GETMEM_RAM_SIZE - 1u)
#define CF 0x0001u
#define ZF 0x0040u

typedef struct {
    uint8_t *ram;
    pr_system_getmem_cpu *cpu;
    uint16_t code_cs;
    unsigned steps, max_steps;
    pr_system_getmem_status stop;
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

static uint16_t pop16(machine *m)
{
    uint16_t value = read16(m, m->cpu->ss, m->cpu->sp);
    m->cpu->sp = (uint16_t)(m->cpu->sp + 2u);
    return value;
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

/* OR, XOR or AND: CF = OF = AF = 0, ZF/SF/PF from the result whose sign bit is `sign`; the other flags are kept. */
static void logic_flags(pr_system_getmem_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= ZF;
    if (result & sign) cpu->flags |= 0x0080u;
}

/* CMP left,right in `bits` bits: the six arithmetic flags. */
static void cmp_flags(pr_system_getmem_cpu *cpu, uint32_t left, uint32_t right, unsigned bits)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u, result = (left - right) & mask;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= CF;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= ZF;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & sign) cpu->flags |= 0x0800u;
}

/* A near RET (n = 0) back to `expected`; anything else stops with TRANSFER. */
static int near_ret(machine *m, uint16_t expected)
{
    m->cpu->ip = pop16(m);
    if (m->cpu->ip != expected) {
        m->stop = PR_SYSTEM_GETMEM_TRANSFER;
        return 0;
    }
    return 1;
}

/* ADD AX,DI; ADD DX,SI; ADD AL,0F0h; ADC DX,0; AND AL,0Fh: DX:AX := the normalized sum (flags folded but AND's). */
static void normalized_sum(pr_system_getmem_cpu *cpu)
{
    uint16_t low = (uint16_t)(cpu->ax + cpu->di);
    cpu->dx = (uint16_t)(cpu->dx + cpu->si);
    if ((low & 0x00FFu) >= 0x10u)                                        /* ADD AL,0F0h carries; ADC DX,0 */
        cpu->dx = (uint16_t)(cpu->dx + 1u);
    cpu->ax = (uint16_t)((low & 0xFF00u) | (uint8_t)((low + 0xF0u) & 0x0Fu));
    logic_flags(cpu, (uint8_t)cpu->ax, 0x80u);
}

/* CALL 0491 from +030F: ADD AX,7; MOV DX,AX; RCR DX,1; SHR DX,1 x3; AND AX,8; RET. */
static int round_up(machine *m)
{
    pr_system_getmem_cpu *cpu = m->cpu;
    uint32_t sum = (uint32_t)cpu->ax + 7u;
    push16(m, 0x0312u);
    cpu->ip = 0x0491u;
    cpu->ax = (uint16_t)sum;
    cpu->dx = (uint16_t)(sum >> 4);                                       /* the 17-bit sum through RCR, then three SHR */
    cpu->ax &= 0x0008u;
    logic_flags(cpu, cpu->ax, 0x8000u);
    return near_ret(m, 0x0312u);
}

/* LCALL [4414h] at `site`, its argument pushed: the default HeapError CS:00A9 (XOR AX,AX; RETF 2) is folded; any other
 * pointer stops at the site. */
static int heap_error(machine *m, uint16_t site)
{
    pr_system_getmem_cpu *cpu = m->cpu;
    size_t at;
    cpu->ip = site;
    if (read16(m, cpu->ds, 0x4414u) != 0x00A9u || read16(m, cpu->ds, 0x4416u) != m->code_cs) {
        m->stop = PR_SYSTEM_GETMEM_HEAP_ERROR;
        return 0;
    }
    push16(m, cpu->cs);
    push16(m, (uint16_t)(site + 4u));
    cpu->ip = 0x00A9u;
    cpu->ax = 0u;                                                         /* 00A9 XOR AX,AX; RETF 2 */
    logic_flags(cpu, 0u, 0x8000u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(m->ram, at);
    cpu->cs = read16_at(m->ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    if (cpu->ip != (uint16_t)(site + 4u) || cpu->cs != m->code_cs) {
        m->stop = PR_SYSTEM_GETMEM_TRANSFER;
        return 0;
    }
    return 1;
}

/* CALL 0308 from +0245: the core. Returns 1 when its RET came back to +0248. */
static int core(machine *m)
{
    pr_system_getmem_cpu *cpu = m->cpu;
    uint16_t value;
    int exact;
    push16(m, 0x0248u);
    cpu->ip = 0x0308u;
    logic_flags(cpu, cpu->ax, 0x8000u);                                   /* 0308 OR AX,AX; JE 0383 */
    if (cpu->ax == 0u)
        goto nil;
    write16(m, cpu->ds, 0x8754u, cpu->ax);                                /* 030C MOV [8754h],AX */
    if (!round_up(m))                                                     /* 030F CALL 0491 */
        return 0;
    cpu->di = 0x440Cu;                                                    /* 0312 MOV DI,440Ch; MOV CX,DS; MOV ES,CX */
    cpu->cx = cpu->ds;
    cpu->es = cpu->cx;
    for (;;) {
        cpu->cx = cpu->di;                                                /* 0319 MOV CX,DI; MOV BX,ES: the link */
        cpu->bx = cpu->es;
        les(m, cpu->es, cpu->di);                                         /* 031D LES DI,ES:[DI]; MOV SI,ES */
        cpu->si = cpu->es;
        value = read16(m, cpu->ds, 0x4406u);                              /* 0322 CMP SI,[4406h]; JE 0387 */
        cmp_flags(cpu, cpu->si, value, 16u);
        if (cpu->si == value)
            goto top;
        value = read16(m, cpu->es, (uint16_t)(cpu->di + 6u));             /* 0328 CMP DX,ES:[DI+6]; JA 0319; JB 0336 */
        cmp_flags(cpu, cpu->dx, value, 16u);
        if (cpu->dx == value) {
            value = read16(m, cpu->es, (uint16_t)(cpu->di + 4u));         /* 0330 CMP AX,ES:[DI+4]; JA 0319 */
            cmp_flags(cpu, cpu->ax, value, 16u);
            if (cpu->ax <= value)
                break;
        } else if (cpu->dx < value) {
            break;
        }
        if (m->steps == m->max_steps) {                                   /* the taken JA 0319, past the cap */
            cpu->ip = 0x0319u;
            m->stop = PR_SYSTEM_GETMEM_LIMIT;
            return 0;
        }
        m->steps++;
    }
    exact = (cpu->flags & ZF) != 0u;                                      /* 0336 PUSH ES; PUSH DI; PUSH BX; PUSH CX */
    push16(m, cpu->es);
    push16(m, cpu->di);
    push16(m, cpu->bx);
    push16(m, cpu->cx);
    cpu->cx = read16(m, cpu->es, cpu->di);                                /* 033A MOV CX,ES:[DI]; MOV BX,ES:[DI+2]: the next */
    cpu->bx = read16(m, cpu->es, (uint16_t)(cpu->di + 2u));
    if (!exact) {                                                         /* 0341 JE 0376 */
        uint32_t rest;
        push16(m, cpu->bx);                                               /* 0343 PUSH BX; PUSH CX */
        push16(m, cpu->cx);
        cpu->cx = read16(m, cpu->es, (uint16_t)(cpu->di + 4u));           /* 0345 MOV CX,ES:[DI+4]; MOV BX,ES:[DI+6] */
        cpu->bx = read16(m, cpu->es, (uint16_t)(cpu->di + 6u));
        rest = ((uint32_t)cpu->bx << 16 | cpu->cx) - ((uint32_t)cpu->dx << 16 | cpu->ax); /* 034D SUB CX,AX; SBB BX,DX */
        cpu->cx = (uint16_t)(rest & 0x000Fu);                             /* 0351 AND CX,0Fh */
        cpu->bx = (uint16_t)(rest >> 16);
        normalized_sum(cpu);                                              /* 0354..035D: the remainder's address */
        cpu->di = cpu->ax;                                                /* 035F MOV DI,AX; MOV ES,DX */
        cpu->es = cpu->dx;
        value = pop16(m);                                                 /* 0363 POP ES:[DI]; POP ES:[DI+2] */
        write16(m, cpu->es, cpu->di, value);
        value = pop16(m);
        write16(m, cpu->es, (uint16_t)(cpu->di + 2u), value);
        write16(m, cpu->es, (uint16_t)(cpu->di + 4u), cpu->cx);           /* 036A MOV ES:[DI+4],CX; MOV ES:[DI+6],BX */
        write16(m, cpu->es, (uint16_t)(cpu->di + 6u), cpu->bx);
        cpu->cx = cpu->di;                                                /* 0372 MOV CX,DI; MOV BX,ES */
        cpu->bx = cpu->es;
    }
    cpu->di = pop16(m);                                                   /* 0376 POP DI; POP ES: the link */
    cpu->es = pop16(m);
    write16(m, cpu->es, cpu->di, cpu->cx);                                /* 0378 MOV ES:[DI],CX; MOV ES:[DI+2],BX */
    write16(m, cpu->es, (uint16_t)(cpu->di + 2u), cpu->bx);
    cpu->ax = pop16(m);                                                   /* 037F POP AX; POP DX: the block */
    cpu->dx = pop16(m);
    cpu->flags &= (uint16_t)~CF;                                          /* 0381 CLC; RET */
    return near_ret(m, 0x0248u);

top:
    normalized_sum(cpu);                                                  /* 0387..0390 HeapPtr + size, normalized */
    value = read16(m, cpu->ds, 0x440Au);                                  /* 0392 CMP DX,[440Ah]; JB 03A0; JA 03BF */
    cmp_flags(cpu, cpu->dx, value, 16u);
    if (cpu->dx > value)
        goto fail;
    if (cpu->dx == value) {
        value = read16(m, cpu->ds, 0x4408u);                              /* 039A CMP AX,[4408h]; JA 03BF */
        cmp_flags(cpu, cpu->ax, value, 16u);
        if (cpu->ax > value)
            goto fail;
    }
    push16(m, cpu->es);                                                   /* 03A0 PUSH ES; PUSH DI: the block */
    push16(m, cpu->di);
    cpu->di = cpu->cx;                                                    /* 03A2 MOV DI,CX; MOV ES,BX */
    cpu->es = cpu->bx;
    write16(m, cpu->ds, 0x4404u, cpu->ax);                                /* 03A6 HeapPtr := the new top */
    write16(m, cpu->ds, 0x4406u, cpu->dx);
    write16(m, cpu->es, cpu->di, cpu->ax);                                /* 03AD the last link := the new top */
    write16(m, cpu->es, (uint16_t)(cpu->di + 2u), cpu->dx);
    cpu->ax = 0u;                                                         /* 03B4 XOR AX,AX; PUSH AX; LCALL [4414h] */
    logic_flags(cpu, 0u, 0x8000u);
    push16(m, cpu->ax);
    if (!heap_error(m, 0x03B7u))
        return 0;
    cpu->ax = pop16(m);                                                   /* 03BB POP AX; POP DX; CLC; RET */
    cpu->dx = pop16(m);
    cpu->flags &= (uint16_t)~CF;
    return near_ret(m, 0x0248u);

fail:
    push16(m, read16(m, cpu->ds, 0x8754u));                               /* 03BF PUSH WORD [8754h]; LCALL [4414h] */
    if (!heap_error(m, 0x03C3u))
        return 0;
    cmp_flags(cpu, (uint8_t)cpu->ax, 1u, 8u);                             /* 03C7 CMP AL,1; JB 0382 (the default leaves AL 0) */
    return near_ret(m, 0x0248u);                                          /* 0382 RET with CF set */

nil:
    cpu->ax = 0u;                                                         /* 0383 XOR AX,AX; CWD; RET */
    logic_flags(cpu, 0u, 0x8000u);
    cpu->dx = 0u;
    return near_ret(m, 0x0248u);
}

pr_system_getmem_status pr_system_getmem_023f(uint8_t *ram, size_t ram_size, pr_system_getmem_cpu *cpu, unsigned max_steps)
{
    machine m;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_GETMEM_RAM_SIZE || cpu->ip != 0x023Fu)
        return PR_SYSTEM_GETMEM_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.cpu = cpu; m.code_cs = cpu->cs; m.max_steps = max_steps; m.stop = PR_SYSTEM_GETMEM_RETURNED;
    push16(&m, cpu->bp);                                                  /* 023F PUSH BP; MOV BP,SP; MOV AX,[BP+6] */
    cpu->bp = cpu->sp;
    cpu->ax = read16(&m, cpu->ss, (uint16_t)(cpu->bp + 6u));
    if (!core(&m))                                                        /* 0245 CALL 0308 */
        return m.stop;
    cpu->bp = pop16(&m);                                                  /* 0248 POP BP; JB 024E */
    if (cpu->flags & CF) {
        cpu->ax = 0x00CBu;                                                /* 024E MOV AX,0CBh; JMP 00E2 */
        cpu->ip = 0x00E2u;
        return PR_SYSTEM_GETMEM_RUNERROR;
    }
    at = physical(cpu->ss, cpu->sp);                                      /* 024B RETF 2 */
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_SYSTEM_GETMEM_RETURNED;
}
