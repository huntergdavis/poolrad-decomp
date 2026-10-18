/* GAME.OVR unit 0x3B0 +08D7 and +0923 (raw CPU); docs/re/U3B0_SPACE_SCAN_CPU.md. */
#include "poolrad/recovered/overlay_u3b0_space_scan_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#include <string.h>

#define MASK (PR_U3B0S_RAM_SIZE - 1u)
#define STORE_ENTRY 0x0B25u

_Static_assert(sizeof(pr_u3b0s_cpu) == sizeof(pr_system_0b25_cpu), "SYSTEM 0B25 CPU layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void push16(uint8_t *ram, pr_u3b0s_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u3b0s_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

/* SUB SP,2Ch: live only when the CODE stop leaves them; the string store's XOR CH,CH overwrites them otherwise. */
static void sub16(pr_u3b0s_cpu *cpu, uint16_t right)
{
    uint16_t left = cpu->sp, result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    cpu->sp = result;
}

static void set_al(pr_u3b0s_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint8_t frame8(const uint8_t *ram, const pr_u3b0s_cpu *cpu, unsigned delta)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - delta));
}

/* PUSH BP; MOV BP,SP; SUB SP,2Ch; copy S into SS:BP-2Ah through the string store at `site`. */
static pr_u3b0s_status enter_and_copy(uint8_t *ram, size_t ram_size, pr_u3b0s_cpu *cpu, uint16_t entry, uint16_t base,
                                      uint16_t site, int *callee_status)
{
    uint16_t caller_cs, at = (uint16_t)(entry + (uint16_t)(site - base)), return_ip = (uint16_t)(at + 5u);
    size_t frame;
    pr_system_0b25_cpu leaf;
    pr_system_0b25_status status;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub16(cpu, 0x2Cu);
    frame = physical(cpu->ss, (uint16_t)(cpu->bp + 8u));                  /* LES DI,[BP+8]; PUSH ES; PUSH DI */
    cpu->di = read16_at(ram, frame);
    cpu->es = read16_at(ram, (frame + 2u) & MASK);
    push16(ram, cpu, cpu->es);
    push16(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x2Au);                                 /* LEA DI,[BP-2Ah]; PUSH SS; PUSH DI */
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->ax = 0x28u;                                                       /* MOV AX,28h; PUSH AX */
    push16(ram, cpu, cpu->ax);
    cpu->ip = at;                                                          /* the LCALL is reached */
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != STORE_ENTRY)
        return PR_U3B0S_CODE;
    caller_cs = cpu->cs;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->cs = read16(ram, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = STORE_ENTRY;
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_system_strstore_0b25(ram, ram_size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U3B0S_TRANSFER;
    return PR_U3B0S_OK;
}

/* MOV AL,[BP-2Bh]; MOV [BP-1],AL; MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 6. */
static pr_u3b0s_status leave(uint8_t *ram, pr_u3b0s_cpu *cpu)
{
    size_t at;
    set_al(cpu, frame8(ram, cpu, 0x2Bu));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    set_al(cpu, frame8(ram, cpu, 1u));
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 6u);
    return PR_U3B0S_OK;
}

/* MOV AL,[BP-2Bh]; XOR AH,AH; MOV DI,AX; CMP BYTE [BP+DI-2Ah],20h. Returns whether the byte is a space. */
static int is_space(const uint8_t *ram, pr_u3b0s_cpu *cpu)
{
    uint8_t value;
    cpu->ax = frame8(ram, cpu, 0x2Bu);
    cpu->di = cpu->ax;
    value = read8(ram, cpu->ss, (uint16_t)(cpu->bp + cpu->di - 0x2Au));
    cmp8(cpu, value, 0x20u);
    return value == 0x20u;
}

pr_u3b0s_status pr_u3b0_lead_spaces_08d7_cpu(uint8_t *ram, size_t ram_size, pr_u3b0s_cpu *cpu, int *callee_status)
{
    pr_u3b0s_status status;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U3B0S_RAM_SIZE)
        return PR_U3B0S_ARGUMENT;
    status = enter_and_copy(ram, ram_size, cpu, cpu->ip, 0x08D7u, 0x08EBu, callee_status);
    if (status != PR_U3B0S_OK)
        return status;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2Bu), 1u);                 /* +08F0 MOV BYTE [BP-2Bh],1 */
    set_al(cpu, frame8(ram, cpu, 0x2Au));                                  /* +08F4 MOV AL,[BP-2Ah]; MOV [BP-2Ch],AL */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2Cu), (uint8_t)cpu->ax);
    for (;;) {
        uint8_t limit;
        set_al(cpu, frame8(ram, cpu, 0x2Bu));                              /* +08FA MOV AL,[BP-2Bh]; CMP AL,[BP-2Ch]; JAE */
        limit = frame8(ram, cpu, 0x2Cu);
        cmp8(cpu, (uint8_t)cpu->ax, limit);
        if ((uint8_t)cpu->ax >= limit)
            break;
        if (!is_space(ram, cpu))                                           /* +0902..+090D JNE +0914 */
            break;
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2Bu),                  /* +090F INC BYTE [BP-2Bh] (flags folded) */
               (uint8_t)(frame8(ram, cpu, 0x2Bu) + 1u));
    }
    return leave(ram, cpu);
}

pr_u3b0s_status pr_u3b0_trail_spaces_0923_cpu(uint8_t *ram, size_t ram_size, pr_u3b0s_cpu *cpu, int *callee_status)
{
    pr_u3b0s_status status;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U3B0S_RAM_SIZE)
        return PR_U3B0S_ARGUMENT;
    status = enter_and_copy(ram, ram_size, cpu, cpu->ip, 0x0923u, 0x0937u, callee_status);
    if (status != PR_U3B0S_OK)
        return status;
    set_al(cpu, frame8(ram, cpu, 0x2Au));                                  /* +093C MOV AL,[BP-2Ah]; MOV [BP-2Bh],AL */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2Bu), (uint8_t)cpu->ax);
    for (;;) {
        uint8_t value = frame8(ram, cpu, 0x2Bu);                           /* +0942 CMP BYTE [BP-2Bh],1; JBE */
        cmp8(cpu, value, 1u);
        if (value <= 1u)
            break;
        if (!is_space(ram, cpu))                                           /* +0948..+0953 JNE +095A */
            break;
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2Bu),                  /* +0955 DEC BYTE [BP-2Bh] (flags folded) */
               (uint8_t)(frame8(ram, cpu, 0x2Bu) - 1u));
    }
    return leave(ram, cpu);
}
