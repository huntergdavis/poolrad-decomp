/* GAME.OVR U9E +1081..+10E3, complete local body. */
#include "poolrad/recovered/overlay_u9e_entry_1081.h"

#define MASK (PR_U9E_1081_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define LOGIC_FLAGS 0x08C5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u9e_1081_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u ? 0x0004u : 0u;
}

static void sub_sp(pr_u9e_1081_cpu *cpu, uint16_t count)
{
    uint16_t left = cpu->sp;
    uint16_t result = (uint16_t)(left - count);
    uint16_t flags = parity((uint8_t)result);
    if (left < count) flags |= 0x0001u;
    if (((left ^ count ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (((left ^ count) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | flags);
    cpu->sp = result;
}

static void or_al_al(pr_u9e_1081_cpu *cpu)
{
    uint8_t al = (uint8_t)cpu->ax;
    uint16_t flags = parity(al);
    if (al == 0u) flags |= 0x0040u;
    if ((al & 0x80u) != 0u) flags |= 0x0080u;
    /* AF is undefined after OR; source convention retains its input bit. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~LOGIC_FLAGS) | flags);
}

static pr_u9e_1081_status call_at(
    uint8_t *ram, size_t ram_size, pr_u9e_1081_cpu *cpu,
    const pr_u9e_1081_io *io, pr_u9e_1081_call_kind kind,
    uint16_t target_cs, uint16_t target_ip, uint16_t return_ip,
    int near_call, pr_u9e_1081_call_kind *stopped_at)
{
    uint16_t source_cs = cpu->cs;
    if (!near_call) push16(ram, cpu, source_cs);
    push16(ram, cpu, return_ip);
    cpu->cs = target_cs;
    cpu->ip = target_ip;
    if (stopped_at != NULL) *stopped_at = kind;
    if (io->call == NULL) return PR_U9E_1081_CALLEE_REQUIRED;
    if (!io->call(io->context, kind, ram, ram_size, cpu))
        return PR_U9E_1081_CALLEE_FAILED;
    if (cpu->cs != source_cs || cpu->ip != return_ip)
        return PR_U9E_1081_TRANSFER;
    return PR_U9E_1081_RETURNED;
}

pr_u9e_1081_status pr_u9e_entry_1081(
    uint8_t *ram, size_t ram_size, pr_u9e_1081_cpu *cpu,
    const pr_u9e_1081_io *io, pr_u9e_1081_call_kind *stopped_at)
{
    pr_u9e_1081_status status;
    uint16_t source_cs;
    if (ram == NULL || cpu == NULL || io == NULL ||
        ram_size != PR_U9E_1081_RAM_SIZE || cpu->ip != 0x1081u)
        return PR_U9E_1081_ARGUMENT;
    source_cs = cpu->cs;

    /* +1081..+109A: prologue, local flag, three inputs and far output. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub_sp(cpu, 14u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 10u)));
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    push16(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 5u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    status = call_at(ram, ram_size, cpu, io, PR_U9E_1081_CALL_LOOKUP,
                     io->loaded_sidecar_cs, 0x2199u, 0x109Fu, 0,
                     stopped_at);
    if (status != PR_U9E_1081_RETURNED) return status;

    /* +109F OR AL,AL; JE +10DB. */
    or_al_al(cpu);
    if ((uint8_t)cpu->ax != 0u) {
        /* +10A3..+10B3: four arguments and CS:1078 source pointer. */
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 10u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
        cpu->di = (uint16_t)(cpu->bp - 14u);
        push16(ram, cpu, cpu->ss);
        push16(ram, cpu, cpu->di);
        cpu->di = 0x1078u;
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, cpu->di);
        status = call_at(ram, ram_size, cpu, io, PR_U9E_1081_CALL_COPY,
                         io->loaded_system_cs, 0x0B0Bu, 0x10B8u, 0,
                         stopped_at);
        if (status != PR_U9E_1081_RETURNED) return status;

        /* +10B8..+10C3: two byte-valued WORD pushes, preserving AH. */
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x000Au);
        push16(ram, cpu, cpu->ax);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x0001u);
        push16(ram, cpu, cpu->ax);
        status = call_at(ram, ram_size, cpu, io, PR_U9E_1081_CALL_EMIT,
                         io->loaded_sidecar_cs, 0x16ECu, 0x10C3u, 0,
                         stopped_at);
        if (status != PR_U9E_1081_RETURNED) return status;

        /* +10C3..+10D7: local +0026 call with an explicit pushed CS. */
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 10u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                             read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
        push16(ram, cpu, cpu->ax);
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u)));
        push16(ram, cpu, cpu->cs);
        status = call_at(ram, ram_size, cpu, io,
                         PR_U9E_1081_CALL_LOCAL_0026,
                         source_cs, 0x0026u, 0x10D7u, 1, stopped_at);
        if (status != PR_U9E_1081_RETURNED) return status;
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);
    }

    /* +10DB..+10E3: live local byte, POP BP, RETF 6. */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->cs = read16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U9E_1081_RETURNED;
}
