#include "poolrad/replay/slot_a_and_dispatch.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v) { size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
uint16_t pr_slot_a_and_dispatch(uint8_t *r,size_t n,const uint8_t *selector,const uint8_t *call,pr_u1c_3171_cpu *c)
{
    size_t p,a; unsigned i,j; uint16_t rel;
    if(!r||n!=0x100000u||!selector||!call||!c||c->ip!=0x2f3e||(uint8_t)c->ax!=0x2f||c->sp<4) return 0x2f3e;
    p=phys(c->cs,0x2f3e); if(p>n-441u||memcmp(r+p,selector,437)||r[p+437]!=call[0]||call[0]!=0x0e) return 0x2f3e;
    for(i=2;i<=4;i+=2) { a=phys(c->ss,(uint16_t)(c->sp-i)); for(j=0;j<441;j++) if(a==p+j||((a+1u)&0xfffffu)==p+j) return 0x2f3e; }
    c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|0x0044u); push(r,c,c->cs); c->ip=0x30f4;
    rel=(uint16_t)(r[p+439]|((uint16_t)r[p+440]<<8));
    if(memcmp(r+p+438,call+1,3)||r[p+438]!=0xe8||(uint16_t)(0x30f7u+rel)!=0x0def) return c->ip;
    push(r,c,0x30f7); c->ip=0x0def; return c->ip;
}
