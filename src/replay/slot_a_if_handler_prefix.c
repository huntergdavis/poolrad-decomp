#include "poolrad/replay/slot_a_if_handler_prefix.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static uint16_t rd(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8)); }
static void wr(uint8_t *r,size_t p,uint16_t v) { r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_if_handler_prefix(uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t p,stack,ds,pc;uint16_t v;
    if(!ram||size!=0x100000u||!code||code_size!=111u||!cpu||cpu->ip!=0x0b52u||cpu->sp<2u)return 1;
    p=phys(cpu->cs,0x0b52);if(p>size-111u||memcmp(ram+p,code,111u))return 1;
    ds=phys(cpu->ds,0);stack=phys(cpu->ss,(uint16_t)(cpu->sp-2u));
    if(((stack-ds)&0xfffffu)<0x10000u||((((stack+1u)&0xfffffu)-ds)&0xfffffu)<0x10000u)return 1;
    if(ram[phys(cpu->ds,0x6f7fu)]!=0x1bu||ram[phys(cpu->ds,0x6f7du)]!=0u)return 1;
    pc=phys(cpu->ds,0x49edu);v=rd(ram,pc);cpu->sp=(uint16_t)(cpu->sp-2u);wr(ram,stack,cpu->bp);cpu->bp=cpu->sp;wr(ram,pc,(uint16_t)(v+1u));
    cpu->ax=(uint16_t)((cpu->ax&0xff00u)|0x1bu);cpu->flags=(uint16_t)((cpu->flags&(uint16_t)~0x08d5u)|0x0044u);cpu->ip=0x0bc1u;return 0;
}
