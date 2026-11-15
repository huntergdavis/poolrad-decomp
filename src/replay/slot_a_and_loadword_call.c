#include "poolrad/replay/slot_a_and_loadword_call.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v) { size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_and_loadword_call(uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p,a;uint16_t rel;unsigned i,j;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x018c||c->sp<6u)return 1;
    p=phys(c->cs,0x018c);if(p>n-5u||memcmp(r+p,code,5u)||code[0]!=0x50||code[1]!=0x0e||code[2]!=0xe8)return 1;
    rel=(uint16_t)(code[3]|((uint16_t)code[4]<<8));if((uint16_t)(0x0191u+rel)!=0x0e0c)return 1;
    for(i=2;i<=6;i+=2){a=phys(c->ss,(uint16_t)(c->sp-i));for(j=0;j<5;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j)return 1;}
    push(r,c,c->ax);push(r,c,c->cs);push(r,c,0x0191);c->ip=0x0e0c;return 0;
}
