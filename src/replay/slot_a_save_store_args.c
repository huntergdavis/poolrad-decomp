#include "poolrad/replay/slot_a_save_store_args.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static uint16_t get(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8)); }
static void put(uint8_t *r,size_t p,uint16_t v) { r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v) { c->sp=(uint16_t)(c->sp-2u); put(r,phys(c->ss,c->sp),v); }
int pr_slot_a_save_store_args(uint8_t *ram,size_t size,const uint8_t *code,
    size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t pos,w[3]; unsigned i,j;
    if(!ram||!code||!cpu||size!=0x100000u||code_size!=9u||cpu->ip!=0x02bfu) return 1;
    pos=phys(cpu->cs,0x02bf); if(pos>size-9u||memcmp(ram+pos,code,9u)) return 1;
    w[0]=phys(cpu->ss,(uint16_t)(cpu->bp-2u)); w[1]=phys(cpu->ss,(uint16_t)(cpu->sp-2u)); w[2]=phys(cpu->ss,(uint16_t)(cpu->sp-4u));
    for(i=0;i<9u;i++) for(j=0;j<3u;j++) if(pos+i==w[j]||pos+i==((w[j]+1u)&0xfffffu)) return 1;
    put(ram,w[0],cpu->ax); push(ram,cpu,get(ram,phys(cpu->ss,(uint16_t)(cpu->bp-4u))));
    push(ram,cpu,get(ram,phys(cpu->ss,(uint16_t)(cpu->bp-2u)))); cpu->ip=0x02c8u; return 0;
}
