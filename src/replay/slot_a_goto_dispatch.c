#include "poolrad/replay/slot_a_goto_dispatch.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v) { size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
uint16_t pr_slot_a_goto_dispatch(uint8_t *ram,size_t size,const uint8_t *selector,const uint8_t *call,pr_u1c_3171_cpu *cpu)
{
    size_t p,a;uint16_t rel;unsigned i,j;
    if(!ram||size!=0x100000u||!selector||!call||!cpu||cpu->ip!=0x2f3eu||(uint8_t)cpu->ax!=1u||cpu->sp<4u)return 0x2f3eu;
    p=phys(cpu->cs,0x2f3e);if(p>size-8u||memcmp(ram+p,selector,4u)||ram[p+4u]!=call[0]||call[0]!=0x0eu)return 0x2f3eu;
    for(i=2u;i<=4u;i+=2u){a=phys(cpu->ss,(uint16_t)(cpu->sp-i));for(j=0;j<8u;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j)return 0x2f3eu;}
    cpu->flags=(uint16_t)((cpu->flags&(uint16_t)~0x08d5u)|0x0044u);push(ram,cpu,cpu->cs);cpu->ip=0x2f43u;
    rel=(uint16_t)(ram[p+6u]|((uint16_t)ram[p+7u]<<8));if(memcmp(ram+p+5u,call+1u,3u)||ram[p+5u]!=0xe8u||(uint16_t)(0x2f46u+rel)!=0x00f4u)return cpu->ip;
    push(ram,cpu,0x2f46u);cpu->ip=0x00f4u;return cpu->ip;
}
