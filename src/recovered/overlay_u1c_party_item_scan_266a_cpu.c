/* GAME.OVR unit 0x1C +266A (raw CPU, composing U25 +0000/+0136 through the overlay stub and SYSTEM FillChar);
 * docs/re/U1C_PARTY_ITEM_SCAN_266A.md. */
#include "poolrad/recovered/overlay_u1c_party_item_scan_266a_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"
#include "poolrad/recovered/overlay_u25_slot_value_0136.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#include <string.h>

#define MASK (PR_U1C_266A_RAM_SIZE - 1u)
#define ENTRY_IP 0x266Au

_Static_assert(sizeof(pr_u1c_266a_cpu) == sizeof(pr_u25_0000_cpu), "U25 +0000 CPU layout differs");
_Static_assert(sizeof(pr_u1c_266a_cpu) == sizeof(pr_u25_slot_value_cpu), "U25 +0136 CPU layout differs");
_Static_assert(sizeof(pr_u1c_266a_cpu) == sizeof(pr_system_leaf_cpu), "SYSTEM leaf CPU layout differs");

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u1c_266a_cpu *cpu;
    const pr_u1c_266a_limits *limits;
    uint16_t entry;
    pr_u1c_266a_callee *callee;
    int *callee_status;
    pr_u1c_266a_status stop;
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

static void sub_flags(pr_u1c_266a_cpu *cpu, uint16_t left, uint16_t right, unsigned bits)
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

/* OR AX,[BP-disp]: CF = OF = 0, ZF/SF/PF from the word, AF clear as both oracles leave it. Returns AX. */
static uint16_t or_word(machine *m, unsigned below)
{
    pr_u1c_266a_cpu *cpu = m->cpu;
    cpu->ax = (uint16_t)(cpu->ax | read16(m, cpu->ss, (uint16_t)(cpu->bp - below)));
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)cpu->ax)) cpu->flags |= 0x0004u;
    if (cpu->ax == 0u) cpu->flags |= 0x0040u;
    if (cpu->ax & 0x8000u) cpu->flags |= 0x0080u;
    return cpu->ax;
}

/* CMP BYTE [BP-0Bh],0: returns whether the found flag is set. */
static int found(machine *m)
{
    uint8_t value = read8(m, m->cpu->ss, (uint16_t)(m->cpu->bp - 0x0Bu));
    sub_flags(m->cpu, value, 0u, 8u);
    return value != 0u;
}

static void note(machine *m, pr_u1c_266a_callee which, int status)
{
    if (m->callee != NULL)
        *m->callee = which;
    if (m->callee_status != NULL)
        *m->callee_status = status;
}

/* The LCALL at original offset `site`. A stub call (stub_ip != 0xFFFF) requires `9A <offset> <seg>` and an EA jump at
 * seg:offset to IP `stub_ip`; a direct call requires `9A <entry>`. Pushes CS and the next IP, then runs `which`. */
static int far_call(machine *m, uint16_t site, uint16_t offset, uint16_t stub_ip, pr_u1c_266a_callee which)
{
    pr_u1c_266a_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs, target_cs, target_ip;
    int status;
    cpu->ip = at;
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != offset) {
        m->stop = PR_U1C_266A_CODE;
        return 0;
    }
    target_cs = read16(m, cpu->cs, (uint16_t)(at + 3u));
    target_ip = offset;
    if (stub_ip != 0xFFFFu) {
        if (read8(m, target_cs, offset) != 0xEAu || read16(m, target_cs, (uint16_t)(offset + 1u)) != stub_ip) {
            m->stop = PR_U1C_266A_CODE;
            return 0;
        }
        target_ip = stub_ip;
        target_cs = read16(m, target_cs, (uint16_t)(offset + 3u));
    }
    push16(m, caller_cs);
    push16(m, return_ip);
    cpu->cs = target_cs;
    cpu->ip = target_ip;
    if (which == PR_U1C_266A_LOAD_OPERANDS) {
        pr_u25_0000_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_u25_load_operands_0000(m->ram, m->size, &leaf, m->limits->system_cs, m->limits->max_operands,
                                                m->limits->max_string_iterations);
        memcpy(cpu, &leaf, sizeof leaf);
        note(m, which, status);
        if (status != (int)PR_U25_0000_OK) {
            m->stop = PR_U1C_266A_CALLEE;
            return 0;
        }
    } else if (which == PR_U1C_266A_SLOT_VALUE) {
        pr_u25_slot_value_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_u25_slot_value_0136(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
        note(m, which, status);
        if (status != (int)PR_U25_SLOT_VALUE_OK) {
            m->stop = PR_U1C_266A_CALLEE;
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
        m->stop = PR_U1C_266A_TRANSFER;
        return 0;
    }
    return 1;
}

/* LES DI,[BP-disp]; MOV AX,ES:[DI+field]; MOV DX,ES:[DI+field+2]; MOV [BP-low],AX; MOV [BP-high],DX */
static void follow(machine *m, unsigned from, uint16_t field, unsigned low, unsigned high)
{
    pr_u1c_266a_cpu *cpu = m->cpu;
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp - from));
    cpu->di = read16_at(m->ram, at);
    cpu->es = read16_at(m->ram, (at + 2u) & MASK);
    cpu->ax = read16(m, cpu->es, (uint16_t)(cpu->di + field));
    cpu->dx = read16(m, cpu->es, (uint16_t)(cpu->di + field + 2u));
    write16(m, cpu->ss, (uint16_t)(cpu->bp - low), cpu->ax);
    write16(m, cpu->ss, (uint16_t)(cpu->bp - high), cpu->dx);
}

static int loop_back(machine *m, unsigned *loopbacks, uint16_t head)
{
    if ((*loopbacks)++ == m->limits->max_loopbacks) {
        m->cpu->ip = here(m, head);
        m->stop = PR_U1C_266A_LIMIT;
        return 0;
    }
    return 1;
}

pr_u1c_266a_status pr_u1c_party_item_scan_266a_cpu(uint8_t *ram, size_t ram_size, pr_u1c_266a_cpu *cpu,
                                                   const pr_u1c_266a_limits *limits, pr_u1c_266a_callee *callee,
                                                   int *callee_status)
{
    machine m;
    unsigned loopbacks = 0u;
    size_t at;
    if (callee != NULL)
        *callee = PR_U1C_266A_NO_CALLEE;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || limits == NULL || ram_size != PR_U1C_266A_RAM_SIZE)
        return PR_U1C_266A_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.limits = limits; m.entry = cpu->ip;
    m.callee = callee; m.callee_status = callee_status; m.stop = PR_U1C_266A_OK;
    push16(&m, cpu->bp);                                                  /* +266A PUSH BP; MOV BP,SP; SUB SP,0Ch */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 0x0Cu, 16u);
    cpu->sp = (uint16_t)(cpu->sp - 0x0Cu);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);                       /* +2670 MOV AL,1; PUSH AX; LCALL stub:002A */
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x2673u, 0x002Au, 0x0000u, PR_U1C_266A_LOAD_OPERANDS))
        return m.stop;
    write8(&m, cpu->ss, (uint16_t)(cpu->bp - 0x0Bu), 0u);                 /* +2678 MOV BYTE [BP-0Bh],0 */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);                       /* +267C MOV AL,1; PUSH AX; LCALL stub:0025 */
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x267Fu, 0x0025u, 0x0136u, PR_U1C_266A_SLOT_VALUE))
        return m.stop;
    write8(&m, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);      /* +2684 MOV [BP-1],AL */
    cpu->di = 0x6F78u;                                                    /* +2687 MOV DI,6F78h; PUSH DS; PUSH DI */
    push16(&m, cpu->ds);
    push16(&m, cpu->di);
    cpu->ax = 6u;                                                         /* +268C MOV AX,6; PUSH AX; MOV AL,0; PUSH AX */
    push16(&m, cpu->ax);
    cpu->ax = (uint16_t)(cpu->ax & 0xFF00u);
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x2693u, 0x16A8u, 0xFFFFu, PR_U1C_266A_FILLCHAR))  /* +2693 LCALL 0AF8:16A8 */
        return m.stop;
    write8(&m, cpu->ds, 0x6F79u, 1u);                                     /* +2698 MOV BYTE [6F79h],1 */
    cpu->ax = read16(&m, cpu->ds, 0x5D96u);                               /* +269D MOV AX,[5D96h]; MOV DX,[5D98h]; [BP-6],[BP-4] */
    cpu->dx = read16(&m, cpu->ds, 0x5D98u);
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->dx);
    for (;;) {
        cpu->ax = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 6u));          /* +26AA MOV AX,[BP-6]; OR AX,[BP-4]; JE +271B */
        if (or_word(&m, 4u) == 0u || found(&m))                           /* +26B2 CMP BYTE [BP-0Bh],0; JNE +271B */
            break;
        follow(&m, 6u, 0x00C8u, 0x0Au, 8u);                               /* +26B8 q := p^[0C8h] into [BP-0Ah]/[BP-8] */
        for (;;) {
            cpu->ax = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 0x0Au));   /* +26CB MOV AX,[BP-0Ah]; OR AX,[BP-8]; JE +2706 */
            if (or_word(&m, 8u) == 0u || found(&m))                       /* +26D3 CMP BYTE [BP-0Bh],0; JNE +2706 */
                break;
            {
                uint8_t type;
                size_t q;
                cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(&m, cpu->ss, (uint16_t)(cpu->bp - 1u))); /* +26D9 MOV AL,[BP-1] */
                q = physical(cpu->ss, (uint16_t)(cpu->bp - 0x0Au));       /* +26DC LES DI,[BP-0Ah]; CMP AL,ES:[DI+2Eh]; JNE */
                cpu->di = read16_at(ram, q);
                cpu->es = read16_at(ram, (q + 2u) & MASK);
                type = read8(&m, cpu->es, (uint16_t)(cpu->di + 0x2Eu));
                sub_flags(cpu, (uint8_t)cpu->ax, type, 8u);
                if ((uint8_t)cpu->ax == type) {
                    write8(&m, cpu->ds, 0x6F78u, 1u);                     /* +26E5 MOV BYTE [6F78h],1; [6F79h],0; [BP-0Bh],1 */
                    write8(&m, cpu->ds, 0x6F79u, 0u);
                    write8(&m, cpu->ss, (uint16_t)(cpu->bp - 0x0Bu), 1u);
                }
            }
            follow(&m, 0x0Au, 0x002Au, 0x0Au, 8u);                        /* +26F3 q := q^[2Ah] */
            if (!loop_back(&m, &loopbacks, 0x26CBu))                      /* +2704 JMP +26CB */
                return m.stop;
        }
        follow(&m, 6u, 0x0104u, 6u, 4u);                                  /* +2706 p := p^[104h] */
        if (!loop_back(&m, &loopbacks, 0x26AAu))                          /* +2719 JMP +26AA */
            return m.stop;
    }
    cpu->sp = cpu->bp;                                                    /* +271B MOV SP,BP; POP BP; RETF */
    cpu->bp = read16(&m, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U1C_266A_OK;
}
