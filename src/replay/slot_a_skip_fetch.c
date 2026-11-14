#include "poolrad/replay/slot_a_skip_fetch.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static uint16_t rd(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8)); }
static void wr(uint8_t *r,size_t p,uint16_t v) { r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_skip_fetch(uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t p,stack,ds,source;uint16_t pc,off,seg,res,flags;uint32_t wide;uint8_t parity,op;
    if(!ram||size!=0x100000u||!code||code_size!=23u||!cpu||cpu->ip!=0x1ea6u||cpu->sp<2u)return 1;
    p=phys(cpu->cs,0x1ea6);if(p>size-23u||memcmp(ram+p,code,23u))return 1;
    ds=phys(cpu->ds,0);stack=phys(cpu->ss,(uint16_t)(cpu->sp-2u));
    if(((stack-ds)&0xfffffu)<0x10000u||((((stack+1u)&0xfffffu)-ds)&0xfffffu)<0x10000u)return 1;
    pc=rd(ram,phys(cpu->ds,0x49ed));off=rd(ram,phys(cpu->ds,0x49de));seg=rd(ram,phys(cpu->ds,0x49e0));res=(uint16_t)(off+pc);source=phys(seg,(uint16_t)(res+0x6700u));
    if(source==stack||source==((stack+1u)&0xfffffu))return 1;
    wide=(uint32_t)off+pc;parity=(uint8_t)res;parity^=(uint8_t)(parity>>4);parity^=(uint8_t)(parity>>2);parity^=(uint8_t)(parity>>1);
    flags=(uint16_t)((wide>0xffffu?1u:0u)|((parity&1u)==0u?4u:0u)|(((off^pc^res)&0x10u)?0x10u:0u)|(res==0?0x40u:0u)|(res&0x8000u?0x80u:0u)|((~(off^pc)&(off^res)&0x8000u)?0x800u:0u));
    op=ram[source];cpu->sp=(uint16_t)(cpu->sp-2u);wr(ram,stack,cpu->bp);cpu->bp=cpu->sp;cpu->di=res;cpu->es=seg;cpu->ax=(uint16_t)((pc&0xff00u)|op);ram[phys(cpu->ds,0x6f7f)]=op;cpu->flags=(uint16_t)((cpu->flags&(uint16_t)~0x08d5u)|flags);cpu->ip=0x1ebdu;return 0;
}
