#include "poolrad/replay/slot_a_synthetic_dispatch_return.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static uint16_t rd(const uint8_t *r,size_t p){return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8));}
int pr_slot_a_synthetic_dispatch_return(const uint8_t *r,size_t n,const uint8_t *jump,const uint8_t *tail,uint8_t synthetic,pr_u1c_3171_cpu *c)
{
    size_t j,t;uint16_t sp;
    if(!r||n!=0x100000u||!jump||!tail||!c||c->cs!=0x3582u||c->ip!=0x30f7u||
       c->sp<18u||c->sp>0xfff9u||c->bp!=c->sp||
       rd(r,phys(c->ss,(uint16_t)(c->sp-18u)))!=(synthetic&0x7fu)||
       rd(r,phys(c->ss,(uint16_t)(c->sp+2u)))!=0x31a8u||
       rd(r,phys(c->ss,(uint16_t)(c->sp+4u)))!=0x3582u)return 1;
    j=phys(c->cs,0x30f7u);t=phys(c->cs,0x316fu);
    if(j>n-2u||t>n-2u||memcmp(r+j,jump,2u)||memcmp(r+t,tail,2u)||
       jump[0]!=0xebu||jump[1]!=0x76u||tail[0]!=0x5du||tail[1]!=0xcbu)return 1;
    sp=c->sp;c->bp=rd(r,phys(c->ss,sp));c->ip=rd(r,phys(c->ss,(uint16_t)(sp+2u)));
    c->cs=rd(r,phys(c->ss,(uint16_t)(sp+4u)));c->sp=(uint16_t)(sp+6u);return 0;
}
