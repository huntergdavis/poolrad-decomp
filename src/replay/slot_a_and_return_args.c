#include "poolrad/replay/slot_a_and_return_args.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static void put(uint8_t *r,size_t p,uint16_t v){r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8);}
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v){c->sp=(uint16_t)(c->sp-2u);put(r,phys(c->ss,c->sp),v);}
int pr_slot_a_and_return_args(uint8_t *r,size_t n,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p,a;unsigned i,j;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0e10||c->bp<4u||c->sp<4u||c->ax!=synthetic)return 1;
    p=phys(c->cs,0x0e10);if(p>n-11u||memcmp(r+p,code,11u))return 1;
    for(i=0;i<3;i++){a=i==0?phys(c->ss,(uint16_t)(c->bp-4u)):phys(c->ss,(uint16_t)(c->sp-2u*i));for(j=0;j<11;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j)return 1;}
    put(r,phys(c->ss,(uint16_t)(c->bp-4u)),c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|r[phys(c->ds,0x7088)]);push(r,c,c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|r[phys(c->ds,0x7048)]);push(r,c,c->ax);c->ip=0x0e1b;return 0;
}
