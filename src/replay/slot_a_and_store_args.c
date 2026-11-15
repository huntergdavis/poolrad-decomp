#include "poolrad/replay/slot_a_and_store_args.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v){size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8);}
int pr_slot_a_and_store_args(uint8_t *r,size_t n,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p,d,v,a;unsigned i,j;uint8_t result=(uint8_t)(synthetic&0x7fu);
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0e4c||c->bp<7u||c->sp<4u)return 1;
    p=phys(c->cs,0x0e4c);d=phys(c->ss,(uint16_t)(c->bp-2u));v=phys(c->ss,(uint16_t)(c->bp-7u));
    if(p>n-9u||memcmp(r+p,code,9u)||r[d]!=0x82u||r[(d+1u)&0xfffffu]!=0x6eu||r[v]!=result)return 1;
    for(i=2;i<=4;i+=2){a=phys(c->ss,(uint16_t)(c->sp-i));if(i==2&&(a==v||((a+1u)&0xfffffu)==v))return 1;for(j=0;j<9;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j)return 1;}
    push(r,c,0x6e82u);c->ax=r[v];c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|0x0044u);push(r,c,c->ax);c->ip=0x0e55;return 0;
}
