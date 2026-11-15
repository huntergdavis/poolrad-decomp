#include "poolrad/replay/slot_a_synthetic_reader_return.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static uint16_t get(const uint8_t *r,size_t p){return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8));}
int pr_slot_a_synthetic_reader_return(uint8_t *r,size_t n,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p,a;uint16_t bp,ret,seg;unsigned i;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0191||c->bp<2u||c->bp>0xfff8u||c->ax!=synthetic)return 1;
    p=phys(c->cs,0x0191);a=phys(c->ss,(uint16_t)(c->bp-2u));if(p>n-37u||memcmp(r+p,code,37u))return 1;
    for(i=0;i<37;i++)if(a==p+i||((a+1u)&0xfffffu)==p+i)return 1;
    r[a]=synthetic;r[(a+1u)&0xfffffu]=0;bp=get(r,phys(c->ss,c->bp));ret=get(r,phys(c->ss,(uint16_t)(c->bp+2u)));seg=get(r,phys(c->ss,(uint16_t)(c->bp+4u)));
    c->sp=(uint16_t)(c->bp+8u);c->bp=bp;c->ip=ret;c->cs=seg;return 0;
}
