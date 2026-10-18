/* GAME.OVR unit 0x86:+07D6..+08C2 (raw CPU); docs/re/U86_ROLL_VALUE_07D6.md. */
#include "poolrad/recovered/overlay_u86_roll_value_07d6.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"
#include "poolrad/recovered/resident_00ba_class_value_26c2_cpu.h"

#include <string.h>

#define MASK (PR_U86_07D6_RAM_SIZE - 1u)
#define STUB_OFFSET 0x48u          /* unit 0x9E entry 8 */
#define ENTRY8_IP 0x0DE3u
#define LOOKUP_ENTRY 0x26C2u
#define F_CF 0x0001u
#define F_PF 0x0004u
#define F_AF 0x0010u
#define F_ZF 0x0040u
#define F_SF 0x0080u
#define F_OF 0x0800u

_Static_assert(sizeof(pr_u86_07d6_cpu) == sizeof(pr_u9e_entry8_compose_cpu), "U9E +0DE3 CPU layout differs");
_Static_assert(sizeof(pr_u86_07d6_cpu) == sizeof(pr_res_26c2_cpu), "RES_00BA_26C2 CPU layout differs");

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

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u86_07d6_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void put_arith(pr_u86_07d6_cpu *cpu, uint16_t flags)
{
    cpu->flags = (uint16_t)((cpu->flags & ~(F_CF | F_PF | F_AF | F_ZF | F_SF | F_OF)) | flags);
}

static void cmp8(pr_u86_07d6_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    uint16_t f = 0u;
    if (left < right) f |= F_CF;
    if (even_parity(result)) f |= F_PF;
    if ((left ^ right ^ result) & 0x10u) f |= F_AF;
    if (result == 0u) f |= F_ZF;
    if (result & 0x80u) f |= F_SF;
    if ((left ^ right) & (left ^ result) & 0x80u) f |= F_OF;
    put_arith(cpu, f);
}

/* ADD AX,src (16-bit). */
static void add_ax(pr_u86_07d6_cpu *cpu, uint16_t src)
{
    uint16_t left = cpu->ax, result = (uint16_t)(left + src);
    uint16_t f = 0u;
    if ((uint32_t)left + src > 0xFFFFu) f |= F_CF;
    if (even_parity((uint8_t)result)) f |= F_PF;
    if ((left ^ src ^ result) & 0x10u) f |= F_AF;
    if (result == 0u) f |= F_ZF;
    if (result & 0x8000u) f |= F_SF;
    if (~(left ^ src) & (left ^ result) & 0x8000u) f |= F_OF;
    put_arith(cpu, f);
    cpu->ax = result;
}

/* XOR AH,AH: the 8-bit result is zero (ZF, PF; CF = OF = SF = 0; AF clear as both oracles leave it). */
static void xor_ah(pr_u86_07d6_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
    put_arith(cpu, F_ZF | F_PF);
}

/* MUL r16, the pinned DOSBox-X normal-core profile shared with the merged U3C6 +0CB9 model. */
static void mul16(pr_u86_07d6_cpu *cpu, uint16_t src)
{
    uint32_t p = (uint32_t)cpu->ax * src;
    uint16_t f = (uint16_t)(cpu->flags & (F_SF | F_AF));
    cpu->ax = (uint16_t)p;
    cpu->dx = (uint16_t)(p >> 16);
    if (even_parity((uint8_t)(p ^ (p >> 8) ^ (p >> 16) ^ (p >> 24)))) f |= F_PF;
    if (cpu->ax == 0u) f |= F_ZF;
    if (cpu->dx != 0u) f |= F_CF | F_OF;
    put_arith(cpu, f);
}

static void note(int *callee_status, int status)
{
    if (callee_status != NULL)
        *callee_status = status;
}

/* MOV AL,n; PUSH AX; MOV AL,s; PUSH AX; LCALL through the loaded 00B0:0048 stub to U9E +0DE3; XOR AH,AH. */
static pr_u86_07d6_status roll(uint8_t *ram, size_t ram_size, pr_u86_07d6_cpu *cpu, uint16_t entry, uint16_t site,
                               uint8_t count, uint8_t sides, unsigned max_system_calls, int *callee_status)
{
    uint16_t caller_cs = cpu->cs, at = (uint16_t)(entry + (uint16_t)(site - 0x07D6u)), return_ip = (uint16_t)(at + 5u);
    uint16_t stub_segment;
    pr_u9e_entry8_compose_cpu chain;
    pr_u9e_entry8_compose_status status;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | count);
    push16(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | sides);
    push16(ram, cpu, cpu->ax);
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != STUB_OFFSET)
        return PR_U86_07D6_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(at + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu || read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U86_07D6_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = ENTRY8_IP;
    cpu->cs = read16(ram, stub_segment, STUB_OFFSET + 3u);
    memcpy(&chain, cpu, sizeof chain);
    status = pr_u9e_entry8_fullstate_compose_0de3(ram, ram_size, &chain, max_system_calls);
    memcpy(cpu, &chain, sizeof chain);
    note(callee_status, (int)status);
    if (status != PR_U9E_ENTRY8_COMPOSE_RETURNED)
        return PR_U86_07D6_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U86_07D6_TRANSFER;
    xor_ah(cpu);
    return PR_U86_07D6_OK;
}

/* MOV AL,[BP+6]; XOR AH,AH; MOV DI,AX; MOV CL,4; SHL DI,CL; MOV AL,[DI+table]; XOR AH,AH. Only the last XOR's
 * flags survive to the next arithmetic instruction, which overwrites them. */
static void table_byte(const uint8_t *ram, pr_u86_07d6_cpu *cpu, uint16_t table)
{
    cpu->ax = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = (uint16_t)(cpu->ax << 4);
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    cpu->ax = read8(ram, cpu->ds, (uint16_t)(cpu->di + table));
}

/* +087C..+0885: MOV AL,[BP+6]; PUSH AX; LCALL xxxx:26C2 (read from RAM). */
static pr_u86_07d6_status lookup(uint8_t *ram, size_t ram_size, pr_u86_07d6_cpu *cpu, uint16_t entry, int *callee_status)
{
    uint16_t caller_cs = cpu->cs, at = (uint16_t)(entry + (uint16_t)(0x0880u - 0x07D6u)), return_ip = (uint16_t)(at + 5u);
    pr_res_26c2_cpu leaf;
    pr_res_26c2_status status;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    push16(ram, cpu, cpu->ax);
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != LOOKUP_ENTRY)
        return PR_U86_07D6_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->cs = read16(ram, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = LOOKUP_ENTRY;
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_res_00ba_26c2_cpu(ram, ram_size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    note(callee_status, (int)status);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U86_07D6_TRANSFER;
    return PR_U86_07D6_OK;
}

pr_u86_07d6_status pr_u86_roll_value_07d6(uint8_t *ram, size_t ram_size, pr_u86_07d6_cpu *cpu,
                                          unsigned max_system_calls, int *callee_status)
{
    uint16_t entry;
    uint8_t k;
    size_t at;
    pr_u86_07d6_status status = PR_U86_07D6_OK;
    note(callee_status, -1);
    if (ram == NULL || cpu == NULL || ram_size != PR_U86_07D6_RAM_SIZE)
        return PR_U86_07D6_ARGUMENT;
    entry = cpu->ip;
    /* +07D6..+07DC: PUSH BP; MOV BP,SP; SUB SP,4 (flags overwritten by the first CMP); MOV AL,[BP+6]. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    k = (uint8_t)cpu->ax;
    cmp8(cpu, k, 0x28u);
    if (k == 0x28u) {                                                     /* +07E3: 10 * roll(1, 6) */
        status = roll(ram, ram_size, cpu, entry, 0x07E9u, 1u, 6u, max_system_calls, callee_status);
        if (status != PR_U86_07D6_OK) return status;
        cpu->dx = 0x0Au;
        mul16(cpu, cpu->dx);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
        goto tail;
    }
    cmp8(cpu, k, 0x39u);
    if (k != 0x39u)
        cmp8(cpu, k, 0x3Du);
    if (k == 0x39u || k == 0x3Du) {                                       /* +0803: roll(5, 4) */
        status = roll(ram, ram_size, cpu, entry, 0x0809u, 5u, 4u, max_system_calls, callee_status);
        if (status != PR_U86_07D6_OK) return status;
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
        goto tail;
    }
    cmp8(cpu, k, 0x3Bu);
    if (k == 0x3Bu) {                                                     /* +081A: 10 * roll(1, 4) + 28h */
        status = roll(ram, ram_size, cpu, entry, 0x0820u, 1u, 4u, max_system_calls, callee_status);
        if (status != PR_U86_07D6_OK) return status;
        cpu->dx = 0x0Au;
        mul16(cpu, cpu->dx);
        add_ax(cpu, 0x28u);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
        goto tail;
    }
    cmp8(cpu, k, 0x3Fu);
    if (k == 0x3Fu) {
        uint8_t mode = read8(ram, cpu->ds, 0x49F3u);                      /* +0839 CMP BYTE [49F3],5 */
        cmp8(cpu, mode, 5u);
        if (mode == 5u) {                                                 /* +0840: 10 * roll(2, 0Ah) */
            status = roll(ram, ram_size, cpu, entry, 0x0846u, 2u, 0x0Au, max_system_calls, callee_status);
            if (status != PR_U86_07D6_OK) return status;
            cpu->dx = 0x0Au;
            mul16(cpu, cpu->dx);
        } else {                                                          /* +0857: 10 * (roll(1, 0Ah) + 0Ah) */
            status = roll(ram, ram_size, cpu, entry, 0x085Du, 1u, 0x0Au, max_system_calls, callee_status);
            if (status != PR_U86_07D6_OK) return status;
            add_ax(cpu, 0x0Au);
            cpu->dx = 0x0Au;
            mul16(cpu, cpu->dx);
        }
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
        goto tail;
    }
    cmp8(cpu, k, 0x43u);
    if (k == 0x43u) {                                                     /* +0875 */
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), 0x05A0u);
        goto tail;
    }
    /* +087C..+08B1: T[5] * RES_00BA_26C2(k) + T[4]. */
    status = lookup(ram, ram_size, cpu, entry, callee_status);
    if (status != PR_U86_07D6_OK)
        return status;
    cpu->ax &= 0x00FFu;                                                   /* +0885 XOR AH,AH (flags overwritten by MUL) */
    cpu->dx = cpu->ax;
    table_byte(ram, cpu, 0x31FFu);
    mul16(cpu, cpu->dx);                                                  /* +089A MUL DX (flags overwritten by ADD) */
    cpu->dx = cpu->ax;
    table_byte(ram, cpu, 0x31FEu);
    add_ax(cpu, cpu->dx);                                                 /* +08AF ADD AX,DX */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
tail:
    /* +08B4..+08C0: MOV AX,[BP-4]; MOV [BP-2],AX; MOV AX,[BP-2]; MOV SP,BP; POP BP; RETF 2. */
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_U86_07D6_OK;
}
