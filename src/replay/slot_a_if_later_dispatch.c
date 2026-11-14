#include "poolrad/replay/slot_a_if_later_dispatch.h"
#include <string.h>
int pr_slot_a_if_later_dispatch(const uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t p;
    if(!ram||size!=0x100000u||!code||code_size!=149u||!cpu||cpu->ip!=0x2f8cu||(uint8_t)cpu->ax!=0x1bu)return 1;
    p=(((size_t)cpu->cs<<4)+0x2f8cu)&0xfffffu;
    if(p>size-149u||memcmp(ram+p,code,149u))return 1;
    cpu->flags=(uint16_t)((cpu->flags&(uint16_t)~0x08d5u)|0x0044u);cpu->ip=0x3021u;return 0;
}
