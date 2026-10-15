#include "poolrad/recovered/resident_464_2632_fourth_jg.h"

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

static uint16_t cmp_flags16(uint16_t flags, uint16_t left, uint16_t right)
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

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_jg(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    int sign, overflow, zero;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x275Du ||
        cpu->bx != 0u || cpu->dx != 0u || cpu->si != 0x2612u ||
        cpu->di != 8u || cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u || cpu->ss != 0x1D16u)
        return PR_R464_2632_UNSUPPORTED_PATH;
    sign = (cpu->flags & SF) != 0u;
    overflow = (cpu->flags & OF) != 0u;
    zero = (cpu->flags & ZF) != 0u;
    if (!zero && sign == overflow) {
        observe(0x277Du, ram, cpu, observer, context);
        return PR_R464_2632_OK;
    }
    observe(0x275Fu, ram, cpu, observer, context);
    cpu->flags = cmp_flags16(cpu->flags, cpu->ax, cpu->cx);
    observe(0x2761u, ram, cpu, observer, context);
    observe((cpu->flags & CF) == 0u ? 0x277Du : 0x2763u,
            ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
