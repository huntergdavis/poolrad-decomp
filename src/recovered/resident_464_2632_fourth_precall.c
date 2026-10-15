#include "poolrad/recovered/resident_464_2632_fourth_precall.h"

enum {
    CF = 0x0001u, PF = 0x0004u, AF = 0x0010u,
    ZF = 0x0040u, SF = 0x0080u, OF = 0x0800u,
    ARITH = CF | PF | AF | ZF | SF | OF
};

static uint8_t even_parity(uint8_t value)
{
    unsigned odd = 0;
    while (value != 0u) { odd ^= value & 1u; value >>= 1; }
    return (uint8_t)(odd == 0u);
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

/* AF is undefined by SHL and remains as supplied by the incoming flags.
 * Both saved shifts have AF clear. */
static uint16_t shl_flags16(uint16_t flags, uint16_t old, uint16_t result)
{
    unsigned carry = (old & 0x8000u) != 0u;
    flags &= (uint16_t)~(CF | PF | ZF | SF | OF);
    if (carry) flags |= CF;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (((result & 0x8000u) != 0u) != carry) flags |= OF;
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

static uint16_t local(const uint8_t *ram, const pr_r464_2632_cpu *cpu,
                      int delta)
{
    return word_at(ram, cpu->ss, (uint16_t)(cpu->bp + delta));
}

static void set_local(uint8_t *ram, const pr_r464_2632_cpu *cpu,
                      int delta, uint16_t value)
{
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp + delta), value);
}

static void push(uint8_t *ram, pr_r464_2632_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, cpu->ss, cpu->sp, value);
}

static void shift_ax(uint16_t *flags, uint16_t *ax)
{
    uint16_t old = *ax;
    *ax = (uint16_t)(old << 1);
    *flags = shl_flags16(*flags, old, *ax);
}

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_precall(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    uint8_t selector;
    uint16_t right;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x28D9u ||
        cpu->bx != 0u || cpu->cx != 104u || cpu->dx != 0u ||
        cpu->si != 0x2612u || cpu->di != 8u ||
        cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u || cpu->ss != 0x1D16u)
        return PR_R464_2632_UNSUPPORTED_PATH;
    selector = (uint8_t)cpu->ax;
    if (selector == 1u &&
        (local(ram, cpu, -0x0E) != 2u ||
         local(ram, cpu, +0x12) != 11u ||
         local(ram, cpu, -0x0A) != 0u ||
         local(ram, cpu, -0x0C) != 0u ||
         local(ram, cpu, -0x10) != 8u ||
         local(ram, cpu, -0x16) != 0x0516u))
        return PR_R464_2632_UNSUPPORTED_PATH;

    cpu->flags = sub_flags8(cpu->flags, selector, 1u);
    observe(0x28DBu, ram, cpu, observer, context);
    if (selector != 1u) {
        observe(0x28DDu, ram, cpu, observer, context);
        return PR_R464_2632_OK;
    }
    observe(0x28E0u, ram, cpu, observer, context);

    cpu->ax = local(ram, cpu, -0x0E);
    observe(0x28E3u, ram, cpu, observer, context);
    set_local(ram, cpu, -0x20, cpu->ax);
    observe(0x28E6u, ram, cpu, observer, context);
    cpu->ax = local(ram, cpu, +0x12);
    observe(0x28E9u, ram, cpu, observer, context);
    right = local(ram, cpu, -0x0A);
    cpu->flags = sub_flags16(cpu->flags, cpu->ax, right);
    cpu->ax = (uint16_t)(cpu->ax - right);
    observe(0x28ECu, ram, cpu, observer, context);
    right = local(ram, cpu, -0x0C);
    cpu->flags = sub_flags16(cpu->flags, cpu->ax, right);
    cpu->ax = (uint16_t)(cpu->ax - right);
    observe(0x28EFu, ram, cpu, observer, context);
    set_local(ram, cpu, -0x12, cpu->ax);
    observe(0x28F2u, ram, cpu, observer, context);
    cpu->ax = local(ram, cpu, -0x0E);
    observe(0x28F5u, ram, cpu, observer, context);
    shift_ax(&cpu->flags, &cpu->ax);
    observe(0x28F7u, ram, cpu, observer, context);
    shift_ax(&cpu->flags, &cpu->ax);
    observe(0x28F9u, ram, cpu, observer, context);
    set_local(ram, cpu, -0x0E, cpu->ax);
    observe(0x28FCu, ram, cpu, observer, context);
    cpu->ax = local(ram, cpu, -0x10);
    observe(0x28FFu, ram, cpu, observer, context);
    shift_ax(&cpu->flags, &cpu->ax);
    observe(0x2901u, ram, cpu, observer, context);
    shift_ax(&cpu->flags, &cpu->ax);
    observe(0x2903u, ram, cpu, observer, context);
    set_local(ram, cpu, -0x10, cpu->ax);
    observe(0x2906u, ram, cpu, observer, context);
    cpu->ax = local(ram, cpu, -0x16);
    observe(0x2909u, ram, cpu, observer, context);
    shift_ax(&cpu->flags, &cpu->ax);
    observe(0x290Bu, ram, cpu, observer, context);
    shift_ax(&cpu->flags, &cpu->ax);
    observe(0x290Du, ram, cpu, observer, context);
    set_local(ram, cpu, -0x16, cpu->ax);
    observe(0x2910u, ram, cpu, observer, context);
    cpu->di = (uint16_t)(cpu->bp - 0x1Cu);
    observe(0x2913u, ram, cpu, observer, context);
    push(ram, cpu, cpu->ss);
    observe(0x2914u, ram, cpu, observer, context);
    push(ram, cpu, cpu->di);
    observe(0x2915u, ram, cpu, observer, context);
    cpu->ax = 4u;
    observe(0x2918u, ram, cpu, observer, context);
    push(ram, cpu, cpu->ax);
    observe(0x2919u, ram, cpu, observer, context);
    cpu->ax &= 0xFF00u;
    observe(0x291Bu, ram, cpu, observer, context);
    push(ram, cpu, cpu->ax);
    observe(0x291Cu, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
