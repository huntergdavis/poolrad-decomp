/* GAME.OVR unit 0x31:+0675..+07D5; docs/re/U31_STEP_GATE_0675.md. */
#include "poolrad/recovered/overlay_u31_step_gate_0675.h"
#include "poolrad/recovered/overlay_u3c6_0cb9_cpu.h"
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"

#include <string.h>

#define MASK (PR_U31_0675_RAM_SIZE - 1u)
#define SCAN_SITE 0x65u            /* +06DA: LCALL 03E0:0070 */
#define STUB_OFFSET 0x70u
#define SCAN_ENTRY 0x0CB9u
#define LOOKUP_ENTRY 0x2199u

_Static_assert(sizeof(pr_u31_0675_cpu) == sizeof(pr_u3c6_0cb9_cpu), "u3C6 +0CB9 CPU layout differs");
_Static_assert(sizeof(pr_u31_0675_cpu) == sizeof(pr_res_2199_cpu), "RES_00BA_2199 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u31_0675_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

/* LES DI,[segment:offset]. */
static void les(const uint8_t *ram, pr_u31_0675_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    uint16_t di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->di = di;
}

/* LES DI,[BP+0Eh]; LES DI,ES:[DI+108h]: ES:DI := Y. */
static void load_y(const uint8_t *ram, pr_u31_0675_cpu *cpu)
{
    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));
    les(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x108u));
}

static uint8_t frame8(const uint8_t *ram, const pr_u31_0675_cpu *cpu, int delta)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp + (unsigned)delta));
}

static void set_frame8(uint8_t *ram, const pr_u31_0675_cpu *cpu, int delta, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp + (unsigned)delta), value);
}

static void set_al(pr_u31_0675_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

/* MOV AL,byte; XOR AH,AH (XOR's flags folded). */
static void load_byte(pr_u31_0675_cpu *cpu, uint8_t value)
{
    cpu->ax = value;
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u31_0675_cpu *cpu, uint8_t left, uint8_t right)
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

static void note(pr_u31_0675_callee *callee, uint16_t site, int status)
{
    if (callee != NULL) {
        callee->site = site;
        callee->status = status;
    }
}

/* +06DA: LCALL through the 03E0:0070 stub to U3C6 +0CB9 (RETF 0Eh). */
static pr_u31_0675_status scan_call(uint8_t *ram, size_t ram_size, pr_u31_0675_cpu *cpu, uint16_t entry,
                                    pr_u31_0675_callee *callee)
{
    uint16_t caller_cs = cpu->cs, return_ip = (uint16_t)(entry + SCAN_SITE + 5u), stub_segment;
    pr_u3c6_0cb9_cpu leaf;
    pr_u3c6_0cb9_cpu_status status;
    if (read8(ram, cpu->cs, (uint16_t)(entry + SCAN_SITE)) != 0x9Au ||
        read16(ram, cpu->cs, (uint16_t)(entry + SCAN_SITE + 1u)) != STUB_OFFSET)
        return PR_U31_0675_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(entry + SCAN_SITE + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu ||
        read16(ram, stub_segment, (uint16_t)(STUB_OFFSET + 1u)) != SCAN_ENTRY)
        return PR_U31_0675_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = SCAN_ENTRY;
    cpu->cs = read16(ram, stub_segment, (uint16_t)(STUB_OFFSET + 3u));
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_u3c6_0cb9_cpu_run(ram, ram_size, &leaf, PR_U3C6_0CB9_RETF_CONTIGUOUS_PHYSICAL);
    memcpy(cpu, &leaf, sizeof leaf);
    note(callee, 0x06DAu, (int)status);
    if (status != PR_U3C6_0CB9_CPU_RETURNED)
        return PR_U31_0675_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U31_0675_TRANSFER;
    return PR_U31_0675_OK;
}

/* +0758/+076F/+0786/+079D: LCALL to resident RES_00BA_2199 (RETF 0Ah). */
static pr_u31_0675_status lookup_call(uint8_t *ram, size_t ram_size, pr_u31_0675_cpu *cpu, uint16_t entry,
                                      uint16_t site, unsigned max_loopbacks, pr_u31_0675_callee *callee)
{
    uint16_t caller_cs = cpu->cs, at = (uint16_t)(entry + (uint16_t)(site - 0x0675u));
    uint16_t return_ip = (uint16_t)(at + 5u);
    pr_res_2199_cpu leaf;
    pr_res_2199_status status;
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != LOOKUP_ENTRY)
        return PR_U31_0675_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->cs = read16(ram, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = LOOKUP_ENTRY;
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_res_00ba_2199_cpu(ram, ram_size, &leaf, max_loopbacks);
    memcpy(cpu, &leaf, sizeof leaf);
    note(callee, site, (int)status);
    if (status != PR_RES_2199_OK)
        return PR_U31_0675_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U31_0675_TRANSFER;
    return PR_U31_0675_OK;
}

pr_u31_0675_status pr_u31_step_gate_0675(uint8_t *ram, size_t ram_size, pr_u31_0675_cpu *cpu,
                                         unsigned lookup_max_loopbacks, pr_u31_0675_callee *callee)
{
    static const uint8_t keys[4] = {0x20u, 0x1Eu, 0x6Fu, 0x7Du};
    static const uint16_t sites[4] = {0x0758u, 0x076Fu, 0x0786u, 0x079Du};
    uint16_t entry, product_low;
    uint32_t product;
    int32_t dividend;
    uint8_t value, odd;
    pr_u31_0675_status status;
    size_t at;
    note(callee, 0u, -1);
    if (ram == NULL || cpu == NULL || ram_size != PR_U31_0675_RAM_SIZE)
        return PR_U31_0675_ARGUMENT;
    entry = cpu->ip;
    /* +0675..+0678: PUSH BP; MOV BP,SP; SUB SP,0Ah (SUB's flags are overwritten by the first CMP or a callee). */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x0Au);
    /* +067B..+0682: *R := 0; [BP-1] := 0. */
    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
    write8(ram, cpu->es, cpu->di, 0u);
    set_frame8(ram, cpu, -1, 0u);
    /* +0686..+06A5: DX := c; ES:DI := Y; DI := 5 * Y[15h] + c (SHL/ADD flags folded). */
    load_byte(cpu, frame8(ram, cpu, 0x0C));
    cpu->dx = cpu->ax;
    load_y(ram, cpu);
    load_byte(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x15u)));
    cpu->di = cpu->ax;
    cpu->si = cpu->di;
    cpu->di = (uint16_t)(cpu->di << 2);
    cpu->di = (uint16_t)(cpu->di + cpu->si);
    cpu->di = (uint16_t)(cpu->di + cpu->dx);
    /* +06A7..+06BA: [BP-6] := t; AX := b + [BP-6]. */
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x02AEu)));
    set_frame8(ram, cpu, -6, (uint8_t)cpu->ax);
    load_byte(cpu, frame8(ram, cpu, -6));
    cpu->dx = cpu->ax;
    load_byte(cpu, frame8(ram, cpu, 0x0A));
    cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
    /* +06BC..+06C3: CWD; MOV CX,8; IDIV CX; XCHG DX,AX; [BP-7] := remainder. The dividend is a sign-extended
     * word, so the quotient always fits and IDIV cannot fault; its undefined flags are folded. */
    cpu->dx = (cpu->ax & 0x8000u) ? 0xFFFFu : 0x0000u;
    cpu->cx = 8u;
    dividend = (int32_t)(int16_t)cpu->ax;
    cpu->dx = (uint16_t)(int16_t)(dividend % 8);
    cpu->ax = (uint16_t)(int16_t)(dividend / 8);
    {
        uint16_t swap = cpu->ax;
        cpu->ax = cpu->dx;
        cpu->dx = swap;
    }
    set_frame8(ram, cpu, -7, (uint8_t)cpu->ax);
    /* +06C6..+06DA: push Q = X, k = [BP-7], P10 = SS:BP-8, P6 = SS:BP-9; LCALL +0CB9. */
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x10u)));
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu)));
    set_al(cpu, frame8(ram, cpu, -7));
    push16(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 8u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 9u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    status = scan_call(ram, ram_size, cpu, entry, callee);
    if (status != PR_U31_0675_OK)
        return status;
    /* +06DF..+06EC: if [BP-9] = 0, *R := 1 and leave. */
    value = frame8(ram, cpu, -9);
    cmp8(cpu, value, 0u);
    if (value == 0u) {
        les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
        write8(ram, cpu->es, cpu->di, 1u);
        goto leave;
    }
    /* +06EF..+06F5: leave if [BP-8] is nonzero. */
    value = frame8(ram, cpu, -8);
    cmp8(cpu, value, 0u);
    if (value != 0u)
        goto leave;
    /* +06F8..+0715: AL := w = DS:[2744 + 4*[BP-9]]; leave unless w < Y[6]. */
    load_byte(cpu, frame8(ram, cpu, -9));
    cpu->di = (uint16_t)(cpu->ax << 2);
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x2744u)));
    load_y(ram, cpu);
    value = read8(ram, cpu->es, (uint16_t)(cpu->di + 6u));
    cmp8(cpu, (uint8_t)cpu->ax, value);
    if (!((uint8_t)cpu->ax < value))
        goto leave;
    /* +0718..+0723: [BP-0Ah] := 2, or 3 when SHR AL,1 of [BP-7] carries (the remainder is odd). */
    set_frame8(ram, cpu, -0x0A, 2u);
    set_al(cpu, frame8(ram, cpu, -7));
    odd = (uint8_t)(cpu->ax & 1u);
    set_al(cpu, (uint8_t)((uint8_t)cpu->ax >> 1));
    if (odd)
        set_frame8(ram, cpu, -0x0A, 3u);
    /* +0727..+0741: [BP-0Ah] := low byte of [BP-0Ah] * w (MUL DX; DX := high word). */
    load_byte(cpu, frame8(ram, cpu, -9));
    cpu->di = (uint16_t)(cpu->ax << 2);
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x2744u)));
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    load_byte(cpu, frame8(ram, cpu, -0x0A));
    product = (uint32_t)cpu->ax * cpu->dx;
    product_low = (uint16_t)product;
    cpu->ax = product_low;
    cpu->dx = (uint16_t)(product >> 16);
    set_frame8(ram, cpu, -0x0A, (uint8_t)cpu->ax);
    /* +0744..+07B5: when [BP-9] is 1Eh, look up 20h/1Eh/6Fh/7Dh; with no hit [BP-0Ah] := Y[6] + 1. */
    value = frame8(ram, cpu, -9);
    cmp8(cpu, value, 0x1Eu);
    if (value == 0x1Eu) {
        unsigned n;
        for (n = 0u; n < 4u; ++n) {
            push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x10u)));
            push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu)));
            set_al(cpu, keys[n]);
            push16(ram, cpu, cpu->ax);
            cpu->di = (uint16_t)(cpu->bp - 5u);
            push16(ram, cpu, cpu->ss);
            push16(ram, cpu, cpu->di);
            status = lookup_call(ram, ram_size, cpu, entry, sites[n], lookup_max_loopbacks, callee);
            if (status != PR_U31_0675_OK)
                return status;
            if ((uint8_t)cpu->ax != 0u)                       /* OR AL,AL; JNE +07B8 (OR's flags folded) */
                break;
        }
        if (n == 4u) {
            load_y(ram, cpu);
            load_byte(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 6u)));
            cpu->ax = (uint16_t)(cpu->ax + 1u);                /* INC AX (flags folded) */
            set_frame8(ram, cpu, -0x0A, (uint8_t)cpu->ax);
        }
    }
    /* +07B8..+07C9: the result := 1 unless Y[6] < [BP-0Ah]. */
    load_y(ram, cpu);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 6u)));
    value = frame8(ram, cpu, -0x0A);
    cmp8(cpu, (uint8_t)cpu->ax, value);
    if (!((uint8_t)cpu->ax < value))
        set_frame8(ram, cpu, -1, 1u);
leave:
    /* +07CD..+07D3: MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 0Ch. */
    set_al(cpu, frame8(ram, cpu, -1));
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Cu);
    return PR_U31_0675_OK;
}
