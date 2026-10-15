#include "poolrad/recovered/resident_464_2632_fourth_29ab.h"

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

static uint16_t inc_flags8(uint16_t flags, uint8_t old, uint8_t result)
{
    flags &= (uint16_t)~(ARITH & ~CF);
    if (even_parity(result)) flags |= PF;
    if (((old ^ 1u ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x80u) != 0u) flags |= SF;
    if (old == 0x7Fu) flags |= OF;
    return flags;
}

static uint16_t add_flags16(uint16_t flags, uint16_t left,
                            uint16_t right, uint16_t result)
{
    uint32_t sum = (uint32_t)left + right;
    flags &= (uint16_t)~ARITH;
    if (sum > 0xFFFFu) flags |= CF;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (((~(left ^ right)) & (left ^ result) & 0x8000u) != 0u)
        flags |= OF;
    return flags;
}

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

static void push(uint8_t *ram, pr_r464_2632_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, cpu->ss, cpu->sp, value);
}

pr_r464_2632_status pr_r464_2632_fourth_29ab(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    uint16_t left;
    uint8_t old, result;
    size_t global;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x29ABu ||
        cpu->ax != 1u || cpu->bx != 0x3E4Eu || cpu->cx != 0u ||
        cpu->dx != 0x14A0u || cpu->si != 0x3E68u ||
        cpu->di != 0x1483u || cpu->bp != 0x3E80u ||
        cpu->sp != 0x3E5Cu || cpu->ds != 0x14A0u ||
        cpu->es != 0x3DE4u || cpu->ss != 0x1D16u ||
        cpu->flags != 0x7297u)
        return PR_R464_2632_UNSUPPORTED_PATH;

    observe(0x29ABu, ram, cpu, observer, context);
    global = physical(cpu->ds, 0x70C7u);
    old = ram[global];
    result = (uint8_t)(old + 1u);
    ram[global] = result;
    cpu->flags = inc_flags8(cpu->flags, old, result);
    observe(0x29AFu, ram, cpu, observer, context);
    cpu->di = (uint16_t)(cpu->bp - 0x1Cu);
    observe(0x29B2u, ram, cpu, observer, context);
    push(ram, cpu, cpu->ss);
    observe(0x29B3u, ram, cpu, observer, context);
    push(ram, cpu, cpu->di);
    observe(0x29B4u, ram, cpu, observer, context);
    cpu->ax = word_at(ram, cpu->ss, (uint16_t)(cpu->bp - 0x16u));
    observe(0x29B7u, ram, cpu, observer, context);
    cpu->di = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->es = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    observe(0x29BAu, ram, cpu, observer, context);
    left = cpu->di;
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
    cpu->flags = add_flags16(cpu->flags, left, cpu->ax, cpu->di);
    observe(0x29BCu, ram, cpu, observer, context);
    left = cpu->di;
    cpu->di = (uint16_t)(cpu->di + 0x17u);
    cpu->flags = add_flags16(cpu->flags, left, 0x17u, cpu->di);
    observe(0x29C0u, ram, cpu, observer, context);
    push(ram, cpu, cpu->es);
    observe(0x29C1u, ram, cpu, observer, context);
    push(ram, cpu, cpu->di);
    observe(0x29C2u, ram, cpu, observer, context);
    cpu->ax = 4u;
    observe(0x29C5u, ram, cpu, observer, context);
    push(ram, cpu, cpu->ax);
    observe(0x29C6u, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
