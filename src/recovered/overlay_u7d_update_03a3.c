/* GAME.OVR unit 0x7D:+03A3..+03D4; docs/re/U7D_UPDATE_03A3.md. */
#include "poolrad/recovered/overlay_u7d_update_03a3.h"

#define MASK (PR_U7D_03A3_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at & MASK] |
                      ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return word_at(ram, physical(segment, offset));
}

static void put(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push(uint8_t *ram, pr_u7d_03a3_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put(ram, cpu->ss, cpu->sp, value);
}

static uint16_t cmp_flags(uint16_t incoming, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    uint16_t flags = (uint16_t)(incoming & (uint16_t)~0x08D5u);
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (left < right) flags |= 0x0001u;
    if (!(parity & 1u)) flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x0010u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x8000u) flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x8000u) flags |= 0x0800u;
    return flags;
}

pr_u7d_03a3_status pr_u7d_update_03a3(
    uint8_t *ram, size_t ram_size, pr_u7d_03a3_cpu *cpu)
{
    uint16_t source_cs, value;
    uint32_t product;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_U7D_03A3_RAM_SIZE)
        return PR_U7D_03A3_ARGUMENT;

    /* +03A3..+03AC: prologue, far array argument, PUSH CS; CALL +02F2. */
    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->di = 0x6DC2u;
    push(ram, cpu, cpu->ds);
    push(ram, cpu, cpu->di);
    source_cs = cpu->cs;
    push(ram, cpu, source_cs);
    push(ram, cpu, 0x03AFu);
    cpu->ip = 0x02F2u;
    if (pr_u7d_carry_02f2(ram, ram_size, cpu) != PR_U7D_02F2_OK)
        return PR_U7D_03A3_CALLEE_FAILED;
    if (cpu->cs != source_cs || cpu->ip != 0x03AFu)
        return PR_U7D_03A3_CALLEE_TRANSFER;

    /* +03AF CMP [6DCC],0; JBE +03D3. Every source read remains live. */
    value = word(ram, cpu->ds, 0x6DCCu);
    cpu->flags = cmp_flags(cpu->flags, value, 0u);
    if (value != 0u) {
        /* +03B6..+03D1: multiply, add, clear, then cap at 99. */
        cpu->ax = word(ram, cpu->ds, 0x3642u);
        product = (uint32_t)cpu->ax * word(ram, cpu->ds, 0x6DCCu);
        cpu->ax = (uint16_t)product;
        cpu->dx = (uint16_t)(product >> 16);
        value = word(ram, cpu->ds, 0x6DCAu);
        put(ram, cpu->ds, 0x6DCAu, (uint16_t)(value + cpu->ax));
        cpu->ax = 0u;
        put(ram, cpu->ds, 0x6DCCu, cpu->ax);
        value = word(ram, cpu->ds, 0x6DCAu);
        cpu->flags = cmp_flags(cpu->flags, value, 0x0063u);
        if (value > 0x0063u) put(ram, cpu->ds, 0x6DCAu, 0x0063u);
    }

    /* +03D3 POP BP; +03D4 RETF, with live return-word aliases. */
    cpu->sp = cpu->bp;
    cpu->bp = word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U7D_03A3_OK;
}
