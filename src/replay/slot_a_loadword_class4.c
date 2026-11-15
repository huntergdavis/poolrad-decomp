#include "poolrad/replay/slot_a_loadword_class4.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
int pr_slot_a_loadword_class4(uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p,a,b;unsigned i;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0e19||(uint8_t)c->ax!=4u||c->bp<4u)return 1;
    p=phys(c->cs,0x0e19);a=phys(c->ss,(uint16_t)(c->bp-3u));b=phys(c->ss,(uint16_t)(c->bp-4u));
    if(p>n-142u||memcmp(r+p,code,142u))return 1;
    for(i=0;i<142;i++)if(a==p+i||b==p+i)return 1;
    r[a]=4;r[b]=0;c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|0x0044u);c->ip=0x0ea7;return 0;
}
