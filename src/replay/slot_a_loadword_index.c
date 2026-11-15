#include "poolrad/replay/slot_a_loadword_index.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
int pr_slot_a_loadword_index(uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p,a;unsigned i;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0ea7||c->bp>0xfff8u)return 1;
    p=phys(c->cs,0x0ea7);a=phys(c->ss,(uint16_t)(c->bp+6u));
    if(p>n-148u||memcmp(r+p,code,148u)||r[a]!=0x4fu||r[(a+1u)&0xfffffu]!=0xc0u)return 1;
    for(i=0;i<148;i++)if(a==p+i||((a+1u)&0xfffffu)==p+i)return 1;
    r[a]=4;r[(a+1u)&0xfffffu]=0;c->ax=4;c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|0x0044u);c->ip=0x0f3b;return 0;
}
