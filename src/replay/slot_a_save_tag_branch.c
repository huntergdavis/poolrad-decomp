#include "poolrad/replay/slot_a_save_tag_branch.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
int pr_slot_a_save_tag_branch(uint8_t *ram,size_t size,const uint8_t *code,
    size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t pos,dst; unsigned i; uint8_t v,result,parity; uint16_t flags;
    if(!ram||!code||!cpu||size!=0x100000u||code_size!=10u||cpu->ip!=0x02adu) return 1;
    pos=phys(cpu->cs,0x02ad); if(pos>size-10u||memcmp(ram+pos,code,10u)) return 1;
    dst=phys(cpu->ss,(uint16_t)(cpu->bp-4u));
    for(i=0;i<10u;i++) if(pos+i==dst || pos+i==((dst+1u)&0xfffffu)) return 1;
    ram[dst]=(uint8_t)cpu->ax; ram[(dst+1u)&0xfffffu]=(uint8_t)(cpu->ax>>8);
    v=ram[phys(cpu->ds,0x7006u)]; result=(uint8_t)(v-0x80u); parity=result;
    parity^=(uint8_t)(parity>>4); parity^=(uint8_t)(parity>>2); parity^=(uint8_t)(parity>>1);
    flags=(uint16_t)((v<0x80u?1u:0u)|((parity&1u)==0u?4u:0u)|(result==0?0x40u:0u)|(result&0x80u?0x80u:0u)|(v<0x80u?0x800u:0u));
    cpu->flags=(uint16_t)((cpu->flags & (uint16_t)~0x08d5u)|flags); cpu->ip=v<0x80u?0x02b7u:0x02cfu; return 0;
}
