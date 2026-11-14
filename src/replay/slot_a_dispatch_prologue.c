#include "poolrad/replay/slot_a_dispatch_prologue.h"
#include <string.h>
static size_t phys(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & 0xfffffu; }
int pr_slot_a_dispatch_prologue(uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu)
{
    size_t pos, stack;
    if (!ram || !code || !cpu || size != 0x100000u || code_size != 6u || cpu->ip != 0x2f2du) return 1;
    pos=phys(cpu->cs,0x2f2du);
    if (pos > size-6u || memcmp(ram+pos,code,6u)) return 1;
    cpu->sp=(uint16_t)(cpu->sp-2u); stack=phys(cpu->ss,cpu->sp);
    ram[stack]=(uint8_t)cpu->bp; ram[(stack+1u)&0xfffffu]=(uint8_t)(cpu->bp>>8);
    cpu->bp=cpu->sp;
    cpu->ax=(uint16_t)((cpu->ax&0xff00u)|ram[phys(cpu->ds,0x6f7fu)]);
    cpu->ip=0x2f33u; return 0;
}
