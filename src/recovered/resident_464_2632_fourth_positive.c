#include "poolrad/recovered/resident_464_2632_fourth_positive.h"

#include <string.h>

enum {
    CF = 0x0001u, PF = 0x0004u, AF = 0x0010u,
    ZF = 0x0040u, SF = 0x0080u, OF = 0x0800u,
    ARITH = CF | PF | AF | ZF | SF | OF
};

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint16_t word_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return (uint16_t)(ram[physical(segment, offset)] |
        ((uint16_t)ram[physical(segment, (uint16_t)(offset + 1u))] << 8));
}

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t value)
{
    ram[physical(segment, offset)] = (uint8_t)value;
    ram[physical(segment, (uint16_t)(offset + 1u))] = (uint8_t)(value >> 8);
}

static uint8_t even_parity(uint8_t value)
{
    unsigned bits = 0;
    while (value != 0u) {
        bits ^= value & 1u;
        value >>= 1;
    }
    return (uint8_t)(bits == 0u);
}

static uint16_t sub_flags16(uint16_t flags, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    flags &= (uint16_t)~ARITH;
    if (left < right) flags |= CF;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u) flags |= OF;
    return flags;
}

static uint16_t logic_flags16(uint16_t flags, uint16_t result)
{
    flags &= (uint16_t)~ARITH;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    return flags;
}

static uint16_t add_flags16(uint16_t flags, uint16_t left, uint16_t right)
{
    uint32_t full = (uint32_t)left + right;
    uint16_t result = (uint16_t)full;
    flags &= (uint16_t)~ARITH;
    if (full > 0xFFFFu) flags |= CF;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (((~(left ^ right)) & (left ^ result) & 0x8000u) != 0u) flags |= OF;
    return flags;
}

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_positive(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    static const pr_r464_2632_cpu measured_pre = {
        0x0C88u, 0x2656u, 0x3DECu, 0x3E4Eu, 0u, 0x14A0u, 0x2612u,
        0x3E80u, 0x3E80u, 0x3E5Cu, 0x14A0u, 0x1D16u, 0x1D16u, 0x7202u
    };
    uint16_t x, count, object_off, object_seg;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (memcmp(cpu, &measured_pre, sizeof(*cpu)) != 0)
        return PR_R464_2632_UNSUPPORTED_PATH;
    x = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x16u));
    count = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x12u));
    object_off = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    object_seg = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    if (x > 0x7FFFu || count > 64u ||
        (uint32_t)x + count > 0x7FFFu ||
        object_off != 8u || object_seg != 0x3DE4u)
        return PR_R464_2632_UNSUPPORTED_PATH;

    /* +2656 CMP [BP+16],0; +265A JGE +266E. */
    cpu->flags = sub_flags16(cpu->flags, x, 0u);
    observe(0x265Au, ram, cpu, observer, context);
    observe(0x266Eu, ram, cpu, observer, context);

    /* Positive arm +266E..+2679. */
    cpu->ax = 0u;
    cpu->flags = logic_flags16(cpu->flags, cpu->ax);
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Au), cpu->ax);
    cpu->ax = x;
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Eu), cpu->ax);
    observe(0x2679u, ram, cpu, observer, context);

    /* Shared arithmetic and object width read, +2679..+2699. */
    cpu->ax = count;
    cpu->dx = 0u;
    cpu->flags = logic_flags16(cpu->flags, cpu->dx);
    cpu->cx = cpu->ax;
    cpu->bx = cpu->dx;
    cpu->ax = x;
    cpu->dx = 0u; /* CDQ after a nonnegative x. */
    cpu->flags = add_flags16(cpu->flags, cpu->ax, cpu->cx);
    cpu->ax = (uint16_t)(cpu->ax + cpu->cx);
    cpu->flags = add_flags16(cpu->flags, cpu->dx, cpu->bx);
    cpu->dx = 0u; /* The guarded ADD has no carry. */
    cpu->cx = cpu->ax;
    cpu->bx = cpu->dx;
    cpu->di = object_off;
    cpu->es = object_seg;
    cpu->ax = word_at(ram, cpu->es, (uint16_t)(cpu->di + 2u));
    cpu->dx = 0u;
    cpu->flags = logic_flags16(cpu->flags, cpu->dx);
    cpu->flags = sub_flags16(cpu->flags, cpu->dx, cpu->bx);
    observe(0x2699u, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
