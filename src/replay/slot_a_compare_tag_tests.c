#include "poolrad/replay/slot_a_compare_tag_tests.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void cmp80(pr_u1c_3171_cpu *c,uint8_t v)
{ uint8_t res=(uint8_t)(v-0x80u),p=res; uint16_t f; p^=(uint8_t)(p>>4);p^=(uint8_t)(p>>2);p^=(uint8_t)(p>>1); f=(uint16_t)((v<0x80u?0x801u:0u)|((p&1u)==0u?4u:0u)|(res==0u?0x40u:0u)|(res&0x80u));c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|f); }
int pr_slot_a_compare_tag_tests(const uint8_t *ram,size_t size,const uint8_t *code,
 size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t pos;uint8_t v;
    if(!ram||!code||!cpu||size!=0x100000u||code_size!=14u||cpu->ip!=0x0135u) return 1;
    pos=phys(cpu->cs,0x0135);if(pos>size-14u||memcmp(ram+pos,code,14u)) return 1;
    v=ram[phys(cpu->ds,0x7006)];cmp80(cpu,v);if(v>=0x80u){cpu->ip=0x0143u;return 0;}
    v=ram[phys(cpu->ds,0x7007)];cmp80(cpu,v);cpu->ip=v>=0x80u?0x0143u:0x017eu;return 0;
}
