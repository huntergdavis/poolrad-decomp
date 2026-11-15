#include "poolrad/replay/slot_a_and_store_stub.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static uint16_t rd(const uint8_t *r,size_t p){return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8));}
int pr_slot_a_and_store_stub(const uint8_t *r,size_t n,const uint8_t *stub,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p;uint16_t target,result=(uint16_t)(synthetic&0x7fu);
    if(!r||n!=0x100000u||!stub||!c||c->ip!=0x0052||c->sp>0xfff8u||c->ax!=result)return 1;
    p=phys(c->cs,0x0052);if(p>n-5u||memcmp(r+p,stub,5u)||stub[0]!=0xea||stub[1]!=0x77||stub[2]!=0x0c)return 1;
    if(rd(r,phys(c->ss,(uint16_t)(c->sp+4u)))!=result||rd(r,phys(c->ss,(uint16_t)(c->sp+6u)))!=0x6e82u)return 1;
    target=(uint16_t)(stub[3]|((uint16_t)stub[4]<<8));c->cs=target;c->ip=0x0c77;return 0;
}
