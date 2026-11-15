#include "poolrad/replay/slot_a_and_second_call.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void put(uint8_t *r,size_t p,uint16_t v) { r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_and_second_call(uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p,a; uint16_t seg; unsigned i,j;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0e05u||c->sp<6u||c->bp<6u) return 1;
    p=phys(c->cs,0x0e05); if(p>n-11u||memcmp(r+p,code,11u)||code[6]!=0x9a||code[7]!=0x25||code[8]!=0) return 1;
    for(i=0;i<4;i++) { a=i==0?phys(c->ss,(uint16_t)(c->bp-6u)):phys(c->ss,(uint16_t)(c->sp-2u*i)); for(j=0;j<11;j++) if(a==p+j||((a+1u)&0xfffffu)==p+j) return 1; }
    put(r,phys(c->ss,(uint16_t)(c->bp-6u)),c->ax); c->ax=(uint16_t)((c->ax&0xff00u)|2u); c->sp=(uint16_t)(c->sp-2u); put(r,phys(c->ss,c->sp),c->ax); c->sp=(uint16_t)(c->sp-2u); put(r,phys(c->ss,c->sp),c->cs); c->sp=(uint16_t)(c->sp-2u); put(r,phys(c->ss,c->sp),0x0e10); seg=(uint16_t)(code[9]|((uint16_t)code[10]<<8)); c->cs=seg;c->ip=0x0025;return 0;
}
