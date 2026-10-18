/* GAME.OVR unit 0x7F +0FD2 (raw CPU, composing the U9E entry-8 dice chain through the loaded stub 00B0:0048);
 * docs/re/U7F_ROLL_CLASS_0FD2_CPU.md. */
#include "poolrad/recovered/overlay_u7f_roll_class_0fd2_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <string.h>

#define MASK (PR_U7F_0FD2_RAM_SIZE - 1u)
#define ENTRY_IP 0x0FD2u
#define STUB_OFFSET 0x0048u
#define ENTRY8_IP 0x0DE3u

_Static_assert(sizeof(pr_u7f_0fd2_raw_cpu) == sizeof(pr_u9e_entry8_compose_cpu), "U9E +0DE3 CPU layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void push16(uint8_t *ram, pr_u7f_0fd2_raw_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* SUB/CMP left,right in `bits` bits: the six arithmetic flags; the other flags are kept. */
static void sub_flags(pr_u7f_0fd2_raw_cpu *cpu, uint16_t left, uint16_t right, unsigned bits)
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

static void set_al(pr_u7f_0fd2_raw_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

/* CMP AL,imm: returns AL - imm's sign as below (-1), equal (0) or above (1), unsigned. */
static int cmp_al(pr_u7f_0fd2_raw_cpu *cpu, uint8_t imm)
{
    uint8_t al = (uint8_t)cpu->ax;
    sub_flags(cpu, al, imm, 8u);
    return al < imm ? -1 : al > imm;
}

pr_u7f_0fd2_raw_status pr_u7f_roll_class_0fd2_cpu(uint8_t *ram, size_t ram_size, pr_u7f_0fd2_raw_cpu *cpu, unsigned max_system_calls,
                                                  int *callee_status)
{
    uint16_t entry, caller_cs, at, return_ip, stub_segment;
    pr_u9e_entry8_compose_cpu chain;
    pr_u9e_entry8_compose_status status;
    uint8_t roll, cls;
    size_t frame;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U7F_0FD2_RAM_SIZE)
        return PR_U7F_0FD2_RAW_ARGUMENT;
    entry = cpu->ip;
    push16(ram, cpu, cpu->bp);                                            /* +0FD2 PUSH BP; MOV BP,SP; SUB SP,2 */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 2u, 16u);
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    set_al(cpu, 1u);                                                      /* +0FD8 MOV AL,1; PUSH AX; MOV AL,14h; PUSH AX */
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x14u);
    push16(ram, cpu, cpu->ax);
    at = (uint16_t)(entry + (uint16_t)(0x0FDEu - ENTRY_IP));              /* +0FDE LCALL 00B0:0048 (the loaded stub) */
    return_ip = (uint16_t)(at + 5u);
    caller_cs = cpu->cs;
    cpu->ip = at;
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != STUB_OFFSET)
        return PR_U7F_0FD2_RAW_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(at + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu || read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U7F_0FD2_RAW_CODE;
    push16(ram, cpu, caller_cs);
    push16(ram, cpu, return_ip);
    cpu->ip = ENTRY8_IP;
    cpu->cs = read16(ram, stub_segment, STUB_OFFSET + 3u);
    memcpy(&chain, cpu, sizeof chain);
    status = pr_u9e_entry8_fullstate_compose_0de3(ram, ram_size, &chain, max_system_calls);
    memcpy(cpu, &chain, sizeof chain);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (status != PR_U9E_ENTRY8_COMPOSE_RETURNED)
        return PR_U7F_0FD2_RAW_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U7F_0FD2_RAW_TRANSFER;
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 2u))] = (uint8_t)cpu->ax;  /* +0FE3 MOV [BP-2],AL; MOV AL,[BP-2] */
    roll = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    set_al(cpu, roll);
    if (cmp_al(cpu, 1u) >= 0 && cmp_al(cpu, 0x0Eu) <= 0)                 /* +0FE9 CMP AL,1; JB; CMP AL,0Eh; JA */
        cls = 1u;                                                         /* +0FF1 */
    else if (cmp_al(cpu, 0x0Fu) >= 0 && cmp_al(cpu, 0x13u) <= 0)         /* +0FF7 CMP AL,0Fh; JB; CMP AL,13h; JA */
        cls = 2u;                                                         /* +0FFF */
    else
        cls = 3u;                                                         /* +1005 */
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = cls;               /* MOV BYTE [BP-1],class */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));           /* +1009 MOV AL,[BP-1] */
    cpu->sp = cpu->bp;                                                    /* +100C MOV SP,BP; POP BP; RETF */
    frame = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, frame);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    frame = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, frame);
    cpu->cs = read16_at(ram, (frame + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U7F_0FD2_RAW_OK;
}
