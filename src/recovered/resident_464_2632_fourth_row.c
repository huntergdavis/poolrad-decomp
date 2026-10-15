#include "poolrad/recovered/resident_464_2632_fourth_row.h"

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
    unsigned odd = 0;
    while (value != 0u) { odd ^= value & 1u; value >>= 1; }
    return (uint8_t)(odd == 0u);
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

static uint16_t zero_flags(uint16_t flags)
{
    return (uint16_t)((flags & (uint16_t)~ARITH) | PF | ZF);
}

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_row(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    uint16_t row, total, x, count, width, height, sum, product;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    row = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x14u));
    total = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x10u));
    x = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x16u));
    count = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 0x12u));
    width = word_at(ram, cpu->es, (uint16_t)(cpu->di + 2u));
    height = word_at(ram, cpu->es, cpu->di);
    if (cpu->cs != 0x0C88u || cpu->ip != 0x26FBu ||
        cpu->bx != 0u || cpu->dx != 0u || cpu->si != 0x2612u ||
        cpu->di != 8u || cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u || cpu->ss != 0x1D16u ||
        row > 0x7FFFu || total == 0u || total > 256u ||
        x > 0x7FFFu || count > 64u ||
        (uint32_t)x + count > 0x7FFFu ||
        cpu->cx != (uint16_t)(x + count) || width < cpu->cx ||
        cpu->ax != (uint16_t)(width - cpu->cx) ||
        width == 0u || width > 512u ||
        (uint32_t)row * width > 0xFFFFu ||
        (uint32_t)row + total > 0x7FFFu ||
        cpu->flags != sub_flags16(cpu->flags, row, 0u) ||
        word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)) != 8u ||
        word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)) != 0x3DE4u)
        return PR_R464_2632_UNSUPPORTED_PATH;
    product = (uint16_t)(row * width);
    sum = (uint16_t)(row + total);

    /* +26FB JGE jumps to +2722 for the nonnegative saved row. */
    observe(0x2722u, ram, cpu, observer, context);
    cpu->ax = row;
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1Eu), row);
    observe(0x2728u, ram, cpu, observer, context);
    cpu->ax = row;
    cpu->di = 8u;
    cpu->es = 0x3DE4u;
    cpu->ax = product;
    cpu->dx = 0u; /* Guarded MUL has no high word. */
    observe(0x2732u, ram, cpu, observer, context);
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 0x16u), product);
    observe(0x2735u, ram, cpu, observer, context);
    cpu->ax = (uint16_t)(total - 1u);
    cpu->flags = sub_flags16(cpu->flags, total, 1u); /* DEC preserves CF=0. */
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 0x14u), cpu->ax);
    observe(0x273Cu, ram, cpu, observer, context);
    cpu->ax = total;
    cpu->dx = 0u;
    cpu->flags = zero_flags(cpu->flags);
    cpu->cx = total;
    cpu->bx = 0u;
    cpu->ax = row;
    cpu->dx = 0u;
    cpu->ax = sum;
    cpu->dx = 0u;
    cpu->flags = zero_flags(cpu->flags); /* Guarded ADC DX,BX yields zero. */
    cpu->cx = sum;
    cpu->bx = 0u;
    observe(0x2751u, ram, cpu, observer, context);
    cpu->di = 8u;
    cpu->es = 0x3DE4u;
    cpu->ax = height;
    cpu->dx = 0u;
    cpu->flags = zero_flags(cpu->flags); /* XOR DX and CMP DX,BX. */
    observe(0x275Bu, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
