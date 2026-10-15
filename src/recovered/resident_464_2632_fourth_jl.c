#include "poolrad/recovered/resident_464_2632_fourth_jl.h"

pr_r464_2632_status pr_r464_2632_fourth_jl(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    int sign, overflow;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x275Bu ||
        cpu->ax != 168u || cpu->bx != 0u || cpu->cx != 104u ||
        cpu->dx != 0u || cpu->si != 0x2612u || cpu->di != 8u ||
        cpu->bp != 0x3E80u || cpu->sp != 0x3E5Cu ||
        cpu->ds != 0x14A0u || cpu->es != 0x3DE4u || cpu->ss != 0x1D16u)
        return PR_R464_2632_UNSUPPORTED_PATH;
    sign = (cpu->flags & 0x0080u) != 0u;
    overflow = (cpu->flags & 0x0800u) != 0u;
    cpu->ip = sign != overflow ? 0x2763u : 0x275Du;
    if (observer != NULL) observer(cpu->ip, ram, cpu, context);
    return PR_R464_2632_OK;
}
