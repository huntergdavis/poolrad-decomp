/* GAME.OVR unit 0x33:+0810..+08A3; docs/re/U33_NEIGHBOURHOOD_0810.md. */
#include "poolrad/recovered/overlay_u33_neighbourhood_0810.h"
#include "poolrad/recovered/overlay_u33_cell_pair_01ad.h"
#include "poolrad/recovered/overlay_u33_grid_pattern_040f.h"
#include "poolrad/recovered/overlay_u33_grid_pattern_0615.h"
#include "poolrad/recovered/overlay_u33_grid_roll_0000.h"
#include "poolrad/recovered/overlay_u33_stamp_block_02ba.h"
#include "poolrad/recovered/overlay_u33_stamp_square_0377.h"

#include <string.h>

#define MASK (PR_U33_0810_RAM_SIZE - 1u)

_Static_assert(sizeof(pr_u33_0810_cpu) == sizeof(pr_u33_01ad_cpu), "u33 +01AD CPU layout differs");
_Static_assert(sizeof(pr_u33_0810_cpu) == sizeof(pr_u33_040f_cpu), "u33 +040F CPU layout differs");
_Static_assert(sizeof(pr_u33_0810_cpu) == sizeof(pr_u33_0615_cpu), "u33 +0615 CPU layout differs");
_Static_assert(sizeof(pr_u33_0810_cpu) == sizeof(pr_u33_0000_cpu), "u33 +0000 CPU layout differs");
_Static_assert(sizeof(pr_u33_0810_cpu) == sizeof(pr_u33_02ba_cpu), "u33 +02BA CPU layout differs");
_Static_assert(sizeof(pr_u33_0810_cpu) == sizeof(pr_u33_0377_cpu), "u33 +0377 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u33_0810_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void set_al(pr_u33_0810_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u33_0810_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t local(const uint8_t *ram, const pr_u33_0810_cpu *cpu, uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static void set_local(uint8_t *ram, const pr_u33_0810_cpu *cpu, uint16_t back, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - back), value);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u33_0810_cpu *cpu, uint8_t left, uint8_t right)
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

enum callee { C01AD, C02BA, C0377, C040F, C0615, C0000 };

/* PUSH CS; CALL near: push CS and the return offset, enter the model at its offset, run it on the
 * same RAM and CPU words and require its RETF to come back to the frame. */
static pr_u33_0810_status near_call(uint8_t *ram, size_t ram_size, pr_u33_0810_cpu *cpu, uint16_t entry,
                                    uint16_t site, enum callee which, pr_u33_0810_callee *callee)
{
    static const uint16_t back[] = {0x0663u, 0x0556u, 0x0499u, 0u, 0u, 0x0810u};
    uint16_t caller_cs = cpu->cs, return_ip = (uint16_t)(entry + site + 3u);
    pr_u33_0810_status mapped = PR_U33_0810_OK;
    int status = 0;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = which == C040F ? 0x040Fu : which == C0615 ? 0x0615u : (uint16_t)(entry - back[which]);
    switch (which) {
    case C01AD: { pr_u33_01ad_cpu c; memcpy(&c, cpu, sizeof c); status = (int)pr_u33_cell_pair_01ad(ram, ram_size, &c);
                  memcpy(cpu, &c, sizeof c);
                  mapped = status == PR_U33_01AD_OK ? PR_U33_0810_OK : status == PR_U33_01AD_CODE ? PR_U33_0810_CODE
                         : status == PR_U33_01AD_TRANSFER ? PR_U33_0810_TRANSFER : PR_U33_0810_CALLEE; break; }
    case C02BA: { pr_u33_02ba_cpu c; memcpy(&c, cpu, sizeof c); status = (int)pr_u33_stamp_block_02ba(ram, ram_size, &c);
                  memcpy(cpu, &c, sizeof c);
                  mapped = status == PR_U33_02BA_OK ? PR_U33_0810_OK : status == PR_U33_02BA_TRANSFER ? PR_U33_0810_TRANSFER
                         : PR_U33_0810_CALLEE; break; }
    case C0377: { pr_u33_0377_cpu c; memcpy(&c, cpu, sizeof c); status = (int)pr_u33_stamp_square_0377(ram, ram_size, &c);
                  memcpy(cpu, &c, sizeof c);
                  mapped = status == PR_U33_0377_OK ? PR_U33_0810_OK : status == PR_U33_0377_TRANSFER ? PR_U33_0810_TRANSFER
                         : PR_U33_0810_CALLEE; break; }
    case C040F: { pr_u33_040f_cpu c; memcpy(&c, cpu, sizeof c); status = (int)pr_u33_grid_pattern_040f(ram, ram_size, &c);
                  memcpy(cpu, &c, sizeof c);
                  mapped = status == PR_U33_040F_OK ? PR_U33_0810_OK : status == PR_U33_040F_TRANSFER ? PR_U33_0810_TRANSFER
                         : PR_U33_0810_CALLEE; break; }
    case C0615: { pr_u33_0615_cpu c; memcpy(&c, cpu, sizeof c); status = (int)pr_u33_grid_pattern_0615(ram, ram_size, &c);
                  memcpy(cpu, &c, sizeof c);
                  mapped = status == PR_U33_0615_OK ? PR_U33_0810_OK : status == PR_U33_0615_TRANSFER ? PR_U33_0810_TRANSFER
                         : PR_U33_0810_CALLEE; break; }
    case C0000: { pr_u33_0000_cpu c; int chain = -1; memcpy(&c, cpu, sizeof c);
                  status = (int)pr_u33_grid_roll_0000(ram, ram_size, &c, &chain);
                  memcpy(cpu, &c, sizeof c);
                  mapped = status == PR_U33_0000_OK ? PR_U33_0810_OK : status == PR_U33_0000_CODE ? PR_U33_0810_CODE
                         : status == PR_U33_0000_TRANSFER ? PR_U33_0810_TRANSFER : PR_U33_0810_CALLEE;
                  if (status == PR_U33_0000_CALLEE) status = chain;
                  break; }
    }
    if (mapped != PR_U33_0810_OK) {
        if (callee != NULL) { callee->site = site; callee->status = status; }
        return mapped;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0810_TRANSFER;
    return PR_U33_0810_OK;
}

#define TRY(call) do { pr_u33_0810_status status_ = (call); if (status_ != PR_U33_0810_OK) return status_; } while (0)

/* MOV AL,[BP-back]; PUSH AX (the live AH travels with the pushed word) */
static void push_local(uint8_t *ram, pr_u33_0810_cpu *cpu, uint16_t back)
{
    set_al(cpu, local(ram, cpu, back));
    push16(ram, cpu, cpu->ax);
}

pr_u33_0810_status pr_u33_neighbourhood_0810(
    uint8_t *ram, size_t ram_size, pr_u33_0810_cpu *cpu, pr_u33_0810_callee *callee)
{
    static const struct { uint16_t site; uint8_t facing; uint16_t store; } probes[3] = {
        {0x42u, 6u, 8u}, {0x54u, 0u, 7u}, {0x66u, 2u, 9u}};
    uint16_t entry;
    uint8_t value;
    size_t at;
    unsigned i;
    if (callee != NULL) { callee->site = 0u; callee->status = 0; }
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_0810_RAM_SIZE)
        return PR_U33_0810_ARGUMENT;
    entry = cpu->ip;
    /* +0810..+0813: frame and ten local bytes (SUB's flags are overwritten by +01AD). */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x0Au);
    set_local(ram, cpu, 2u, 0xFEu);                                  /* dy := -2 */
    for (;;) {
        set_local(ram, cpu, 1u, 0xFAu);                              /* dx := -6 */
        for (;;) {
            /* +0828..+0843: x := sign(dx) + sign(party x), y := sign(dy) + sign(party y). */
            set_al(cpu, local(ram, cpu, 1u));
            cbw(cpu);
            cpu->dx = cpu->ax;
            set_al(cpu, read8(ram, cpu->ds, 0x6AADu));
            cbw(cpu);
            cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
            set_local(ram, cpu, 3u, (uint8_t)cpu->ax);
            set_al(cpu, local(ram, cpu, 2u));
            cbw(cpu);
            cpu->dx = cpu->ax;
            set_al(cpu, read8(ram, cpu->ds, 0x6AAEu));
            cbw(cpu);
            cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
            set_local(ram, cpu, 4u, (uint8_t)cpu->ax);
            /* +0846..+0879: +01AD(x, y, facing) for facings 6, 0 and 2. */
            for (i = 0; i < 3u; ++i) {
                push_local(ram, cpu, 3u);
                push_local(ram, cpu, 4u);
                set_al(cpu, probes[i].facing);
                push16(ram, cpu, cpu->ax);
                TRY(near_call(ram, ram_size, cpu, entry, probes[i].site, C01AD, callee));
                set_local(ram, cpu, probes[i].store, (uint8_t)cpu->ax);
            }
            /* +087C..+088D: the four pattern bodies with this frame as their static link. */
            push16(ram, cpu, cpu->bp);
            TRY(near_call(ram, ram_size, cpu, entry, 0x6Eu, C02BA, callee));
            push16(ram, cpu, cpu->bp);
            TRY(near_call(ram, ram_size, cpu, entry, 0x73u, C0377, callee));
            push16(ram, cpu, cpu->bp);
            TRY(near_call(ram, ram_size, cpu, entry, 0x78u, C040F, callee));
            push16(ram, cpu, cpu->bp);
            TRY(near_call(ram, ram_size, cpu, entry, 0x7Du, C0615, callee));
            /* +0890..+0894: dx = 6 ends the row. */
            value = local(ram, cpu, 1u);
            cmp8(cpu, value, 6u);
            if (value == 6u)
                break;
            set_local(ram, cpu, 1u, (uint8_t)(local(ram, cpu, 1u) + 1u));   /* +0825 */
        }
        /* +0896..+089A: dy = 2 ends the sweep. */
        value = local(ram, cpu, 2u);
        cmp8(cpu, value, 2u);
        if (value == 2u)
            break;
        set_local(ram, cpu, 2u, (uint8_t)(local(ram, cpu, 2u) + 1u));       /* +081C */
    }
    /* +089C..+089D: the grid roll. */
    TRY(near_call(ram, ram_size, cpu, entry, 0x8Du, C0000, callee));
    /* +08A0..+08A3: MOV SP,BP; POP BP; RETF. */
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U33_0810_OK;
}
