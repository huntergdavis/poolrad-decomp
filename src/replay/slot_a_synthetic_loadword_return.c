#include "poolrad/replay/slot_a_synthetic_loadword_return.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static uint16_t get(const uint8_t *r,size_t p){return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8));}
int pr_slot_a_synthetic_loadword_return(const uint8_t *r,size_t n,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p;uint16_t bp,ax,ret,seg;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0f4a||c->bp<2u||c->bp>0xfff8u)return 1;
    p=phys(c->cs,0x0f4a);if(p>n-9u||memcmp(r+p,code,9u))return 1;
    ax=get(r,phys(c->ss,(uint16_t)(c->bp-2u)));if(ax!=synthetic)return 1;
    bp=get(r,phys(c->ss,c->bp));ret=get(r,phys(c->ss,(uint16_t)(c->bp+2u)));seg=get(r,phys(c->ss,(uint16_t)(c->bp+4u)));
    c->sp=(uint16_t)(c->bp+8u);c->bp=bp;c->ax=ax;c->ip=ret;c->cs=seg;return 0;
}
