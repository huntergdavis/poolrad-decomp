#include "poolrad/recovered/resident_464_fourth_branch_275b.h"

#include <string.h>

enum {
    CF = 0x0001u, PF = 0x0004u, AF = 0x0010u,
    ZF = 0x0040u, SF = 0x0080u, OF = 0x0800u,
    ARITH = CF | PF | AF | ZF | SF | OF
};

static uint8_t even_parity(uint8_t value)
{
    unsigned bits = 0;
    while (value != 0u) {
        bits ^= value & 1u;
        value >>= 1;
    }
    return (uint8_t)(bits == 0u);
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

pr_r464_fourth_status pr_r464_fourth_branch_275b(
    const uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    static const uint8_t code[] = {0x7c, 0x06, 0x7f, 0x1e,
                                   0x3b, 0xc1, 0x73, 0x1a};
    const size_t code_at = (0x0C88u << 4) + 0x275Bu;
    uint16_t flags;

    if (ram == NULL || cpu == NULL || ram_size < PR_R464_FOURTH_RAM_BYTES)
        return PR_R464_FOURTH_INVALID_HOST;
    /* These guards select only the observed successors before any effects. */
    flags = cpu->flags;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x275Bu ||
        memcmp(ram + code_at, code, sizeof code) != 0 ||
        (flags & ZF) == 0u ||
        ((flags & SF) != 0u) != ((flags & OF) != 0u) ||
        cpu->ax < cpu->cx)
        return PR_R464_FOURTH_UNSUPPORTED_PATH;

    observe(0x275Bu, ram, cpu, observer, context);
    observe(0x275Du, ram, cpu, observer, context);
    observe(0x275Fu, ram, cpu, observer, context);
    cpu->flags = cmp_flags16(flags, cpu->ax, cpu->cx);
    observe(0x2761u, ram, cpu, observer, context);
    observe(0x277Du, ram, cpu, observer, context);
    return PR_R464_FOURTH_OK;
}
