#include "poolrad/replay/slot_a_and_reader_bytes.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
int pr_slot_a_and_reader_bytes(uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p,a,b; unsigned i; uint8_t index,value;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x013c||c->bp<10u||c->bp>0xfff9u)return 1;
    p=phys(c->cs,0x013c);a=phys(c->ss,(uint16_t)(c->bp-9u));b=phys(c->ss,(uint16_t)(c->bp-10u));
    if(p>n-26u||memcmp(r+p,code,26u))return 1;
    for(i=0;i<26;i++)if(a==p+i||b==p+i)return 1;
    index=r[phys(c->ss,(uint16_t)(c->bp+6u))];c->ax=(uint16_t)(int16_t)(int8_t)index;c->di=c->ax;value=r[phys(c->ds,(uint16_t)(0x7085u+c->di))];c->ax=(uint16_t)((c->ax&0xff00u)|value);r[a]=value;
    index=r[phys(c->ss,(uint16_t)(c->bp+6u))];c->ax=(uint16_t)(int16_t)(int8_t)index;c->di=c->ax;value=r[phys(c->ds,(uint16_t)(0x7045u+c->di))];c->ax=(uint16_t)((c->ax&0xff00u)|value);r[b]=value;c->ip=0x0156;return 0;
}
