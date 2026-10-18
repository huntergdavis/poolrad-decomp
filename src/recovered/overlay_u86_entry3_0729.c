/* GAME.OVR unit 0x86:+0729..+07D5 (public entry 3), raw CPU; docs/re/RES_26C2_U86_0729.md. */
#include "poolrad/recovered/overlay_u86_entry3_0729.h"
#include "poolrad/recovered/resident_00ba_class_value_26c2_cpu.h"

#include <string.h>

#define MASK (PR_U86_0729_RAM_SIZE - 1u)
#define CALL_SITE 0x11u          /* +073A - +0729 */
#define LOOKUP_ENTRY 0x26C2u

_Static_assert(sizeof(pr_u86_0729_cpu) == sizeof(pr_res_26c2_cpu), "RES_00BA_26C2 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u86_0729_cpu *cpu, uint16_t value)
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

static void cmp8(pr_u86_0729_cpu *cpu, uint8_t left, uint8_t right)
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

/* MOV AL,[BP+6]; XOR AH,AH; MOV DI,AX; MOV CL,4; SHL DI,CL (flags folded): AX = k, DI = 16k, CL = 4. */
static void index16(const uint8_t *ram, pr_u86_0729_cpu *cpu)
{
    cpu->ax = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = (uint16_t)(cpu->ax << 4);
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
}

/* index16, then MOV AL,[DI+table]; XOR AH,AH (flags folded). */
static uint8_t table_byte(const uint8_t *ram, pr_u86_0729_cpu *cpu, uint16_t table)
{
    index16(ram, cpu);
    cpu->ax = read8(ram, cpu->ds, (uint16_t)(cpu->di + table));
    return (uint8_t)cpu->ax;
}

/* +0736..+073F: MOV AL,[BP+6]; PUSH AX; LCALL xxxx:26C2 (read from RAM). */
static pr_u86_0729_status lookup(uint8_t *ram, size_t ram_size, pr_u86_0729_cpu *cpu, uint16_t entry)
{
    uint16_t caller_cs = cpu->cs, at = (uint16_t)(entry + CALL_SITE), return_ip = (uint16_t)(at + 5u);
    pr_res_26c2_cpu leaf;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    push16(ram, cpu, cpu->ax);
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != LOOKUP_ENTRY)
        return PR_U86_0729_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->cs = read16(ram, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = LOOKUP_ENTRY;
    memcpy(&leaf, cpu, sizeof leaf);
    (void)pr_res_00ba_26c2_cpu(ram, ram_size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U86_0729_TRANSFER;
    return PR_U86_0729_OK;
}

pr_u86_0729_status pr_u86_entry3_0729(uint8_t *ram, size_t ram_size, pr_u86_0729_cpu *cpu)
{
    uint16_t entry;
    uint8_t value;
    size_t at;
    pr_u86_0729_status status;
    if (ram == NULL || cpu == NULL || ram_size != PR_U86_0729_RAM_SIZE)
        return PR_U86_0729_ARGUMENT;
    entry = cpu->ip;
    /* +0729..+072C: PUSH BP; MOV BP,SP; SUB SP,2 (flags overwritten by the CMP). */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    value = read8(ram, cpu->ds, 0x6DBFu);                                 /* +072F CMP BYTE [6DBF],0; JNE +0770 */
    cmp8(cpu, value, 0u);
    if (value == 0u) {
        uint32_t product;
        status = lookup(ram, ram_size, cpu, entry);
        if (status != PR_U86_0729_OK)
            return status;
        cpu->ax &= 0x00FFu;                                               /* +073F XOR AH,AH; MOV DX,AX */
        cpu->dx = cpu->ax;
        (void)table_byte(ram, cpu, 0x31FDu);                              /* +0743..+0752 */
        product = (uint32_t)cpu->ax * cpu->dx;                            /* +0754 MUL DX */
        cpu->ax = (uint16_t)product;
        cpu->dx = (uint16_t)(product >> 16);
        cpu->dx = cpu->ax;                                                /* +0756 MOV DX,AX */
        (void)table_byte(ram, cpu, 0x31FCu);                              /* +0758..+0767 */
        cpu->ax = (uint16_t)(cpu->ax + cpu->dx);                          /* +0769 ADD AX,DX */
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
    } else {
        cpu->dx = table_byte(ram, cpu, 0x31FCu);                          /* +0770..+0781 */
        (void)table_byte(ram, cpu, 0x31FDu);                              /* +0783..+0792 */
        cpu->ax = (uint16_t)(cpu->ax << 1);                               /* +0794 SHL AX,1; MOV SI,AX */
        cpu->si = cpu->ax;
        cpu->ax = (uint16_t)(cpu->ax << 1);                               /* +0798 SHL AX,1; ADD AX,SI; ADD AX,DX */
        cpu->ax = (uint16_t)(cpu->ax + cpu->si);
        cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
    }
    value = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));                /* +07A1 CMP BYTE [BP-2],0; JNE +07BD */
    cmp8(cpu, value, 0u);
    if (value == 0u) {
        index16(ram, cpu);                                                /* +07A7..+07B0 */
        value = read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x3200u));       /* +07B2 CMP BYTE [DI+3200],0; JE +07BD */
        cmp8(cpu, value, 0u);
        if (value != 0u)
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);           /* +07B9 */
    }
    value = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));                /* +07BD CMP BYTE [BP-2],FFh; JNE +07C7 */
    cmp8(cpu, value, 0xFFu);
    if (value == 0xFFu)
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);
    /* +07C7..+07D3: MOV AL,[BP-2]; MOV [BP-1],AL; MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 2. */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_U86_0729_OK;
}
