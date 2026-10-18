/* GAME.OVR unit 0x1C +0AED ECL COMPARE AND (raw CPU, composing U25 +0000/+0136 through the overlay stub and SYSTEM
 * FillChar); docs/re/U1C_COMPARE_AND_0AED_CPU.md. */
#include "poolrad/recovered/overlay_u1c_compare_and_0aed_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"
#include "poolrad/recovered/overlay_u25_slot_value_0136.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#include <string.h>

#define MASK (PR_U1C_AND_0AED_RAM_SIZE - 1u)
#define ENTRY_IP 0x0AEDu

_Static_assert(sizeof(pr_u1c_and_0aed_cpu) == sizeof(pr_u25_0000_cpu), "U25 +0000 CPU layout differs");
_Static_assert(sizeof(pr_u1c_and_0aed_cpu) == sizeof(pr_u25_slot_value_cpu), "U25 +0136 CPU layout differs");
_Static_assert(sizeof(pr_u1c_and_0aed_cpu) == sizeof(pr_system_leaf_cpu), "SYSTEM leaf CPU layout differs");

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u1c_and_0aed_cpu *cpu;
    const pr_u1c_and_0aed_limits *limits;
    uint16_t entry;
    pr_u1c_and_0aed_callee *callee;
    int *callee_status;
    pr_u1c_and_0aed_status stop;
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

static void write8(machine *m, uint16_t segment, uint16_t offset, uint8_t value)
{
    m->ram[physical(segment, offset)] = value;
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

/* SUB/CMP left,right in `bits` bits: the six arithmetic flags; the other flags are kept. */
static void sub_flags(pr_u1c_and_0aed_cpu *cpu, uint16_t left, uint16_t right, unsigned bits)
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

/* INC BYTE: OF/SF/ZF/AF/PF from the increment, CF kept. Returns the new byte. */
static uint8_t inc8(pr_u1c_and_0aed_cpu *cpu, uint8_t value)
{
    uint8_t result = (uint8_t)(value + 1u);
    cpu->flags &= (uint16_t)~0x08D4u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((value ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if (result == 0x80u) cpu->flags |= 0x0800u;
    return result;
}

static void note(machine *m, pr_u1c_and_0aed_callee which, int status)
{
    if (m->callee != NULL)
        *m->callee = which;
    if (m->callee_status != NULL)
        *m->callee_status = status;
}

/* The LCALL at original offset `site`. A stub call (stub_ip != 0xFFFF) requires `9A <offset> <seg>` and an EA jump at
 * seg:offset to IP `stub_ip`; a direct call requires `9A <entry>`. Pushes CS and the next IP, then runs `which`. */
static int far_call(machine *m, uint16_t site, uint16_t offset, uint16_t stub_ip, pr_u1c_and_0aed_callee which)
{
    pr_u1c_and_0aed_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs, target_cs, target_ip;
    int status;
    cpu->ip = at;
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != offset) {
        m->stop = PR_U1C_AND_0AED_CODE;
        return 0;
    }
    target_cs = read16(m, cpu->cs, (uint16_t)(at + 3u));
    target_ip = offset;
    if (stub_ip != 0xFFFFu) {
        if (read8(m, target_cs, offset) != 0xEAu || read16(m, target_cs, (uint16_t)(offset + 1u)) != stub_ip) {
            m->stop = PR_U1C_AND_0AED_CODE;
            return 0;
        }
        target_ip = stub_ip;
        target_cs = read16(m, target_cs, (uint16_t)(offset + 3u));
    }
    push16(m, caller_cs);
    push16(m, return_ip);
    cpu->cs = target_cs;
    cpu->ip = target_ip;
    if (which == PR_U1C_AND_0AED_LOAD_OPERANDS) {
        pr_u25_0000_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_u25_load_operands_0000(m->ram, m->size, &leaf, m->limits->system_cs, m->limits->max_operands,
                                                m->limits->max_string_iterations);
        memcpy(cpu, &leaf, sizeof leaf);
        note(m, which, status);
        if (status != (int)PR_U25_0000_OK) {
            m->stop = PR_U1C_AND_0AED_CALLEE;
            return 0;
        }
    } else if (which == PR_U1C_AND_0AED_SLOT_VALUE) {
        pr_u25_slot_value_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_u25_slot_value_0136(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
        note(m, which, status);
        if (status != (int)PR_U25_SLOT_VALUE_OK) {
            m->stop = PR_U1C_AND_0AED_CALLEE;
            return 0;
        }
    } else {
        pr_system_leaf_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_system_fillchar_16a8(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
        note(m, which, status);
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U1C_AND_0AED_TRANSFER;
        return 0;
    }
    return 1;
}

/* MOV AX,[BP-disp]; CMP AX,[BP-next]: returns whether the two words are equal. */
static int same(machine *m, uint16_t disp, uint16_t next)
{
    pr_u1c_and_0aed_cpu *cpu = m->cpu;
    uint16_t right;
    cpu->ax = read16(m, cpu->ss, (uint16_t)(cpu->bp - disp));
    right = read16(m, cpu->ss, (uint16_t)(cpu->bp - next));
    sub_flags(cpu, cpu->ax, right, 16u);
    return cpu->ax == right;
}

pr_u1c_and_0aed_status pr_u1c_compare_and_0aed_cpu(uint8_t *ram, size_t ram_size, pr_u1c_and_0aed_cpu *cpu,
                                                   const pr_u1c_and_0aed_limits *limits, pr_u1c_and_0aed_callee *callee,
                                                   int *callee_status)
{
    machine m;
    size_t at;
    if (callee != NULL)
        *callee = PR_U1C_AND_0AED_NO_CALLEE;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || limits == NULL || ram_size != PR_U1C_AND_0AED_RAM_SIZE)
        return PR_U1C_AND_0AED_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.limits = limits; m.entry = cpu->ip;
    m.callee = callee; m.callee_status = callee_status; m.stop = PR_U1C_AND_0AED_OK;
    push16(&m, cpu->bp);                                                  /* +0AED PUSH BP; MOV BP,SP; SUB SP,0Ah */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 0x0Au, 16u);
    cpu->sp = (uint16_t)(cpu->sp - 0x0Au);
    cpu->di = 0x6F78u;                                                    /* +0AF3 MOV DI,6F78h; PUSH DS; PUSH DI */
    push16(&m, cpu->ds);
    push16(&m, cpu->di);
    cpu->ax = 6u;                                                         /* +0AF8 MOV AX,6; PUSH AX; MOV AL,0; PUSH AX */
    push16(&m, cpu->ax);
    cpu->ax = (uint16_t)(cpu->ax & 0xFF00u);
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x0AFFu, 0x16A8u, 0xFFFFu, PR_U1C_AND_0AED_FILLCHAR))  /* +0AFF FillChar(DS:6F78h, 6, 0) */
        return m.stop;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 4u);                       /* +0B04 MOV AL,4; PUSH AX; LCALL stub:002A */
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x0B07u, 0x002Au, 0x0000u, PR_U1C_AND_0AED_LOAD_OPERANDS))
        return m.stop;
    write8(&m, cpu->ss, (uint16_t)(cpu->bp - 9u), 1u);                    /* +0B0C MOV BYTE [BP-9],1; JMP +0B15 */
    for (;;) {
        uint8_t i = read8(&m, cpu->ss, (uint16_t)(cpu->bp - 9u));         /* +0B15 MOV AL,[BP-9]; PUSH AX; LCALL stub:0025 */
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | i);
        push16(&m, cpu->ax);
        if (!far_call(&m, 0x0B19u, 0x0025u, 0x0136u, PR_U1C_AND_0AED_SLOT_VALUE))
            return m.stop;
        cpu->dx = cpu->ax;                                                /* +0B1E MOV DX,AX; MOV AL,[BP-9]; XOR AH,AH; MOV DI,AX; SHL DI,1 */
        cpu->ax = read8(&m, cpu->ss, (uint16_t)(cpu->bp - 9u));           /* (XOR and SHL flags folded) */
        cpu->di = (uint16_t)(cpu->ax << 1);
        write16(&m, cpu->ss, (uint16_t)(cpu->bp + cpu->di - 0x0Au), cpu->dx);  /* +0B29 MOV [BP+DI-0Ah],DX */
        i = read8(&m, cpu->ss, (uint16_t)(cpu->bp - 9u));                 /* +0B2C CMP BYTE [BP-9],4; JNE +0B12 */
        sub_flags(cpu, i, 4u, 8u);
        if (i == 4u)
            break;
        at = physical(cpu->ss, (uint16_t)(cpu->bp - 9u));                 /* +0B12 INC BYTE [BP-9] */
        ram[at] = inc8(cpu, ram[at]);
    }
    if (same(&m, 8u, 6u) && same(&m, 4u, 2u))                            /* +0B32 v[1] = v[2] and v[3] = v[4] */
        write8(&m, cpu->ds, 0x6F78u, 1u);                                 /* +0B42 MOV BYTE [6F78h],1 */
    else
        write8(&m, cpu->ds, 0x6F79u, 1u);                                 /* +0B49 MOV BYTE [6F79h],1 */
    cpu->sp = cpu->bp;                                                    /* +0B4E MOV SP,BP; POP BP; RETF */
    cpu->bp = read16(&m, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U1C_AND_0AED_OK;
}
