#include "poolrad/replay/slot_a_synthetic_and_return.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static uint16_t rd(const uint8_t *r,size_t p){return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8));}
int pr_slot_a_synthetic_and_return(uint8_t *r,size_t n,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t p;uint16_t bp;
    if(!r||n!=0x100000u||!code||!c||c->cs!=0x3582u||c->ip!=0x0e5au||
       c->bp<12u||c->bp>0xfff9u||c->sp!=(uint16_t)(c->bp-8u)||
       rd(r,phys(c->ss,(uint16_t)(c->sp-4u)))!=(synthetic&0x7fu)||
       rd(r,phys(c->ss,(uint16_t)(c->bp+2u)))!=0x30f7u||
       rd(r,phys(c->ss,(uint16_t)(c->bp+4u)))!=0x3582u) return 1;
    p=phys(c->cs,0x0e5au);if(p>n-4u||memcmp(r+p,code,4u)||
       code[0]!=0x89u||code[1]!=0xecu||code[2]!=0x5du||code[3]!=0xcbu)return 1;
    bp=c->bp;c->bp=rd(r,phys(c->ss,bp));c->ip=rd(r,phys(c->ss,(uint16_t)(bp+2u)));
    c->cs=rd(r,phys(c->ss,(uint16_t)(bp+4u)));c->sp=(uint16_t)(bp+6u);return 0;
}
