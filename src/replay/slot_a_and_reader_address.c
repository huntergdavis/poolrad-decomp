#include "poolrad/replay/slot_a_and_reader_address.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
int pr_slot_a_and_reader_address(const uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p;uint16_t left,right,result,flags;uint8_t parity;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x017a||c->bp<10u)return 1;
    p=phys(c->cs,0x017a);if(p>n-18u||memcmp(r+p,code,18u))return 1;
    right=r[phys(c->ss,(uint16_t)(c->bp-9u))];left=(uint16_t)((uint16_t)r[phys(c->ss,(uint16_t)(c->bp-10u))]<<8);result=(uint16_t)(left+right);
    parity=(uint8_t)result;parity^=(uint8_t)(parity>>4);parity^=(uint8_t)(parity>>2);parity^=(uint8_t)(parity>>1);
    flags=(uint16_t)(((parity&1u)==0u?4u:0u)|(((left^right^result)&0x10u)?0x10u:0u)|(result==0?0x40u:0u)|(result&0x8000u?0x80u:0u)|(((~(left^right))&(left^result)&0x8000u)?0x800u:0u));
    c->dx=right;c->ax=result;c->cx=(uint16_t)((c->cx&0xff00u)|8u);c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|flags);c->ip=0x018c;return 0;
}
