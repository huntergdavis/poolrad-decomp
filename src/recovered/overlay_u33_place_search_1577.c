/* GAME.OVR unit 0x33:+1577..+19F4; docs/re/U33_PLACE_SEARCH_1577.md. */
#include "poolrad/recovered/overlay_u33_place_search_1577.h"
#include "poolrad/recovered/overlay_u33_both_outside_1403.h"
#include "poolrad/recovered/overlay_u33_cell_claim_1434.h"
#include "poolrad/recovered/overlay_u33_cell_pair_01ad.h"

#include <string.h>

#define MASK (PR_U33_1577_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define LOOP_HEAD 0x34u        /* +15AB - +1577 */
#define BACK_1403 0x0174u      /* +1577 - +1403 */
#define BACK_01AD 0x13CAu      /* +1577 - +01AD */
#define BACK_1434 0x0143u      /* +1577 - +1434 */

_Static_assert(sizeof(pr_u33_1577_cpu) == sizeof(pr_u33_1403_cpu), "u33 +1403 CPU layout differs");
_Static_assert(sizeof(pr_u33_1577_cpu) == sizeof(pr_u33_01ad_cpu), "u33 +01AD CPU layout differs");
_Static_assert(sizeof(pr_u33_1577_cpu) == sizeof(pr_u33_1434_cpu), "u33 +1434 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u33_1577_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u33_1577_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t local(const uint8_t *ram, const pr_u33_1577_cpu *cpu, uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static void set_local(uint8_t *ram, const pr_u33_1577_cpu *cpu, uint16_t back, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - back), value);
}

/* INC BYTE [BP-back]; its flags are always overwritten before they are observed. */
static void inc_local(uint8_t *ram, const pr_u33_1577_cpu *cpu, uint16_t back)
{
    set_local(ram, cpu, back, (uint8_t)(local(ram, cpu, back) + 1u));
}

static uint8_t data8(const uint8_t *ram, const pr_u33_1577_cpu *cpu, uint16_t offset)
{
    return read8(ram, cpu->ds, offset);
}

/* MUL r16; its flags are always overwritten before they are observed. */
static void mul16(pr_u33_1577_cpu *cpu, uint16_t source)
{
    uint32_t product = (uint32_t)cpu->ax * source;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    /* Pinned MULW: SF/AF kept, ZF from AX, PF is the even parity of all four bytes of DX:AX. */
    {
        uint32_t fold = product;
        fold ^= fold >> 16; fold ^= fold >> 8; fold ^= fold >> 4; fold ^= fold >> 2; fold ^= fold >> 1;
        cpu->flags = (uint16_t)((cpu->flags & ~0x0845u) | (cpu->dx != 0u ? 0x0801u : 0u) |
                                (cpu->ax == 0u ? 0x0040u : 0u) | ((fold & 1u) ? 0u : 0x0004u));
    }
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u33_1577_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

static void cmp16(pr_u33_1577_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x8000u) cpu->flags |= 0x0800u;
}

static void sub16_sp(pr_u33_1577_cpu *cpu, uint16_t right)
{
    uint16_t left = cpu->sp, result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    cpu->sp = result;
}

/* MOV AL,[4658]; CBW; MOV DI,AX; MOV AL,[DI+table] */
static void map_entry(const uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t table)
{
    set_al(cpu, data8(ram, cpu, 0x4658u));
    cbw(cpu);
    cpu->di = cpu->ax;
    set_al(cpu, data8(ram, cpu, (uint16_t)(cpu->di + table)));
}

/* DI := 4 * sign(DS:[4656 + sign(DS:4658)]) + DX */
static void row_index(const uint8_t *ram, pr_u33_1577_cpu *cpu)
{
    map_entry(ram, cpu, 0x4656u);
    cbw(cpu);
    cpu->di = cpu->ax;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->di = (uint16_t)(cpu->di + cpu->dx);
}

/* AL := ([BP-9] + add) mod 4 through CWD/IDIV CX=4 and XCHG DX,AX. */
static void quadrant(const uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t add)
{
    int32_t dividend;
    set_al(cpu, local(ram, cpu, 9u));
    cpu->ax &= 0x00FFu;
    cpu->ax = (uint16_t)(cpu->ax + add);
    cpu->dx = (cpu->ax & 0x8000u) ? 0xFFFFu : 0x0000u;
    cpu->cx = 4u;
    dividend = (int32_t)(((uint32_t)cpu->dx << 16) | cpu->ax);
    cpu->ax = (uint16_t)(int16_t)(dividend / 4);
    cpu->dx = (uint16_t)(int16_t)(dividend % 4);
    {
        uint16_t swap = cpu->ax;
        cpu->ax = cpu->dx;
        cpu->dx = swap;
    }
}

/* MOV DI,AX; MOV AL,[DI+2F2]; XOR AH,AH; MOV DI,AX; MOV AL,[DI+table]; MOV [BP-back],AL */
static void direction_step(uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t table, uint16_t back)
{
    cpu->di = cpu->ax;
    set_al(cpu, data8(ram, cpu, (uint16_t)(cpu->di + 0x02F2u)));
    cpu->ax &= 0x00FFu;
    cpu->di = cpu->ax;
    set_al(cpu, data8(ram, cpu, (uint16_t)(cpu->di + table)));
    set_local(ram, cpu, back, (uint8_t)cpu->ax);
}

/* Both step bytes for a quadrant offset: [BP-0B] from 2736, [BP-0C] from 273F. */
static void steps(uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t add)
{
    quadrant(ram, cpu, add);
    direction_step(ram, cpu, 0x2736u, 0x0Bu);
    quadrant(ram, cpu, add);
    direction_step(ram, cpu, 0x273Fu, 0x0Cu);
}

/* [BP-target] := [BP-base] + sign([BP-step]) * [BP-10] (states 2 and 3). */
static void offset_axis(uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t step, uint16_t base, uint16_t target)
{
    set_al(cpu, local(ram, cpu, 0x10u));
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    set_al(cpu, local(ram, cpu, step));
    cbw(cpu);
    mul16(cpu, cpu->dx);
    cpu->dx = cpu->ax;
    set_al(cpu, local(ram, cpu, base));
    cbw(cpu);
    cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
    set_local(ram, cpu, target, (uint8_t)cpu->ax);
}

/* State 1 base axis: [BP-target] := DS:[table + 4*(pass > 0) + [BP-9]] + sign([BP-step]) * [BP-0F]. */
static void base_axis(uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t step, uint16_t table, uint16_t target)
{
    uint8_t pass;
    set_al(cpu, local(ram, cpu, 0x0Fu));
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    set_al(cpu, local(ram, cpu, step));
    cbw(cpu);
    mul16(cpu, cpu->dx);
    cpu->cx = cpu->ax;
    set_al(cpu, local(ram, cpu, 9u));
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    pass = local(ram, cpu, 0x14u);
    cmp8(cpu, pass, 0u);
    set_al(cpu, 0u);
    if (pass != 0u)
        cpu->ax = (uint16_t)(cpu->ax + 1u);
    cbw(cpu);
    cpu->di = cpu->ax;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->di = (uint16_t)(cpu->di + cpu->dx);
    set_al(cpu, data8(ram, cpu, (uint16_t)(cpu->di + table)));
    cbw(cpu);
    cpu->ax = (uint16_t)(cpu->ax + cpu->cx);
    set_local(ram, cpu, target, (uint8_t)cpu->ax);
}

/* [BP-target] := sign(DS:party) + sign(DS:[table + sign(DS:4658)]) */
static void party_origin(uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t party, uint16_t table, uint16_t target)
{
    set_al(cpu, data8(ram, cpu, party));
    cbw(cpu);
    cpu->dx = cpu->ax;
    map_entry(ram, cpu, table);
    cbw(cpu);
    cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
    set_local(ram, cpu, target, (uint8_t)cpu->ax);
}

/* [BP-8] := DS:[2D2 + 4*sign(DS:[4656 + sign(DS:4658)]) + [BP-counter]] */
static void pass_direction(uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t counter)
{
    set_al(cpu, local(ram, cpu, counter));
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    row_index(ram, cpu);
    set_al(cpu, data8(ram, cpu, (uint16_t)(cpu->di + 0x02D2u)));
    set_local(ram, cpu, 8u, (uint8_t)cpu->ax);
}

/* [BP-target] := sign(DS:[table + [BP-8]]) + sign(DS:[origin + sign(DS:4658)]) */
static void reseed_axis(uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t table, uint16_t origin, uint16_t target)
{
    set_al(cpu, local(ram, cpu, 8u));
    cpu->ax &= 0x00FFu;
    cpu->di = cpu->ax;
    set_al(cpu, data8(ram, cpu, (uint16_t)(cpu->di + table)));
    cbw(cpu);
    cpu->dx = cpu->ax;
    map_entry(ram, cpu, origin);
    cbw(cpu);
    cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
    set_local(ram, cpu, target, (uint8_t)cpu->ax);
}

static void push_local(uint8_t *ram, pr_u33_1577_cpu *cpu, uint16_t back)
{
    set_al(cpu, local(ram, cpu, back));
    push16(ram, cpu, cpu->ax);
}

enum callee { CALLEE_1403, CALLEE_01AD, CALLEE_1434 };

/* PUSH CS; CALL near: push CS and the return offset, enter the callee model at its
 * offset relative to this entry, and require its RETF to come back to the frame. */
static pr_u33_1577_status near_call(uint8_t *ram, size_t ram_size, pr_u33_1577_cpu *cpu, uint16_t entry,
                                    uint16_t return_delta, enum callee callee, int *callee_status)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + return_delta);
    pr_u33_1577_status mapped = PR_U33_1577_OK;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    if (callee == CALLEE_1403) {
        pr_u33_1403_cpu leaf;
        cpu->ip = (uint16_t)(entry - BACK_1403);
        memcpy(&leaf, cpu, sizeof leaf);
        if (pr_u33_both_outside_1403(ram, ram_size, &leaf) != PR_U33_1403_OK)
            mapped = PR_U33_1577_ARGUMENT;
        memcpy(cpu, &leaf, sizeof leaf);
    } else if (callee == CALLEE_01AD) {
        pr_u33_01ad_cpu leaf;
        pr_u33_01ad_status status;
        cpu->ip = (uint16_t)(entry - BACK_01AD);
        memcpy(&leaf, cpu, sizeof leaf);
        status = pr_u33_cell_pair_01ad(ram, ram_size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
        mapped = status == PR_U33_01AD_OK ? PR_U33_1577_OK
               : status == PR_U33_01AD_CODE ? PR_U33_1577_CODE
               : status == PR_U33_01AD_TRANSFER ? PR_U33_1577_TRANSFER : PR_U33_1577_ARGUMENT;
    } else {
        pr_u33_1434_cpu leaf;
        pr_u33_1434_status status;
        cpu->ip = (uint16_t)(entry - BACK_1434);
        memcpy(&leaf, cpu, sizeof leaf);
        status = pr_u33_cell_claim_1434(ram, ram_size, &leaf, callee_status);
        memcpy(cpu, &leaf, sizeof leaf);
        mapped = status == PR_U33_1434_OK ? PR_U33_1577_OK
               : status == PR_U33_1434_CODE ? PR_U33_1577_CODE
               : status == PR_U33_1434_CALLEE ? PR_U33_1577_CALLEE
               : status == PR_U33_1434_TRANSFER ? PR_U33_1577_TRANSFER : PR_U33_1577_ARGUMENT;
    }
    if (mapped != PR_U33_1577_OK)
        return mapped;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_1577_TRANSFER;
    return PR_U33_1577_OK;
}

#define TRY(call) do { pr_u33_1577_status status_ = (call); if (status_ != PR_U33_1577_OK) return status_; } while (0)

pr_u33_1577_status pr_u33_place_search_1577(
    uint8_t *ram, size_t ram_size, pr_u33_1577_cpu *cpu, unsigned max_loop_entries,
    int *callee_status)
{
    uint16_t entry;
    unsigned entries = 0u;
    uint8_t value;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_1577_RAM_SIZE)
        return PR_U33_1577_ARGUMENT;
    entry = cpu->ip;
    /* +1577..+157A */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub16_sp(cpu, 0x18u);
    /* +157D..+15A8: flags and the starting origin. */
    set_local(ram, cpu, 3u, 1u);
    set_local(ram, cpu, 4u, 0u);
    set_local(ram, cpu, 7u, 1u);
    set_local(ram, cpu, 0x0Fu, 0u);
    set_local(ram, cpu, 0x14u, 0u);
    map_entry(ram, cpu, 0x4650u);
    set_local(ram, cpu, 0x11u, (uint8_t)cpu->ax);
    map_entry(ram, cpu, 0x4652u);
    set_local(ram, cpu, 0x12u, (uint8_t)cpu->ax);
loop_15ab:
    if (entries == max_loop_entries) {
        cpu->ip = (uint16_t)(entry + LOOP_HEAD);
        return PR_U33_1577_LIMIT;
    }
    ++entries;
    /* +15AB..+15CD: [BP-9] := DS:[2E2 + 4*sign(DS:[4656+map]) + pass] >> 1. */
    set_al(cpu, local(ram, cpu, 0x14u));
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    row_index(ram, cpu);
    set_al(cpu, data8(ram, cpu, (uint16_t)(cpu->di + 0x02E2u)));
    cpu->ax &= 0x00FFu;
    cpu->ax = (uint16_t)(cpu->ax >> 1);
    set_local(ram, cpu, 9u, (uint8_t)cpu->ax);
    /* +15D0..+15D7, +1697, +1710: dispatch on the state [BP-7]. */
    set_al(cpu, local(ram, cpu, 7u));
    value = (uint8_t)cpu->ax;
    cmp8(cpu, value, 1u);
    if (value == 1u) {
        /* +15DA..+1694: state 1, the base on ring [BP-0F]. */
        steps(ram, cpu, 2u);
        base_axis(ram, cpu, 0x0Bu, 0x02F6u, 0x15u);
        base_axis(ram, cpu, 0x0Cu, 0x02FEu, 0x16u);
        set_al(cpu, local(ram, cpu, 0x15u));
        set_local(ram, cpu, 0x17u, (uint8_t)cpu->ax);
        set_al(cpu, local(ram, cpu, 0x16u));
        set_local(ram, cpu, 0x18u, (uint8_t)cpu->ax);
        set_local(ram, cpu, 0x10u, 1u);
        set_local(ram, cpu, 7u, 2u);
        set_local(ram, cpu, 0x13u, 1u);
        goto position_178e;
    }
    cmp8(cpu, value, 2u);
    if (value == 2u) {
        /* +169B..+170E: state 2, one side. */
        steps(ram, cpu, 1u);
        offset_axis(ram, cpu, 0x0Bu, 0x15u, 0x17u);
        offset_axis(ram, cpu, 0x0Cu, 0x16u, 0x18u);
        set_local(ram, cpu, 7u, 3u);
        inc_local(ram, cpu, 0x13u);
        goto position_178e;
    }
    cmp8(cpu, value, 3u);
    if (value == 3u) {
        /* +1714..+178B: state 3, the other side, then a longer step. */
        steps(ram, cpu, 3u);
        offset_axis(ram, cpu, 0x0Bu, 0x15u, 0x17u);
        offset_axis(ram, cpu, 0x0Cu, 0x16u, 0x18u);
        set_local(ram, cpu, 7u, 2u);
        inc_local(ram, cpu, 0x10u);
        inc_local(ram, cpu, 0x13u);
    }
position_178e:
    /* +178E..+17AE: [BP-5] := position outside x 0..0Ah or y 0..5 (signed). */
    value = local(ram, cpu, 0x17u);
    cmp8(cpu, value, 0u);
    if ((int8_t)value < 0) goto outside_17a6;
    value = local(ram, cpu, 0x18u);
    cmp8(cpu, value, 0u);
    if ((int8_t)value < 0) goto outside_17a6;
    value = local(ram, cpu, 0x17u);
    cmp8(cpu, value, 0x0Au);
    if ((int8_t)value > 0x0A) goto outside_17a6;
    value = local(ram, cpu, 0x18u);
    cmp8(cpu, value, 0x05u);
    if ((int8_t)value <= 0x05) {
        set_local(ram, cpu, 5u, 0u);
        goto state_17b0;
    }
outside_17a6:
    set_local(ram, cpu, 5u, 1u);
state_17b0:
    /* +17B0..+17FA: in states 2 and 3, decide whether this ring is finished. */
    value = local(ram, cpu, 7u);
    cmp8(cpu, value, 1u);
    if (value <= 1u) goto probe_18b6;
    value = local(ram, cpu, 5u);
    cmp8(cpu, value, 0u);
    if (value != 0u) {
        push_local(ram, cpu, 0x17u);
        push_local(ram, cpu, 0x18u);
        TRY(near_call(ram, ram_size, cpu, entry, 0x0254u, CALLEE_1403, callee_status));
        if ((uint8_t)cpu->ax == 0u) goto next_ring_17fd;
    }
    value = local(ram, cpu, 3u);
    cmp8(cpu, value, 0u);
    if (value != 0u) {
        map_entry(ram, cpu, 0x4654u);
        cbw(cpu);
        cpu->dx = cpu->ax;
        set_al(cpu, local(ram, cpu, 0x13u));
        cpu->ax &= 0x00FFu;
        cmp16(cpu, cpu->ax, cpu->dx);
        if ((int16_t)cpu->ax >= (int16_t)cpu->dx) goto next_ring_17fd;
    }
    value = local(ram, cpu, 3u);
    cmp8(cpu, value, 0u);
    if (value != 0u) goto probe_18b6;
    value = local(ram, cpu, 0x13u);
    cmp8(cpu, value, 0x0Bu);
    if (value <= 0x0Bu) goto probe_18b6;
next_ring_17fd:
    /* +17FD..+18AB: next ring; on map 0 with an odd row byte, pass 0 and ring 1,
     * three +01AD probes around the party may add a ring. */
    inc_local(ram, cpu, 0x0Fu);
    value = data8(ram, cpu, 0x4658u);
    cmp8(cpu, value, 0u);
    if (value != 0u) goto restart_18ae;
    set_al(cpu, data8(ram, cpu, 0x4656u));
    value = (uint8_t)cpu->ax;
    set_al(cpu, (uint8_t)(value >> 1));
    if ((value & 1u) == 0u) goto restart_18ae;
    value = local(ram, cpu, 0x14u);
    cmp8(cpu, value, 0u);
    if (value != 0u) goto restart_18ae;
    value = local(ram, cpu, 0x0Fu);
    cmp8(cpu, value, 1u);
    if (value != 1u) goto restart_18ae;
    party_origin(ram, cpu, 0x6AADu, 0x4650u, 0x0Du);
    party_origin(ram, cpu, 0x6AAEu, 0x4652u, 0x0Eu);
    set_local(ram, cpu, 6u, 0u);
    set_local(ram, cpu, 0x0Au, 1u);
    for (;;) {
        pass_direction(ram, cpu, 0x0Au);
        value = data8(ram, cpu, 0x49FAu);
        cmp8(cpu, value, 1u);
        if (value > 1u) {
            set_local(ram, cpu, 6u, 1u);
        } else {
            push_local(ram, cpu, 0x0Du);
            push_local(ram, cpu, 0x0Eu);
            push_local(ram, cpu, 8u);
            TRY(near_call(ram, ram_size, cpu, entry, 0x0320u, CALLEE_01AD, callee_status));
            cmp8(cpu, (uint8_t)cpu->ax, 1u);
            if ((uint8_t)cpu->ax != 1u)
                set_local(ram, cpu, 6u, 1u);
        }
        value = local(ram, cpu, 0x0Au);
        cmp8(cpu, value, 3u);
        if (value == 3u)
            break;
        inc_local(ram, cpu, 0x0Au);
    }
    value = local(ram, cpu, 6u);
    cmp8(cpu, value, 0u);
    if (value != 0u)
        inc_local(ram, cpu, 0x0Fu);
restart_18ae:
    set_local(ram, cpu, 7u, 1u);
    set_local(ram, cpu, 3u, 0u);
probe_18b6:
    /* +18B6..+18CF: a position outside both axes starts the next pass. */
    value = local(ram, cpu, 5u);
    cmp8(cpu, value, 0u);
    if (value == 0u) goto claim_19a8;
    push_local(ram, cpu, 0x17u);
    push_local(ram, cpu, 0x18u);
    TRY(near_call(ram, ram_size, cpu, entry, 0x0354u, CALLEE_1403, callee_status));
    if ((uint8_t)cpu->ax == 0u) goto claim_19a8;
    /* +18D2..+199B: up to three passes re-seed the origin from the party. */
    set_local(ram, cpu, 2u, 0u);
    set_local(ram, cpu, 7u, 0u);
    for (;;) {
        value = local(ram, cpu, 0x14u);
        cmp8(cpu, value, 3u);
        if (value >= 3u) break;
        value = local(ram, cpu, 7u);
        cmp8(cpu, value, 1u);
        if (value == 1u) break;
        inc_local(ram, cpu, 0x14u);
        party_origin(ram, cpu, 0x6AADu, 0x4650u, 0x0Du);
        party_origin(ram, cpu, 0x6AAEu, 0x4652u, 0x0Eu);
        pass_direction(ram, cpu, 0x14u);
        value = data8(ram, cpu, 0x49FAu);
        cmp8(cpu, value, 1u);
        if (value <= 1u) {
            push_local(ram, cpu, 0x0Du);
            push_local(ram, cpu, 0x0Eu);
            push_local(ram, cpu, 8u);
            TRY(near_call(ram, ram_size, cpu, entry, 0x03DCu, CALLEE_01AD, callee_status));
            cmp8(cpu, (uint8_t)cpu->ax, 1u);
            if ((uint8_t)cpu->ax == 1u)
                continue;
        }
        reseed_axis(ram, cpu, 0x2736u, 0x4650u, 0x11u);
        reseed_axis(ram, cpu, 0x273Fu, 0x4652u, 0x12u);
        set_local(ram, cpu, 0x0Fu, 0u);
        set_local(ram, cpu, 7u, 1u);
    }
    /* +199E..+19A4: no pass left without a new origin ends the search. */
    value = local(ram, cpu, 7u);
    cmp8(cpu, value, 1u);
    if (value != 1u)
        set_local(ram, cpu, 4u, 1u);
claim_19a8:
    /* +19A8..+19CA: an in-grid position is offered to +1434. */
    value = local(ram, cpu, 5u);
    cmp8(cpu, value, 0u);
    if (value == 0u) {
        set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
        push16(ram, cpu, cpu->ax);
        push_local(ram, cpu, 0x17u);
        push_local(ram, cpu, 0x18u);
        push_local(ram, cpu, 0x11u);
        push_local(ram, cpu, 0x12u);
        push_local(ram, cpu, 0x14u);
        TRY(near_call(ram, ram_size, cpu, entry, 0x0453u, CALLEE_1434, callee_status));
        set_local(ram, cpu, 2u, (uint8_t)cpu->ax);
    }
    /* +19CD..+19D9: loop until a claim or exhaustion. */
    value = local(ram, cpu, 2u);
    cmp8(cpu, value, 0u);
    if (value == 0u) {
        value = local(ram, cpu, 4u);
        cmp8(cpu, value, 0u);
        if (value == 0u)
            goto loop_15ab;
    }
    /* +19DC..+19F2: AL := exhausted ? 0 : 1; MOV SP,BP; POP BP; RETF 2. */
    value = local(ram, cpu, 4u);
    cmp8(cpu, value, 0u);
    set_local(ram, cpu, 1u, value != 0u ? 0u : 1u);
    set_al(cpu, local(ram, cpu, 1u));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_U33_1577_OK;
}
