#include "poolrad/replay/slot_a_and_store_call.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static uint16_t rd(const uint8_t *r,size_t p){return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8));}
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v){size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8);}
int pr_slot_a_and_store_call(uint8_t *r,size_t n,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p,a;uint16_t seg,result=(uint16_t)(synthetic&0x7fu);unsigned i,j;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0e55||c->sp<4u||c->sp>0xfffbu||c->ax!=result)return 1;
    p=phys(c->cs,0x0e55);if(p>n-5u||memcmp(r+p,code,5u)||code[0]!=0x9a||code[1]!=0x52||code[2]!=0)return 1;
    if(rd(r,phys(c->ss,c->sp))!=result||rd(r,phys(c->ss,(uint16_t)(c->sp+2u)))!=0x6e82u)return 1;
    for(i=2;i<=4;i+=2){a=phys(c->ss,(uint16_t)(c->sp-i));for(j=0;j<5;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j)return 1;}
    seg=(uint16_t)(code[3]|((uint16_t)code[4]<<8));push(r,c,c->cs);push(r,c,0x0e5a);c->cs=seg;c->ip=0x0052;return 0;
}
