/* SYSTEM FillChar 0AF8:16A8 on a memory bus (raw CPU); docs/re/RES_0709_RECT_FILL_00B7_DESIGN.md. */
#include "poolrad/recovered/system_fillchar_bus_16a8_cpu.h"

#define MASK (PR_BUS_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16(pr_bus *bus, size_t at)
{
    return (uint16_t)(pr_bus_read8(bus, at) | ((uint16_t)pr_bus_read8(bus, (at + 1u) & MASK) << 8));
}

pr_system_leaf_status pr_system_fillchar_bus_16a8(pr_bus *bus, pr_system_leaf_cpu *cpu)
{
    size_t at;
    if (bus == NULL || bus->ram == NULL || cpu == NULL)
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->bx = cpu->sp;                                                    /* 16A8 MOV BX,SP; LES DI,SS:[BX+8] */
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 8u));
    cpu->di = read16(bus, at);
    cpu->es = read16(bus, (at + 2u) & MASK);
    cpu->cx = read16(bus, physical(cpu->ss, (uint16_t)(cpu->bx + 6u)));   /* 16AE MOV CX,SS:[BX+6]; MOV AL,SS:[BX+4] */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | pr_bus_read8(bus, physical(cpu->ss, (uint16_t)(cpu->bx + 4u))));
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 16B6 CLD; REP STOSB */
    while (cpu->cx != 0u) {
        pr_bus_write8(bus, physical(cpu->es, cpu->di), (uint8_t)cpu->ax);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    at = physical(cpu->ss, cpu->sp);                                      /* 16B9 RETF 8 */
    cpu->ip = read16(bus, at);
    cpu->cs = read16(bus, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 12u);
    return PR_SYSTEM_LEAF_RETURNED;
}
