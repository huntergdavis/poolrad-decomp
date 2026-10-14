#include "poolrad/recovered/overlay_u3bd_jne_080a.h"

#define STATUS_MASK 0x08D5u
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u

static uint32_t physical(uint16_t seg, uint16_t off)
{
    return (((uint32_t)seg << 4) + off) & 0xFFFFFu;
}

static uint8_t byte_at(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
}

static uint16_t word_at(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return (uint16_t)(byte_at(ram, seg, off) |
                      ((uint16_t)byte_at(ram, seg, (uint16_t)(off + 1u)) << 8));
}

static void put_byte(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

static void put_word(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    put_byte(ram, seg, off, (uint8_t)value);
    put_byte(ram, seg, (uint16_t)(off + 1u), (uint8_t)(value >> 8));
}

static void push(uint8_t *ram, pr_u3bd_jne_080a_frame *f, uint16_t value)
{
    f->sp = (uint16_t)(f->sp - 2u);
    put_word(ram, f->ss, f->sp, value);
}

static uint16_t set_al(uint16_t ax, uint8_t low)
{
    return (uint16_t)((ax & 0xFF00u) | low);
}

static uint16_t parity_flag(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u ? PF : 0u;
}

static void cmp_zero8(pr_u3bd_jne_080a_frame *f, uint8_t value)
{
    f->flags = (uint16_t)((f->flags & (uint16_t)~STATUS_MASK) |
                          parity_flag(value) |
                          (value == 0u ? ZF : 0u) |
                          ((value & 0x80u) != 0u ? SF : 0u));
}

static void add16(pr_u3bd_jne_080a_frame *f, uint16_t other)
{
    uint16_t left = f->ax;
    uint32_t wide = (uint32_t)left + other;
    uint16_t result = (uint16_t)wide;
    uint16_t bits = parity_flag((uint8_t)result);
    if (wide > 0xFFFFu) bits |= CF;
    if (((left ^ other ^ result) & 0x0010u) != 0u) bits |= AF;
    if (result == 0u) bits |= ZF;
    if ((result & 0x8000u) != 0u) bits |= SF;
    if (((~(left ^ other) & (left ^ result)) & 0x8000u) != 0u) bits |= OF;
    f->ax = result;
    f->flags = (uint16_t)((f->flags & (uint16_t)~STATUS_MASK) | bits);
}

static void dec16(pr_u3bd_jne_080a_frame *f)
{
    uint16_t old = f->ax;
    uint16_t result = (uint16_t)(old - 1u);
    uint16_t bits = (uint16_t)(f->flags & CF);
    bits |= parity_flag((uint8_t)result);
    if ((old & 0x000Fu) == 0u) bits |= AF;
    if (result == 0u) bits |= ZF;
    if ((result & 0x8000u) != 0u) bits |= SF;
    if (old == 0x8000u) bits |= OF;
    f->ax = result;
    f->flags = (uint16_t)((f->flags & (uint16_t)~STATUS_MASK) | bits);
}

pr_u3bd_080a_result pr_u3bd_jne_080a(uint8_t *ram, size_t ram_size,
                                       pr_u3bd_jne_080a_frame *f,
                                       uint16_t *ip,
                                       int stop_before_second_call)
{
    uint8_t value;
    if (ram == NULL || f == NULL || ip == NULL ||
        ram_size < PR_U3BD_LOOP_RESET_RAM_BYTES ||
        f->cs != 0x2D14u || *ip != 0x080Au ||
        (stop_before_second_call != 0 && stop_before_second_call != 1))
        return PR_U3BD_080A_INVALID;

    if ((f->flags & ZF) != 0u) {
        /* +080C..+0819: the opposite JNE arm calls local +047B. */
        f->ax = set_al(f->ax, byte_at(ram, f->ss, (uint16_t)(f->bp - 0x0Du)));
        push(ram, f, f->ax);
        f->ax = set_al(f->ax, byte_at(ram, f->ss, (uint16_t)(f->bp - 0x0Fu)));
        push(ram, f, f->ax);
        f->ax = set_al(f->ax, byte_at(ram, f->ss, (uint16_t)(f->bp - 1u)));
        push(ram, f, f->ax);
        push(ram, f, f->cs);
        push(ram, f, 0x081Cu);
        *ip = 0x047Bu;
        return PR_U3BD_080A_CALLEE_047B;
    }

    value = byte_at(ram, f->ss, (uint16_t)(f->bp - 0x14u));
    cmp_zero8(f, value); /* +0824; +0828 JE */
    if (value == 0u) {
        *ip = 0x0865u;
        return PR_U3BD_080A_BRANCH_0865;
    }

    value = byte_at(ram, f->ss, (uint16_t)(f->bp - 0x17u));
    cmp_zero8(f, value); /* +082A; +082E JBE */
    if (value != 0u) {
        /* +0830..+0844: first local +0211 call. */
        f->ax = word_at(ram, f->ds, 0x0CBCu);
        add16(f, word_at(ram, f->ss, (uint16_t)(f->bp - 0x12u)));
        dec16(f);
        push(ram, f, f->ax);
        f->ax = set_al(f->ax, byte_at(ram, f->ds, 0x0CD0u));
        push(ram, f, f->ax);
        f->ax = set_al(f->ax, value);
        push(ram, f, f->ax);
        f->ax = set_al(f->ax, 9u);
        push(ram, f, f->ax);
        push(ram, f, f->cs);
        push(ram, f, 0x0847u);
        *ip = 0x0211u;
        return PR_U3BD_080A_FIRST_CALLEE_0211;
    }

    /* +0847..+0860: saved slot-A arm; stop before or just inside CALL. */
    f->ax = set_al(f->ax, byte_at(ram, f->ss, (uint16_t)(f->bp - 0x14u)));
    put_byte(ram, f->ss, (uint16_t)(f->bp - 0x17u), (uint8_t)f->ax);
    f->ax = word_at(ram, f->ds, 0x0CAAu);
    add16(f, word_at(ram, f->ss, (uint16_t)(f->bp - 0x12u)));
    push(ram, f, f->ax);
    f->ax = set_al(f->ax, byte_at(ram, f->ds, 0x0CBEu));
    push(ram, f, f->ax);
    f->ax = set_al(f->ax, byte_at(ram, f->ss, (uint16_t)(f->bp - 0x14u)));
    push(ram, f, f->ax);
    f->ax = set_al(f->ax, 0u);
    push(ram, f, f->ax);
    push(ram, f, f->cs);
    if (stop_before_second_call != 0) {
        *ip = 0x0860u;
        return PR_U3BD_080A_PRECALL_0211;
    }
    push(ram, f, 0x0863u);
    *ip = 0x0211u;
    return PR_U3BD_080A_CALLEE_0211;
}
