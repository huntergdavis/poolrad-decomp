#include "poolrad/replay/slot_a_and_compare_args.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v){size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8);}
int pr_slot_a_and_compare_args(uint8_t *r,size_t n,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p,v,a;unsigned i,j;uint8_t result=(uint8_t)(synthetic&0x7fu);
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0e3e||c->bp<7u||c->sp<4u||c->ax!=result)return 1;
    p=phys(c->cs,0x0e3e);v=phys(c->ss,(uint16_t)(c->bp-7u));if(p>n-9u||memcmp(r+p,code,9u)||r[v]!=result)return 1;
    for(i=2;i<=4;i+=2){a=phys(c->ss,(uint16_t)(c->sp-i));if(i==2&&(a==v||((a+1u)&0xfffffu)==v))return 1;for(j=0;j<9;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j)return 1;}
    c->ax=0;push(r,c,0);c->ax=r[v];c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|0x0044u);push(r,c,c->ax);c->ip=0x0e47;return 0;
}
