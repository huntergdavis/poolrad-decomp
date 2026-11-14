#include "poolrad/replay/slot_a_save_handler_prefix.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r, pr_u1c_3171_cpu *c, uint16_t v)
{ size_t p; c->sp=(uint16_t)(c->sp-2u); p=phys(c->ss,c->sp); r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_save_handler_prefix(uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu)
{
    size_t pos; uint16_t left,result,flags; uint8_t parity;
    if (!ram || !code || !cpu || size!=0x100000u || code_size!=9u || cpu->ip!=0x0292u) return 1;
    pos=phys(cpu->cs,0x0292u); if(pos>size-9u || memcmp(ram+pos,code,9u)) return 1;
    push(ram,cpu,cpu->bp); cpu->bp=cpu->sp; left=cpu->sp; result=(uint16_t)(left-4u);
    parity=(uint8_t)result; parity^=(uint8_t)(parity>>4); parity^=(uint8_t)(parity>>2); parity^=(uint8_t)(parity>>1);
    flags=(uint16_t)((left<4u?1u:0u)|((parity&1u)==0u?4u:0u)|(((left^4u^result)&0x10u)?0x10u:0u)|(result==0?0x40u:0u)|(result&0x8000u?0x80u:0u)|(((left^4u)&(left^result)&0x8000u)?0x800u:0u));
    cpu->flags=(uint16_t)((cpu->flags & (uint16_t)~0x08d5u)|flags); cpu->sp=result;
    cpu->ax=(uint16_t)((cpu->ax&0xff00u)|2u); push(ram,cpu,cpu->ax); cpu->ip=0x029bu; return 0;
}
