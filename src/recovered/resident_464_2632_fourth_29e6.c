#include "poolrad/recovered/resident_464_2632_fourth_29e6.h"

enum {
    CF = 0x0001u, PF = 0x0004u, AF = 0x0010u,
    ZF = 0x0040u, SF = 0x0080u, OF = 0x0800u,
    ARITH = CF | PF | AF | ZF | SF | OF
};

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

static uint8_t even_parity(uint8_t value)
{
    unsigned odd = 0u;
    while (value != 0u) { odd ^= value & 1u; value >>= 1; }
    return (uint8_t)(odd == 0u);
}

static uint16_t dec_flags16(uint16_t flags, uint16_t old, uint16_t result)
{
    flags &= (uint16_t)~(ARITH & ~CF);
    if (even_parity((uint8_t)result)) flags |= PF;
    if (((old ^ 1u ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (old == 0x8000u) flags |= OF;
    return flags;
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

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_29e6(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    uint16_t old;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x29E6u ||
        cpu->ax != 0x148Bu || cpu->bx != 0x3E4Eu || cpu->cx != 0u ||
        cpu->dx != 0x14A0u || cpu->si != 0x3E68u ||
        cpu->di != 0x007Cu || cpu->bp != 0x3E80u ||
        cpu->sp != 0x3E5Cu || cpu->ds != 0x14A0u ||
        cpu->es != 0x3DE4u || cpu->ss != 0x1D16u ||
        cpu->flags != 0x7206u)
        return PR_R464_2632_UNSUPPORTED_PATH;

    observe(0x29E6u, ram, cpu, observer, context);
    cpu->ax = word_at(ram, cpu->ss, (uint16_t)(cpu->bp - 0x16u));
    observe(0x29E9u, ram, cpu, observer, context);
    old = cpu->ax;
    cpu->ax = (uint16_t)(cpu->ax - 1u);
    cpu->flags = dec_flags16(cpu->flags, old, cpu->ax);
    observe(0x29EAu, ram, cpu, observer, context);
    cpu->di = word_at(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1Eu));
    observe(0x29EDu, ram, cpu, observer, context);
    old = cpu->di;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->flags = shl_flags16(cpu->flags, old, cpu->di);
    observe(0x29EFu, ram, cpu, observer, context);
    put_word(ram, cpu->ds, (uint16_t)(cpu->di + 0x4D70u), cpu->ax);
    observe(0x29F3u, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
