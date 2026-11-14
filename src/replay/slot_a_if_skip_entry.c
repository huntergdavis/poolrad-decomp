#include "poolrad/replay/slot_a_if_skip_entry.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v) { size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_if_skip_entry(uint8_t *ram,size_t size,const uint8_t *call,const uint8_t *stub,pr_u1c_3171_cpu *cpu)
{
    size_t p,q,a;uint16_t seg,target;unsigned i,j;
    if(!ram||size!=0x100000u||!call||!stub||!cpu||cpu->ip!=0x0bc1u||cpu->sp<4u)return 1;
    p=phys(cpu->cs,0x0bc1);if(p>size-5u||memcmp(ram+p,call,5u)||call[0]!=0x9au||call[1]!=0x8eu||call[2]!=0u)return 1;
    seg=(uint16_t)(call[3]|((uint16_t)call[4]<<8));q=phys(seg,0x008e);if(q>size-5u||memcmp(ram+q,stub,5u)||stub[0]!=0xeau||stub[1]!=0xa6u||stub[2]!=0x1eu)return 1;
    target=(uint16_t)(stub[3]|((uint16_t)stub[4]<<8));
    for(i=2u;i<=4u;i+=2u){a=phys(cpu->ss,(uint16_t)(cpu->sp-i));for(j=0;j<5u;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j||a==q+j||((a+1u)&0xfffffu)==q+j)return 1;}
    push(ram,cpu,cpu->cs);push(ram,cpu,0x0bc6u);cpu->cs=target;cpu->ip=0x1ea6u;return 0;
}
