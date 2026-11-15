#include "poolrad/replay/slot_a_and_handler_prefix.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void put(uint8_t *r,size_t p,uint16_t v) { r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_and_handler_prefix(uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p,a; uint16_t left,result,flags; uint8_t parity; unsigned i,j;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0def||c->sp<12u) return 1;
    p=phys(c->cs,0x0def); if(p>n-9u||memcmp(r+p,code,9u)) return 1;
    for(i=2;i<=12;i+=10) { a=phys(c->ss,(uint16_t)(c->sp-i)); for(j=0;j<9;j++) if(a==p+j||((a+1u)&0xfffffu)==p+j) return 1; }
    c->sp=(uint16_t)(c->sp-2u); put(r,phys(c->ss,c->sp),c->bp); c->bp=c->sp; left=c->sp; result=(uint16_t)(left-8u);
    parity=(uint8_t)result; parity^=(uint8_t)(parity>>4); parity^=(uint8_t)(parity>>2); parity^=(uint8_t)(parity>>1);
    flags=(uint16_t)((left<8u?1u:0u)|((parity&1u)==0u?4u:0u)|(((left^8u^result)&0x10u)?0x10u:0u)|(result==0?0x40u:0u)|(result&0x8000u?0x80u:0u)|(((left^8u)&(left^result)&0x8000u)?0x800u:0u));
    c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|flags); c->sp=result; c->ax=(uint16_t)((c->ax&0xff00u)|3u); c->sp=(uint16_t)(c->sp-2u); put(r,phys(c->ss,c->sp),c->ax); c->ip=0x0df8; return 0;
}
