#include "poolrad/recovered/resident_464_2632_fourth_2981.h"

enum {
    CF = 0x0001u, PF = 0x0004u, AF = 0x0010u,
    ZF = 0x0040u, SF = 0x0080u, OF = 0x0800u,
    ARITH = CF | PF | AF | ZF | SF | OF
};

static uint8_t even_parity(uint8_t value)
{
    unsigned odd = 0u;
    while (value != 0u) { odd ^= value & 1u; value >>= 1; }
    return (uint8_t)(odd == 0u);
}

static uint16_t shl_flags16(uint16_t flags, uint16_t old, uint16_t result)
{
    unsigned carry = (old & 0x8000u) != 0u;
    flags &= (uint16_t)~ARITH;
    if (carry) flags |= CF;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (((result & 0x8000u) != 0u) != carry) flags |= OF;
    return flags;
}

static uint16_t sub_flags8(uint16_t flags, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    flags &= (uint16_t)~ARITH;
    if (left < right) flags |= CF;
    if (even_parity(result)) flags |= PF;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x80u) != 0u) flags |= SF;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u) flags |= OF;
    return flags;
}

static size_t physical(uint16_t segment, uint16_t offset)
{
    return ((size_t)segment << 4) + offset;
}

static uint16_t word_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t address = physical(segment, offset);
    return (uint16_t)(ram[address] | ((uint16_t)ram[address + 1u] << 8));
}

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t value)
{
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
}

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_2981(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    uint16_t old_di;
    uint8_t right;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x2981u ||
        cpu->ax != 0xFFFFu || cpu->bx != 0x3E50u || cpu->cx != 0u ||
        cpu->dx != 0u || cpu->si != 0x2612u || cpu->di != 0x007Cu ||
        cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x1D16u ||
        cpu->ss != 0x1D16u || cpu->flags != 0x7286u)
        return PR_R464_2632_UNSUPPORTED_PATH;

    observe(0x2981u, ram, cpu, observer, context);
    cpu->ax = word_at(ram, cpu->ss, (uint16_t)(cpu->bp - 0x20u));
    observe(0x2984u, ram, cpu, observer, context);
    cpu->di = word_at(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1Eu));
    observe(0x2987u, ram, cpu, observer, context);
    old_di = cpu->di;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->flags = shl_flags16(cpu->flags, old_di, cpu->di);
    observe(0x2989u, ram, cpu, observer, context);
    put_word(ram, cpu->ds, (uint16_t)(cpu->di + 0x4AD0u), cpu->ax);
    observe(0x298Du, ram, cpu, observer, context);
    cpu->ax = word_at(ram, cpu->ss, (uint16_t)(cpu->bp - 0x16u));
    observe(0x2990u, ram, cpu, observer, context);
    cpu->di = word_at(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1Eu));
    observe(0x2993u, ram, cpu, observer, context);
    old_di = cpu->di;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->flags = shl_flags16(cpu->flags, old_di, cpu->di);
    observe(0x2995u, ram, cpu, observer, context);
    put_word(ram, cpu->ds, (uint16_t)(cpu->di + 0x4C20u), cpu->ax);
    observe(0x2999u, ram, cpu, observer, context);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
        ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x12u))]);
    observe(0x299Cu, ram, cpu, observer, context);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x24u))] = (uint8_t)cpu->ax;
    observe(0x299Fu, ram, cpu, observer, context);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
    observe(0x29A1u, ram, cpu, observer, context);
    right = ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x24u))];
    cpu->flags = sub_flags8(cpu->flags, (uint8_t)cpu->ax, right);
    observe(0x29A4u, ram, cpu, observer, context);
    observe((cpu->flags & ZF) == 0u &&
            ((cpu->flags & SF) != 0u) == ((cpu->flags & OF) != 0u)
                ? 0x29D7u : 0x29A6u, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
