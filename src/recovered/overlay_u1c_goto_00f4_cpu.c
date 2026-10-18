/* GAME.OVR unit 0x1C +00F4 ECL GOTO (raw CPU, composing U25 +0000 and +06AF through the overlay stub);
 * docs/re/U1C_GOTO_00F4_CPU.md. */
#include "poolrad/recovered/overlay_u1c_goto_00f4_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"

#include <string.h>

#define MASK (PR_U1C_GOTO_RAM_SIZE - 1u)
#define ENTRY_IP 0x00F4u

_Static_assert(sizeof(pr_u1c_goto_cpu) == sizeof(pr_u25_0000_cpu), "U25 CPU layout differs");

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u1c_goto_cpu *cpu;
    const pr_u1c_goto_limits *limits;
    uint16_t entry;
    pr_u1c_goto_callee *callee;
    int *callee_status;
    pr_u1c_goto_status stop;
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

static uint8_t read8(const machine *m, uint16_t segment, uint16_t offset)
{
    return m->ram[physical(segment, offset)];
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

static void set_al(pr_u1c_goto_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

/* The stub LCALL at original offset `site`: `9A <offset> <seg>` with an EA jump at seg:offset to IP `stub_ip`. Pushes
 * CS and the next IP, then runs `which`; returns 1 when it came back to the next instruction. */
static int far_call(machine *m, uint16_t site, uint16_t offset, uint16_t stub_ip, pr_u1c_goto_callee which)
{
    pr_u1c_goto_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs, stub_cs;
    pr_u25_0000_cpu leaf;
    int status;
    cpu->ip = at;
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != offset) {
        m->stop = PR_U1C_GOTO_CODE;
        return 0;
    }
    stub_cs = read16(m, cpu->cs, (uint16_t)(at + 3u));
    if (read8(m, stub_cs, offset) != 0xEAu || read16(m, stub_cs, (uint16_t)(offset + 1u)) != stub_ip) {
        m->stop = PR_U1C_GOTO_CODE;
        return 0;
    }
    push16(m, caller_cs);
    push16(m, return_ip);
    cpu->cs = read16(m, stub_cs, (uint16_t)(offset + 3u));
    cpu->ip = stub_ip;
    memcpy(&leaf, cpu, sizeof leaf);
    if (which == PR_U1C_GOTO_LOAD_OPERANDS)
        status = (int)pr_u25_load_operands_0000(m->ram, m->size, &leaf, m->limits->system_cs, m->limits->max_operands,
                                                m->limits->max_string_iterations);
    else
        status = (int)pr_u25_word_06af(m->ram, m->size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (m->callee != NULL)
        *m->callee = which;
    if (m->callee_status != NULL)
        *m->callee_status = status;
    if (status != (int)PR_U25_0000_OK) {
        m->stop = PR_U1C_GOTO_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U1C_GOTO_TRANSFER;
        return 0;
    }
    return 1;
}

pr_u1c_goto_status pr_u1c_goto_00f4_cpu(uint8_t *ram, size_t ram_size, pr_u1c_goto_cpu *cpu,
                                             const pr_u1c_goto_limits *limits, pr_u1c_goto_callee *callee,
                                             int *callee_status)
{
    machine m;
    size_t at;
    if (callee != NULL)
        *callee = PR_U1C_GOTO_NO_CALLEE;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || limits == NULL || ram_size != PR_U1C_GOTO_RAM_SIZE)
        return PR_U1C_GOTO_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.limits = limits; m.entry = cpu->ip;
    m.callee = callee; m.callee_status = callee_status; m.stop = PR_U1C_GOTO_OK;
    push16(&m, cpu->bp);                                                  /* +00F4 PUSH BP; MOV BP,SP */
    cpu->bp = cpu->sp;
    set_al(cpu, 1u);                                                      /* +00F7 MOV AL,1; PUSH AX; LCALL stub:002A */
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x00FAu, 0x002Au, 0x0000u, PR_U1C_GOTO_LOAD_OPERANDS))
        return m.stop;
    set_al(cpu, read8(&m, cpu->ds, 0x7086u));                             /* +00FF MOV AL,[7086h]; PUSH AX */
    push16(&m, cpu->ax);
    set_al(cpu, read8(&m, cpu->ds, 0x7046u));                             /* +0103 MOV AL,[7046h]; PUSH AX; LCALL stub:004D */
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x0107u, 0x004Du, 0x06AFu, PR_U1C_GOTO_WORD))
        return m.stop;
    write16(&m, cpu->ds, 0x49EDu, cpu->ax);                               /* +010C MOV [49EDh],AX */
    cpu->bp = read16(&m, cpu->ss, cpu->sp);                               /* +010F POP BP; RETF */
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U1C_GOTO_OK;
}
