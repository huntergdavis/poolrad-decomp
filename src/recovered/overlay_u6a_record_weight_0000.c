/* GAME.OVR unit 0x6A:+0000..+0158; docs/re/U6A_RECORD_WEIGHT_0000.md. */
#include "poolrad/recovered/overlay_u6a_record_weight_0000.h"
#include "overlay_loaded_stubs.h"
#include "poolrad/recovered/overlay_u86_record_predicate_323d.h"

#include <string.h>

#define MASK (PR_U6A_0000_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */
#define CALLEE_IP 0x323Du   /* unit 0x86 entry 6 behind stub 0096:003E */
#define RETURN_DELTA 0x8Fu /* the far call site plus five, from the entry offset */
#define TABLE 0x31FBu

_Static_assert(sizeof(pr_u6a_0000_cpu) == sizeof(pr_u86_323d_cpu),
               "u86 +323D CPU state layout differs");

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

/* LES DI,SS:[offset]: both words are physically contiguous. */
static void les(const uint8_t *ram, pr_u6a_0000_cpu *cpu, uint16_t offset)
{
    size_t at = physical(cpu->ss, offset);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
}

static void push16(uint8_t *ram, pr_u6a_0000_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u6a_0000_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint8_t local(const uint8_t *ram, const pr_u6a_0000_cpu *cpu, uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static void set_local(uint8_t *ram, const pr_u6a_0000_cpu *cpu, uint16_t back, uint8_t value)
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

/* OR flags: CF/OF clear, undefined AF represented as zero. */
static void or_flags(pr_u6a_0000_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

/* ADD AX,source with every defined flag. */
static void add16(pr_u6a_0000_cpu *cpu, uint16_t source)
{
    uint16_t left = cpu->ax;
    uint32_t wide = (uint32_t)left + source;
    uint16_t result = (uint16_t)wide;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (wide & 0x10000u) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ source ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((left ^ result) & (source ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    cpu->ax = result;
}

/* XOR AH,AH; MOV DX,imm; MUL DX: AX:DX := zero-extended AL times imm.
 * MUL's flags are overwritten by the two ADDs before any observation. */
static void mul_byte(pr_u6a_0000_cpu *cpu, uint16_t factor)
{
    uint32_t product;
    cpu->ax &= 0x00FFu;
    cpu->dx = factor;
    product = (uint32_t)cpu->ax * cpu->dx;
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

/* The shared lookup step: AL := byte, AND AL,7F; XOR AH,AH; DI := AX << 4;
 * AL := DS:[DI+31FB] through the 16-bit offset wrap. CL becomes 4. */
static void table_lookup(const uint8_t *ram, pr_u6a_0000_cpu *cpu, uint8_t byte)
{
    set_al(cpu, (uint8_t)(byte & 0x7Fu));
    cpu->ax &= 0x00FFu;
    cpu->di = cpu->ax;
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    cpu->di = (uint16_t)(cpu->di << 4);
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + TABLE)));
}

/* +008A: LCALL 0096:003E. The loaded stub jumps to unit 0x86 +323D; the
 * callee sees the pushed CS:+008F and consumes it with RETF 4. */
static pr_u6a_0000_status call_323d(uint8_t *ram, size_t ram_size, pr_u6a_0000_cpu *cpu,
                                    uint16_t return_ip, uint16_t callee_cs)
{
    pr_u86_323d_cpu leaf;
    uint16_t caller_cs = cpu->cs;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->cs = callee_cs;
    cpu->ip = CALLEE_IP;
    memcpy(&leaf, cpu, sizeof leaf);
    if (pr_u86_record_predicate_323d(ram, ram_size, &leaf) != PR_U86_323D_OK)
        return PR_U6A_0000_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U6A_0000_TRANSFER;
    return PR_U6A_0000_OK;
}

pr_u6a_0000_status pr_u6a_record_weight_0000(
    uint8_t *ram, size_t ram_size, pr_u6a_0000_cpu *cpu)
{
    uint16_t return_ip, callee_cs;
    if (ram == NULL || cpu == NULL || ram_size < PR_U6A_0000_RAM_SIZE)
        return PR_U6A_0000_ARGUMENT;
    /* Reject cold/malformed stubs before mutation; honor the live 0xEA target. */
    if (!pr_overlay_stub_target(ram, cpu->cs, (uint16_t)(cpu->ip + 0x8Au),
                                0x003Eu, CALLEE_IP, &callee_cs))
        return PR_U6A_0000_ARGUMENT;
    /* The loaded game enters at offset 0 of its unit; the pushed return offset follows the entry offset. */
    return_ip = (uint16_t)(cpu->ip + RETURN_DELTA);

    /* +0000..+0016: frame, fourteen local bytes, five cleared locals. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x0Eu);
    set_local(ram, cpu, 3u, 0u);   /* first maximum */
    set_local(ram, cpu, 4u, 0u);   /* second maximum */
    set_local(ram, cpu, 5u, 0u);   /* first sum */
    set_local(ram, cpu, 6u, 0u);   /* second sum */
    set_local(ram, cpu, 0x0Bu, 0u); /* index */

    /* +001F..+0067: record bytes +17+index for index 0..14h. */
    for (;;) {
        uint8_t byte;
        set_al(cpu, local(ram, cpu, 0x0Bu));
        cpu->ax &= 0x00FFu;
        les(ram, cpu, (uint16_t)(cpu->bp + 6u));
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        byte = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x17u));
        if (byte > 0x7Fu) {
            uint8_t value;
            set_al(cpu, local(ram, cpu, 0x0Bu));
            cpu->ax &= 0x00FFu;
            les(ram, cpu, (uint16_t)(cpu->bp + 6u));
            cpu->di = (uint16_t)(cpu->di + cpu->ax);
            table_lookup(ram, cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x17u)));
            set_local(ram, cpu, 0x0Cu, (uint8_t)cpu->ax);
            value = local(ram, cpu, 0x0Cu);
            set_al(cpu, value);
            if (value > local(ram, cpu, 3u)) {
                set_al(cpu, local(ram, cpu, 0x0Cu));
                set_local(ram, cpu, 3u, (uint8_t)cpu->ax);
            }
            set_al(cpu, local(ram, cpu, 0x0Cu));
            set_local(ram, cpu, 5u, (uint8_t)(local(ram, cpu, 5u) + (uint8_t)cpu->ax));
        }
        if (local(ram, cpu, 0x0Bu) == 0x14u)
            break;
        set_local(ram, cpu, 0x0Bu, (uint8_t)(local(ram, cpu, 0x0Bu) + 1u));
    }

    /* +0069..+0079: current := record^.link at +C8/+CA. */
    les(ram, cpu, (uint16_t)(cpu->bp + 6u));
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xC8u));
    cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCAu));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Au), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->dx);

    for (;;) {
        pr_u6a_0000_status status;

        /* +007C..+0082: both pointer words zero ends the walk. */
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Au));
        cpu->ax = (uint16_t)(cpu->ax | read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u)));
        or_flags(cpu, cpu->ax, 0x8000u);
        if (cpu->ax == 0u)
            break;

        /* +0084..+008A: push segment then offset, rereading after each push. */
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Au)));
        status = call_323d(ram, ram_size, cpu, return_ip, callee_cs);
        if (status != PR_U6A_0000_OK)
            return status;

        /* +008F..+0091: OR AL,AL on the predicate result. */
        or_flags(cpu, (uint8_t)cpu->ax, 0x80u);
        if ((uint8_t)cpu->ax != 0u) {
            /* +0093: index := 1; +0099..+00E4: node bytes +3B+index for 1..3. */
            set_local(ram, cpu, 0x0Bu, 1u);
            for (;;) {
                uint8_t byte;
                set_al(cpu, local(ram, cpu, 0x0Bu));
                cpu->ax &= 0x00FFu;
                les(ram, cpu, (uint16_t)(cpu->bp - 0x0Au));
                cpu->di = (uint16_t)(cpu->di + cpu->ax);
                byte = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x3Bu));
                if (byte > 0x7Fu) {
                    uint8_t value;
                    set_al(cpu, local(ram, cpu, 0x0Bu));
                    cpu->ax &= 0x00FFu;
                    les(ram, cpu, (uint16_t)(cpu->bp - 0x0Au));
                    cpu->di = (uint16_t)(cpu->di + cpu->ax);
                    table_lookup(ram, cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x3Bu)));
                    set_local(ram, cpu, 0x0Cu, (uint8_t)cpu->ax);
                    value = local(ram, cpu, 0x0Cu);
                    set_al(cpu, value);
                    if (value > local(ram, cpu, 4u)) {
                        set_al(cpu, local(ram, cpu, 0x0Cu));
                        set_local(ram, cpu, 4u, (uint8_t)cpu->ax);
                    }
                    set_al(cpu, local(ram, cpu, 0x0Cu));
                    set_local(ram, cpu, 6u, (uint8_t)(local(ram, cpu, 6u) + (uint8_t)cpu->ax));
                }
                if (local(ram, cpu, 0x0Bu) == 3u)
                    break;
                set_local(ram, cpu, 0x0Bu, (uint8_t)(local(ram, cpu, 0x0Bu) + 1u));
            }
        }

        /* +00E6..+00F7: current := current^.link at +2A/+2C. */
        les(ram, cpu, (uint16_t)(cpu->bp - 0x0Au));
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Au), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->dx);
    }

    /* +00F9..+0119: tier 0, 4 when either sum is nonzero, 6 when either
     * maximum is above 2; every compare rereads its live local. */
    set_local(ram, cpu, 0x0Du, 0u);
    if (local(ram, cpu, 5u) > 0u || local(ram, cpu, 6u) > 0u)
        set_local(ram, cpu, 0x0Du, 4u);
    if (local(ram, cpu, 3u) > 2u || local(ram, cpu, 4u) > 2u)
        set_local(ram, cpu, 0x0Du, 6u);

    /* +011D..+0123: record +2C := tier. */
    set_al(cpu, local(ram, cpu, 0x0Du));
    les(ram, cpu, (uint16_t)(cpu->bp + 6u));
    write8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu), (uint8_t)cpu->ax);

    /* +0127..+014B: 15*sum2 (BX), 15*sum1 (CX), 60*tier, two ADDs. */
    set_al(cpu, local(ram, cpu, 6u));
    mul_byte(cpu, 0x000Fu);
    cpu->bx = cpu->ax;
    set_al(cpu, local(ram, cpu, 5u));
    mul_byte(cpu, 0x000Fu);
    cpu->cx = cpu->ax;
    set_al(cpu, local(ram, cpu, 0x0Du));
    mul_byte(cpu, 0x003Cu);
    add16(cpu, cpu->cx);
    add16(cpu, cpu->bx);

    /* +014D..+0156: result through the local word; MOV SP,BP; POP BP; RETF 4. */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U6A_0000_OK;
}
