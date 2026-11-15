#include "poolrad/replay/slot_a_and_reader_prefix.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
int pr_slot_a_and_reader_prefix(uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p,a;uint16_t left,result,flags;uint8_t parity;unsigned i;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0136||c->sp<12u) return 1;
    p=phys(c->cs,0x0136);a=phys(c->ss,(uint16_t)(c->sp-2u));
    if(p>n-6u||memcmp(r+p,code,6u))return 1;
    for(i=0;i<6;i++)if(a==p+i||((a+1u)&0xfffffu)==p+i)return 1;
    c->sp=(uint16_t)(c->sp-2u);r[a]=(uint8_t)c->bp;r[(a+1u)&0xfffffu]=(uint8_t)(c->bp>>8);c->bp=c->sp;left=c->sp;result=(uint16_t)(left-10u);
    parity=(uint8_t)result;parity^=(uint8_t)(parity>>4);parity^=(uint8_t)(parity>>2);parity^=(uint8_t)(parity>>1);
    flags=(uint16_t)((left<10u?1u:0u)|((parity&1u)==0u?4u:0u)|(((left^10u^result)&0x10u)?0x10u:0u)|(result==0?0x40u:0u)|(result&0x8000u?0x80u:0u)|(((left^10u)&(left^result)&0x8000u)?0x800u:0u));
    c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|flags);c->sp=result;c->ip=0x013c;return 0;
}
