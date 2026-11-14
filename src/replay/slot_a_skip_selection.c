#include "poolrad/replay/slot_a_skip_selection.h"
#include <string.h>
int pr_slot_a_skip_selection(const uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t p;
    if(!ram||size!=0x100000u||!code||code_size!=102u||!cpu||cpu->ip!=0x1ebdu||(uint8_t)cpu->ax!=9u)return 1;
    p=(((size_t)cpu->cs<<4)+0x1ebdu)&0xfffffu;if(p>size-102u||memcmp(ram+p,code,102u))return 1;
    cpu->flags=(uint16_t)((cpu->flags&(uint16_t)~0x08d5u)|0x0044u);cpu->ip=0x1f23u;return 0;
}
