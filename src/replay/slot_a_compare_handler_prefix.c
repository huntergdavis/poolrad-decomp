#include "poolrad/replay/slot_a_compare_handler_prefix.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v)
{ size_t p; c->sp=(uint16_t)(c->sp-2u); p=phys(c->ss,c->sp); r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_compare_handler_prefix(uint8_t *ram,size_t size,const uint8_t *call,
 size_t call_size,const uint8_t *body,size_t body_size,pr_u1c_3171_cpu *cpu)
{
    size_t a,b,w[4]; unsigned i,j; uint16_t left,res,f; uint8_t p;
    if(!ram||!call||!body||!cpu||size!=0x100000u||call_size!=4u||body_size!=10u||cpu->ip!=0x2f58u||cpu->sp<0x020cu) return 1;
    a=phys(cpu->cs,0x2f58);b=phys(cpu->cs,0x0126); if(a>size-4u||b>size-10u||memcmp(ram+a,call,4u)||memcmp(ram+b,body,10u)) return 1;
    w[0]=phys(cpu->ss,(uint16_t)(cpu->sp-2u));w[1]=phys(cpu->ss,(uint16_t)(cpu->sp-4u));w[2]=phys(cpu->ss,(uint16_t)(cpu->sp-6u));w[3]=phys(cpu->ss,(uint16_t)(cpu->sp-0x020cu));
    for(i=0;i<4u;i++) for(j=0;j<4u;j++) if(a+i==w[j]||a+i==((w[j]+1u)&0xfffffu)) return 1;
    for(i=0;i<10u;i++) for(j=0;j<4u;j++) if(b+i==w[j]||b+i==((w[j]+1u)&0xfffffu)) return 1;
    push(ram,cpu,cpu->cs);push(ram,cpu,0x2f5cu);push(ram,cpu,cpu->bp);cpu->bp=cpu->sp;
    left=cpu->sp;res=(uint16_t)(left-0x0204u);p=(uint8_t)res;p^=(uint8_t)(p>>4);p^=(uint8_t)(p>>2);p^=(uint8_t)(p>>1);
    f=(uint16_t)((left<0x0204u?1u:0u)|((p&1u)==0u?4u:0u)|(((left^0x0204u^res)&0x10u)?0x10u:0u)|(res==0?0x40u:0u)|(res&0x8000u?0x80u:0u)|(((left^0x0204u)&(left^res)&0x8000u)?0x800u:0u));
    cpu->flags=(uint16_t)((cpu->flags&(uint16_t)~0x08d5u)|f);cpu->sp=res;cpu->ax=(uint16_t)((cpu->ax&0xff00u)|2u);push(ram,cpu,cpu->ax);cpu->ip=0x0130u;return 0;
}
