#include "poolrad/replay/slot_a_synthetic_and_arithmetic.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
int pr_slot_a_synthetic_and_arithmetic(uint8_t *r,size_t n,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p,d,v,x,y,latch;unsigned i;uint8_t result,parity;uint16_t flags;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0e20||c->bp<7u||c->ax!=0x6e82u)return 1;
    p=phys(c->cs,0x0e20);d=phys(c->ss,(uint16_t)(c->bp-2u));v=phys(c->ss,(uint16_t)(c->bp-7u));x=phys(c->ss,(uint16_t)(c->bp-6u));y=phys(c->ss,(uint16_t)(c->bp-4u));latch=phys(c->ds,0x6f7f);
    if(p>n-30u||memcmp(r+p,code,30u)||r[latch]!=0x2fu||r[x]!=0x7fu||r[(x+1u)&0xfffffu]!=0u||r[y]!=synthetic||r[(y+1u)&0xfffffu]!=0u)return 1;
    if(d==latch||((d+1u)&0xfffffu)==latch||v==latch)return 1;
    for(i=0;i<30;i++)if(d==p+i||((d+1u)&0xfffffu)==p+i||v==p+i)return 1;
    r[d]=0x82;r[(d+1u)&0xfffffu]=0x6e;result=(uint8_t)(0x7fu&synthetic);r[v]=result;c->ax=result;
    parity=result;parity^=(uint8_t)(parity>>4);parity^=(uint8_t)(parity>>2);parity^=(uint8_t)(parity>>1);flags=(uint16_t)(((parity&1u)==0u?4u:0u)|(result==0?0x40u:0u));c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|flags);c->ip=0x0e3e;return 0;
}
