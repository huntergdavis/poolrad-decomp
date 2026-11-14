#include "poolrad/replay/slot_a_runner_opcode_fetch.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
int pr_slot_a_runner_opcode_fetch(uint8_t *ram,size_t size,const uint8_t *code,
 size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t pos,dst; uint16_t left,right,res,f; uint32_t wide; uint8_t p,v; unsigned i;
    if(!ram||!code||!cpu||size!=0x100000u||code_size!=10u||cpu->ip!=0x319au) return 1;
    pos=phys(cpu->cs,0x319a); if(pos>size-10u||memcmp(ram+pos,code,10u)) return 1;
    dst=phys(cpu->ds,0x6f7f); for(i=0;i<10u;i++) if(dst==pos+i) return 1;
    left=cpu->di; right=cpu->ax; wide=(uint32_t)left+right; res=(uint16_t)wide; p=(uint8_t)res;
    p^=(uint8_t)(p>>4); p^=(uint8_t)(p>>2); p^=(uint8_t)(p>>1);
    f=(uint16_t)((wide>0xffffu?1u:0u)|((p&1u)==0u?4u:0u)|(((left^right^res)&0x10u)?0x10u:0u)|(res==0?0x40u:0u)|(res&0x8000u?0x80u:0u)|((~(left^right)&(left^res)&0x8000u)?0x800u:0u));
    cpu->flags=(uint16_t)((cpu->flags&(uint16_t)~0x08d5u)|f); cpu->di=res;
    v=ram[phys(cpu->es,(uint16_t)(res+0x6700u))]; cpu->ax=(uint16_t)((cpu->ax&0xff00u)|v); ram[dst]=v; cpu->ip=0x31a4u; return 0;
}
