/* GAME.OVR unit 0x33:+01AD..+0220; docs/re/U33_CELL_PAIR_01AD.md. */
#include "poolrad/recovered/overlay_u33_cell_pair_01ad.h"
#include "poolrad/recovered/overlay_u33_cell_lookup_012b.h"

#include <string.h>

#define MASK (PR_U33_01AD_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define STEP_X 0x2736u
#define STEP_Y 0x273Fu
#define CALLEE_BACK 0x82u      /* +01AD - +012B */
#define FIRST_RETURN 0x29u     /* +01D6 - +01AD */
#define SECOND_RETURN 0x5Fu    /* +020C - +01AD */

_Static_assert(sizeof(pr_u33_01ad_cpu) == sizeof(pr_u33_012b_cpu),
               "u33 +012B CPU state layout differs");

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

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u33_01ad_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u33_01ad_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u33_01ad_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t arg(const uint8_t *ram, const pr_u33_01ad_cpu *cpu, uint16_t forward)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp + forward));
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* OR AL,[BP-3]: CF/OF clear, undefined AF represented as zero. */
static void or8(pr_u33_01ad_cpu *cpu, uint8_t result)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
}

/* PUSH CS; CALL +012B: the callee model runs at its own load offset. */
static pr_u33_01ad_status call_012b(uint8_t *ram, size_t ram_size, pr_u33_01ad_cpu *cpu,
                                    uint16_t entry, uint16_t return_delta)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + return_delta);
    pr_u33_012b_cpu leaf;
    pr_u33_012b_status status;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = (uint16_t)(entry - CALLEE_BACK);
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_u33_cell_lookup_012b(ram, ram_size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (status == PR_U33_012B_ARGUMENT)
        return PR_U33_01AD_ARGUMENT;
    if (status == PR_U33_012B_CODE)
        return PR_U33_01AD_CODE;
    if (status == PR_U33_012B_TRANSFER || cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_01AD_TRANSFER;
    return PR_U33_01AD_OK;
}

/* AL := facing (zero-extended) as DI; AL := DS:[table+DI]; sign-extend to DX;
 * AX := sign(argument) + DX. */
static void stepped(uint8_t *ram, pr_u33_01ad_cpu *cpu, uint16_t table, uint16_t forward)
{
    set_al(cpu, arg(ram, cpu, 0x06u));
    cpu->ax &= 0x00FFu;
    cpu->di = cpu->ax;
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + table)));
    cbw(cpu);
    cpu->dx = cpu->ax;
    set_al(cpu, arg(ram, cpu, forward));
    cbw(cpu);
    cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
}

pr_u33_01ad_status pr_u33_cell_pair_01ad(
    uint8_t *ram, size_t ram_size, pr_u33_01ad_cpu *cpu)
{
    uint16_t entry;
    pr_u33_01ad_status status;
    int32_t dividend;
    if (ram == NULL || cpu == NULL || ram_size < PR_U33_01AD_RAM_SIZE)
        return PR_U33_01AD_ARGUMENT;
    entry = cpu->ip;
    /* +01AD..+01B0 */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    /* +01B3..+01C2: (facing + 4) divided by 8, remainder into [BP-4]. The
     * XOR and ADD flags are overwritten inside the first call; IDIV leaves
     * the flags alone in both tested engines. */
    set_al(cpu, arg(ram, cpu, 0x06u));
    cpu->ax &= 0x00FFu;
    cpu->ax = (uint16_t)(cpu->ax + 4u);
    cpu->dx = (cpu->ax & 0x8000u) ? 0xFFFFu : 0u;
    cpu->cx = 8u;
    dividend = (int32_t)(((uint32_t)cpu->dx << 16) | cpu->ax);
    cpu->ax = (uint16_t)(dividend / 8);
    cpu->dx = (uint16_t)(dividend % 8);
    /* IDIV CX (this site previously assumed the flags were left alone). Pinned DIVW/IDIVW (instructions.h:655-673, 745-761) overwrite all six flags: AF=SF=OF=0,
     * ZF=(rem==0 && quo odd), CF=((rem&3) in 1..2), PF=even parity of the four bytes of rem:quo. */
    {
        uint16_t rem16 = (uint16_t)(dividend % 8), quo16 = (uint16_t)(dividend / 8), fold = (uint16_t)(rem16 ^ quo16);
        fold ^= (uint16_t)(fold >> 8); fold ^= (uint16_t)(fold >> 4); fold ^= (uint16_t)(fold >> 2); fold ^= (uint16_t)(fold >> 1);
        cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | ((rem16 == 0u && (quo16 & 1u)) ? 0x0040u : 0u) |
                                (((rem16 & 3u) >= 1u && (rem16 & 3u) <= 2u) ? 0x0001u : 0u) | ((fold & 1u) ? 0u : 0x0004u));
    }
    {
        uint16_t swap = cpu->ax;
        cpu->ax = cpu->dx;
        cpu->dx = swap;
    }
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), (uint8_t)cpu->ax);
    /* +01C5..+01D6: +012B on x, y, facing and this frame's BP. */
    set_al(cpu, arg(ram, cpu, 0x0Au));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, arg(ram, cpu, 0x08u));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, arg(ram, cpu, 0x06u));
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->bp);
    status = call_012b(ram, ram_size, cpu, entry, FIRST_RETURN);
    if (status != PR_U33_01AD_OK)
        return status;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
    /* +01D9..+020C: +012B on the stepped cell and the opposite facing. */
    stepped(ram, cpu, STEP_X, 0x0Au);
    push16(ram, cpu, cpu->ax);
    stepped(ram, cpu, STEP_Y, 0x08u);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 4u)));
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->bp);
    status = call_012b(ram, ram_size, cpu, entry, SECOND_RETURN);
    if (status != PR_U33_01AD_OK)
        return status;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), (uint8_t)cpu->ax);
    /* +020F..+021E: OR the two results through [BP-1]; MOV SP,BP; POP BP; RETF 6. */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    set_al(cpu, (uint8_t)(cpu->ax | read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u))));
    or8(cpu, (uint8_t)cpu->ax);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U33_01AD_OK;
}
