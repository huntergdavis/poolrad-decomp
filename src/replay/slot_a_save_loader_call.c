#include "poolrad/replay/slot_a_save_loader_call.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v)
{ size_t p; c->sp=(uint16_t)(c->sp-2u); p=phys(c->ss,c->sp); r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_save_loader_call(uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu)
{
    size_t pos,p1,p2; uint16_t seg; unsigned i;
    if (!ram || !code || !cpu || size!=0x100000u || code_size!=5u || cpu->ip!=0x029bu) return 1;
    pos=phys(cpu->cs,0x029bu);
    if(pos>size-5u || memcmp(ram+pos,code,5u) || code[0]!=0x9au || code[1]!=0x2au || code[2]!=0u) return 1;
    seg=(uint16_t)(code[3]|((uint16_t)code[4]<<8));
    p1=phys(cpu->ss,(uint16_t)(cpu->sp-2u)); p2=phys(cpu->ss,(uint16_t)(cpu->sp-4u));
    for(i=0;i<5u;i++) if(pos+i==p1 || pos+i==((p1+1u)&0xfffffu) || pos+i==p2 || pos+i==((p2+1u)&0xfffffu)) return 1;
    push(ram,cpu,cpu->cs); push(ram,cpu,0x02a0u); cpu->cs=seg; cpu->ip=0x002au; return 0;
}
