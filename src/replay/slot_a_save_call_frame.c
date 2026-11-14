#include "poolrad/replay/slot_a_save_call_frame.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r, pr_u1c_3171_cpu *c, uint16_t value)
{
    size_t p; c->sp=(uint16_t)(c->sp-2u); p=phys(c->ss,c->sp);
    r[p]=(uint8_t)value; r[(p+1u)&0xfffffu]=(uint8_t)(value>>8);
}
int pr_slot_a_save_call_frame(uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu)
{
    size_t pos;
    if (!ram || !code || !cpu || size!=0x100000u || code_size!=4u || cpu->ip!=0x2f85u) return 1;
    pos=phys(cpu->cs,0x2f85u);
    if (pos>size-4u || memcmp(ram+pos,code,4u)) return 1;
    push(ram,cpu,cpu->cs); push(ram,cpu,0x2f89u); cpu->ip=0x0292u; return 0;
}
