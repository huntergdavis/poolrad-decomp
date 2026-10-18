/* GAME.OVR unit 0x7F +00A8 (raw CPU, composing SYSTEM Move, LongInt multiply and divide); docs/re/U7F_WEIGHTED_00A8.md. */
#include "poolrad/recovered/overlay_u7f_weighted_00a8_cpu.h"
#include "poolrad/recovered/system_longdiv_0a76_cpu.h"
#include "poolrad/recovered/system_longmul_0a5b_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#include <string.h>

#define MASK (PR_U7F_00A8_RAM_SIZE - 1u)
#define ENTRY_IP 0x00A8u

_Static_assert(sizeof(pr_u7f_00a8_cpu) == sizeof(pr_system_leaf_cpu), "SYSTEM leaf CPU layout differs");
_Static_assert(sizeof(pr_u7f_00a8_cpu) == sizeof(pr_system_longmul_cpu), "SYSTEM multiply CPU layout differs");
_Static_assert(sizeof(pr_u7f_00a8_cpu) == sizeof(pr_system_longdiv_cpu), "SYSTEM divide CPU layout differs");

typedef enum { MOVE, MULTIPLY, DIVIDE } callee;

static const uint16_t callee_entry[3] = {0x0A3Fu, 0x0A5Bu, 0x0A76u};

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u7f_00a8_cpu *cpu;
    uint16_t entry;
    int *callee_status;
    pr_u7f_00a8_status stop;
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

/* SZP from a word result into the cleared arithmetic flags. */
static void szp(pr_u7f_00a8_cpu *cpu, uint16_t result)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
}

/* SUB SP,26h: live only when the Move site is a CODE stop. */
static void sub_flags(pr_u7f_00a8_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if ((left ^ right) & (left ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    szp(cpu, result);
}

/* SHL r16,1: CF the bit shifted out, OF = MSB ^ CF, AF clear as Unicorn leaves it. */
static uint16_t shl1(pr_u7f_00a8_cpu *cpu, uint16_t value)
{
    uint16_t result = (uint16_t)(value << 1);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (value & 0x8000u) cpu->flags |= 0x0001u;
    if (((result >> 15) ^ (value >> 15)) & 1u) cpu->flags |= 0x0800u;
    szp(cpu, result);
    return result;
}

/* ADC left,0 with carry in `carry`: live only when the divide site is a CODE stop. */
static uint16_t adc0(pr_u7f_00a8_cpu *cpu, uint16_t left, unsigned carry)
{
    uint16_t result = (uint16_t)(left + carry);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (carry && left == 0xFFFFu) cpu->flags |= 0x0001u;
    if ((left ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (~left & result & 0x8000u) cpu->flags |= 0x0800u;
    szp(cpu, result);
    return result;
}

/* LCALL at original offset `site` (arguments already pushed) to the `kind` callee; returns 1 when it came back to the next instruction. */
static int far_call(machine *m, uint16_t site, callee kind)
{
    pr_u7f_00a8_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs;
    int status;
    cpu->ip = at;
    if (m->ram[physical(cpu->cs, at)] != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != callee_entry[kind]) {
        m->stop = PR_U7F_00A8_CODE;
        return 0;
    }
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->cs = read16(m, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = callee_entry[kind];
    if (kind == MOVE) {
        pr_system_leaf_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_system_move_0a3f(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
    } else if (kind == MULTIPLY) {
        pr_system_longmul_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_system_longmul_0a5b(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
    } else {
        pr_system_longdiv_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_system_longdiv_0a76(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
    }
    if (m->callee_status != NULL)
        *m->callee_status = status;
    if (status != 0) {
        m->stop = PR_U7F_00A8_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U7F_00A8_TRANSFER;
        return 0;
    }
    return 1;
}

/* MOV AL,[BP-25h]; CBW; MOV DI,AX; SHL DI,1; SHL DI,1: DI := 4 * j (the second SHL's flags are computed). */
static void scale_j(machine *m)
{
    pr_u7f_00a8_cpu *cpu = m->cpu;
    uint8_t j = m->ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x25u))];
    cpu->ax = (uint16_t)(j & 0x80u ? 0xFF00u | j : j);
    cpu->di = (uint16_t)(cpu->ax << 1);
    cpu->di = shl1(cpu, cpu->di);
}

pr_u7f_00a8_status pr_u7f_weighted_00a8_cpu(uint8_t *ram, size_t ram_size, pr_u7f_00a8_cpu *cpu, int *callee_status)
{
    machine m;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U7F_00A8_RAM_SIZE)
        return PR_U7F_00A8_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.entry = cpu->ip; m.callee_status = callee_status; m.stop = PR_U7F_00A8_OK;
    push16(&m, cpu->bp);                                                  /* +00A8 PUSH BP; MOV BP,SP; SUB SP,26h */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 0x26u);
    cpu->sp = (uint16_t)(cpu->sp - 0x26u);
    at = physical(cpu->ss, (uint16_t)(cpu->bp + 6u));                     /* +00AE LES DI,[BP+6]; PUSH ES; PUSH DI */
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    push16(&m, cpu->es);
    push16(&m, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x20u);                                /* +00B3 LEA DI,[BP-20h]; PUSH SS; PUSH DI; MOV AX,1Ch; PUSH AX */
    push16(&m, cpu->ss);
    push16(&m, cpu->di);
    cpu->ax = 0x001Cu;
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x00BCu, MOVE))                                     /* +00BC Move(P^, v, 28) */
        return m.stop;
    cpu->ax = 0u;                                                         /* +00C1 XOR AX,AX (flags folded); sum := 0; j := 0 */
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 0x24u), cpu->ax);
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 0x22u), cpu->ax);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x25u))] = 0u;
    for (;;) {
        uint16_t old, added;
        scale_j(&m);                                                      /* +00D2 MOV CX,[BP+DI-20h]; MOV BX,[BP+DI-1Eh] */
        cpu->cx = read16(&m, cpu->ss, (uint16_t)(cpu->bp + cpu->di - 0x20u));
        cpu->bx = read16(&m, cpu->ss, (uint16_t)(cpu->bp + cpu->di - 0x1Eu));
        scale_j(&m);                                                      /* +00E2 MOV AX,[DI+0D24h]; MOV DX,[DI+0D26h] */
        cpu->ax = read16(&m, cpu->ds, (uint16_t)(cpu->di + 0x0D24u));
        cpu->dx = read16(&m, cpu->ds, (uint16_t)(cpu->di + 0x0D26u));
        if (!far_call(&m, 0x00F4u, MULTIPLY))                             /* +00F4 DX:AX := w[j] * v[j] */
            return m.stop;
        old = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 0x24u));           /* +00F9 ADD [BP-24h],AX; ADC [BP-22h],DX (flags folded) */
        added = (uint16_t)(old + cpu->ax);
        write16(&m, cpu->ss, (uint16_t)(cpu->bp - 0x24u), added);
        write16(&m, cpu->ss, (uint16_t)(cpu->bp - 0x22u),
                (uint16_t)(read16(&m, cpu->ss, (uint16_t)(cpu->bp - 0x22u)) + cpu->dx + (added < old)));
        if (ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x25u))] == 4u)   /* +00FF CMP BYTE [BP-25h],4; JNE +00CF (flags folded) */
            break;
        at = physical(cpu->ss, (uint16_t)(cpu->bp - 0x25u));              /* +00CF INC BYTE [BP-25h] (flags folded) */
        ram[at] = (uint8_t)(ram[at] + 1u);
    }
    {
        uint16_t low = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 0x24u));  /* +0105 MOV AX/DX,sum; ADD AX,64h; ADC DX,0 */
        cpu->dx = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 0x22u));
        cpu->ax = (uint16_t)(low + 0x64u);
        cpu->dx = adc0(cpu, cpu->dx, cpu->ax < low);
    }
    cpu->cx = read16(&m, cpu->ds, 0x0D30u);                               /* +0111 MOV CX,[0D30h]; MOV BX,[0D32h] */
    cpu->bx = read16(&m, cpu->ds, 0x0D32u);
    if (!far_call(&m, 0x0119u, DIVIDE))                                   /* +0119 DX:AX := (sum + 100) div DS:[0D30h] */
        return m.stop;
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);              /* +011E MOV [BP-4],AX; MOV [BP-2],DX; reload */
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->dx = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;                                                    /* +012A MOV SP,BP; POP BP; RETF 4 */
    cpu->bp = read16(&m, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 4u);
    return PR_U7F_00A8_OK;
}
