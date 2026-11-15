#include "poolrad/replay/slot_a_and_reader_tag.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
int pr_slot_a_and_reader_tag(const uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p; uint8_t index,tag; uint16_t di;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0156||c->bp>0xfff9u)return 1;
    p=phys(c->cs,0x0156);if(p>n-36u||memcmp(r+p,code,36u))return 1;
    index=r[phys(c->ss,(uint16_t)(c->bp+6u))];di=(uint16_t)(int16_t)(int8_t)index;tag=r[phys(c->ds,(uint16_t)(0x7005u+di))];if(tag!=1u)return 1;
    c->di=di;c->ax=(uint16_t)((di&0xff00u)|tag);c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|0x0044u);c->ip=0x017a;return 0;
}
