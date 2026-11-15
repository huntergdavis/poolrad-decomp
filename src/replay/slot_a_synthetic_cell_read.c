#include "poolrad/replay/slot_a_synthetic_cell_read.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
int pr_slot_a_synthetic_cell_read(uint8_t *r,size_t n,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p,a,src,arg;unsigned i;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0f3b||c->bp<2u||c->bp>0xfff8u||c->ax!=4u)return 1;
    p=phys(c->cs,0x0f3b);a=phys(c->ss,(uint16_t)(c->bp-2u));src=phys(c->ds,0x6ab1);arg=phys(c->ss,(uint16_t)(c->bp+6u));
    if(p>n-10u||memcmp(r+p,code,10u)||code[0]!=0xa0||code[1]!=0xb1||code[2]!=0x6a||code[8]!=0xeb||code[9]!=5u||r[src]!=synthetic||r[arg]!=4u||r[(arg+1u)&0xfffffu]!=0u)return 1;
    for(i=0;i<10;i++)if(a==p+i||((a+1u)&0xfffffu)==p+i)return 1;
    c->ax=synthetic;c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|0x0044u);r[a]=synthetic;r[(a+1u)&0xfffffu]=0;c->ip=0x0f4a;return 0;
}
