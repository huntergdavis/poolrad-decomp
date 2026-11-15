#include "poolrad/replay/slot_a_if_equal_dispatch_prefix.h"
#include "poolrad/replay/slot_a_compare_dispatch.h"
#include <string.h>
uint16_t pr_slot_a_if_equal_dispatch_prefix(uint8_t *ram,size_t size,pr_u1c_3171_cpu *cpu,const uint8_t *prologue,const uint8_t *first,const uint8_t *selector)
{
    uint16_t stop;size_t p;
    if(!ram||size!=0x100000u||!cpu||!prologue||!first||!selector||cpu->ip!=0x31a4u)return 0x31a4u;
    stop=pr_slot_a_compare_dispatch(ram,size,cpu,prologue,first,selector);
    if(stop!=0x2f3eu||cpu->ip!=0x2f3eu||(uint8_t)cpu->ax!=0x16u)return stop;
    p=(((size_t)cpu->cs<<4)+0x2f3eu)&0xfffffu;
    if(p>size-78u||memcmp(ram+p,selector,78u))return cpu->ip;
    cpu->flags=(uint16_t)((cpu->flags&(uint16_t)~0x08d5u)|0x0010u);cpu->ip=0x2f8cu;return cpu->ip;
}
