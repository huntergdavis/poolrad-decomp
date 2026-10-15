#include "poolrad/recovered/resident_464_2632_fourth_select.h"

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

static uint16_t dec_flags16(uint16_t flags, uint16_t value)
{
    uint16_t result = (uint16_t)(value - 1u);
    flags &= (uint16_t)~(ARITH & ~CF);
    if (even_parity((uint8_t)result)) flags |= PF;
    if (((value ^ 1u ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (value == 0x8000u) flags |= OF;
    return flags;
}

static uint16_t cmp_flags8(uint16_t flags, uint8_t left, uint8_t right)
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

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_select(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    size_t stack_base, source, target, selector;
    uint16_t value;
    uint8_t choice;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x277Du ||
        cpu->ax != 168u || cpu->bx != 0u || cpu->cx != 104u ||
        cpu->dx != 0u || cpu->si != 0x2612u || cpu->di != 8u ||
        cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u || cpu->ss != 0x1D16u)
        return PR_R464_2632_UNSUPPORTED_PATH;

    stack_base = (size_t)cpu->ss << 4;
    source = stack_base + (uint16_t)(cpu->bp + 0x10u);
    target = stack_base + (uint16_t)(cpu->bp - 0x14u);
    selector = ((size_t)cpu->ds << 4) + 0x4A1Eu;
    value = (uint16_t)(ram[source] | ((uint16_t)ram[source + 1u] << 8));
    cpu->ax = value;
    observe(0x2780u, ram, cpu, observer, context);
    cpu->flags = dec_flags16(cpu->flags, value);
    cpu->ax = (uint16_t)(value - 1u);
    observe(0x2781u, ram, cpu, observer, context);
    ram[target] = (uint8_t)cpu->ax;
    ram[target + 1u] = (uint8_t)(cpu->ax >> 8);
    observe(0x2784u, ram, cpu, observer, context);
    choice = ram[selector];
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | choice);
    observe(0x2787u, ram, cpu, observer, context);
    cpu->flags = cmp_flags8(cpu->flags, choice, 0u);
    observe(0x2789u, ram, cpu, observer, context);
    if (choice == 0u) {
        observe(0x2792u, ram, cpu, observer, context);
        return PR_R464_2632_OK;
    }
    observe(0x278Bu, ram, cpu, observer, context);
    cpu->flags = cmp_flags8(cpu->flags, choice, 2u);
    observe(0x278Du, ram, cpu, observer, context);
    if (choice == 2u) {
        observe(0x2792u, ram, cpu, observer, context);
        return PR_R464_2632_OK;
    }
    observe(0x278Fu, ram, cpu, observer, context);
    observe(0x28D9u, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
