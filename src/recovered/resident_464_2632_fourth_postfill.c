#include "poolrad/recovered/resident_464_2632_fourth_postfill.h"

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

static uint16_t logic_flags8(uint16_t flags, uint8_t result)
{
    flags &= (uint16_t)~ARITH;
    if (even_parity(result)) flags |= PF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x80u) != 0u) flags |= SF;
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

static uint16_t inc_flags8(uint16_t flags, uint8_t old)
{
    uint8_t result = (uint8_t)(old + 1u);
    flags &= (uint16_t)~(ARITH & ~CF);
    if (even_parity(result)) flags |= PF;
    if ((old & 0x0Fu) == 0x0Fu) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x80u) != 0u) flags |= SF;
    if (old == 0x7Fu) flags |= OF;
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

static size_t physical(uint16_t segment, uint16_t offset)
{
    return ((size_t)segment << 4) + offset;
}

static uint8_t local_byte(const uint8_t *ram, const pr_r464_2632_cpu *cpu,
                          int delta)
{
    return ram[physical(cpu->ss, (uint16_t)(cpu->bp + delta))];
}

static void set_local_byte(uint8_t *ram, const pr_r464_2632_cpu *cpu,
                           int delta, uint8_t value)
{
    ram[physical(cpu->ss, (uint16_t)(cpu->bp + delta))] = value;
}

static uint16_t local_word(const uint8_t *ram, const pr_r464_2632_cpu *cpu,
                           int delta)
{
    size_t address = physical(cpu->ss, (uint16_t)(cpu->bp + delta));
    return (uint16_t)(ram[address] | ((uint16_t)ram[address + 1u] << 8));
}

static void set_local_word(uint8_t *ram, const pr_r464_2632_cpu *cpu,
                           int delta, uint16_t value)
{
    size_t address = physical(cpu->ss, (uint16_t)(cpu->bp + delta));
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
}

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_postfill(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    uint8_t index, masked;
    uint16_t count;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x2921u ||
        cpu->ax != 0u || cpu->bx != 0x3E50u || cpu->cx != 0u ||
        cpu->dx != 0u || cpu->si != 0x2612u ||
        cpu->di != 0x3E68u || cpu->bp != 0x3E80u ||
        cpu->sp != 0x3E5Cu || cpu->ds != 0x14A0u ||
        cpu->es != 0x1D16u || cpu->ss != 0x1D16u ||
        cpu->flags != 0x7202u)
        return PR_R464_2632_UNSUPPORTED_PATH;

    observe(0x2921u, ram, cpu, observer, context);
    set_local_byte(ram, cpu, -0x21, 0u);
    observe(0x2925u, ram, cpu, observer, context);
    observe(0x292Au, ram, cpu, observer, context);
    for (index = 0u; index < 4u; ++index) {
        if (index != 0u) {
            uint8_t old;
            observe(0x2927u, ram, cpu, observer, context);
            old = local_byte(ram, cpu, -0x21);
            set_local_byte(ram, cpu, -0x21, (uint8_t)(old + 1u));
            cpu->flags = inc_flags8(cpu->flags, old);
            observe(0x292Au, ram, cpu, observer, context);
        }
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                             local_byte(ram, cpu, -0x21));
        observe(0x292Du, ram, cpu, observer, context);
        cpu->ax &= 0x00FFu;
        cpu->flags = logic_flags8(cpu->flags, 0u);
        observe(0x292Fu, ram, cpu, observer, context);
        cpu->di = cpu->ax;
        observe(0x2931u, ram, cpu, observer, context);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
            ram[physical(cpu->ds, (uint16_t)(cpu->di + 0x25F6u))]);
        observe(0x2935u, ram, cpu, observer, context);
        masked = (uint8_t)cpu->ax & local_byte(ram, cpu, +0x0E);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | masked);
        cpu->flags = logic_flags8(cpu->flags, masked);
        observe(0x2938u, ram, cpu, observer, context);
        cpu->flags = logic_flags8(cpu->flags, masked);
        observe(0x293Au, ram, cpu, observer, context);
        if (masked != 0u) {
            observe(0x293Cu, ram, cpu, observer, context);
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                                 local_byte(ram, cpu, -0x21));
            observe(0x293Fu, ram, cpu, observer, context);
            cpu->ax &= 0x00FFu;
            cpu->flags = logic_flags8(cpu->flags, 0u);
            observe(0x2941u, ram, cpu, observer, context);
            cpu->di = cpu->ax;
            observe(0x2943u, ram, cpu, observer, context);
            set_local_byte(ram, cpu, (int)cpu->di - 0x1C, 0xFFu);
        }
        observe(0x2947u, ram, cpu, observer, context);
        cpu->flags = sub_flags8(cpu->flags,
                                local_byte(ram, cpu, -0x21), 3u);
        observe(0x294Bu, ram, cpu, observer, context);
    }
    observe(0x294Du, ram, cpu, observer, context);
    cpu->ax = local_word(ram, cpu, -0x14);
    observe(0x2950u, ram, cpu, observer, context);
    set_local_word(ram, cpu, -0x23, cpu->ax);
    observe(0x2953u, ram, cpu, observer, context);
    cpu->ax = 0u;
    cpu->flags = logic_flags16(cpu->flags, 0u);
    observe(0x2955u, ram, cpu, observer, context);
    count = local_word(ram, cpu, -0x23);
    cpu->flags = sub_flags16(cpu->flags, cpu->ax, count);
    observe(0x2958u, ram, cpu, observer, context);
    observe(0x295Du, ram, cpu, observer, context);
    set_local_word(ram, cpu, -0x18, cpu->ax);
    observe(0x2960u, ram, cpu, observer, context);
    observe(0x2965u, ram, cpu, observer, context);
    cpu->di = local_word(ram, cpu, -0x1E);
    observe(0x2968u, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
