#include "poolrad/replay/slot_a_compare_loader.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v)
{ size_t p; c->sp=(uint16_t)(c->sp-2u); p=phys(c->ss,c->sp); r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
pr_slot_a_compare_load_status pr_slot_a_compare_loader(uint8_t *ram,size_t size,
 const uint8_t *call,const uint8_t *stub,const uint8_t *body,uint16_t system_cs,
 size_t max_operands,size_t max_strings,pr_u1c_3171_cpu *cpu,pr_slot_a_compare_load_report *report)
{
    size_t pos,p1,p2; uint16_t seg; unsigned i;
    if(!ram||!call||!stub||!body||!cpu||!report||size!=0x100000u) return PR_COMPARE_LOAD_ARGUMENT;
    report->loader=PR_U25_0000_ARGUMENT;
    if(cpu->ip!=0x0130u) return PR_COMPARE_LOAD_CALL_REFUSED;
    pos=phys(cpu->cs,0x0130); if(pos>size-5u||memcmp(ram+pos,call,5u)||call[0]!=0x9au||call[1]!=0x2au||call[2]!=0u) return PR_COMPARE_LOAD_CALL_REFUSED;
    p1=phys(cpu->ss,(uint16_t)(cpu->sp-2u));p2=phys(cpu->ss,(uint16_t)(cpu->sp-4u));
    for(i=0;i<5u;i++) if(pos+i==p1||pos+i==((p1+1u)&0xfffffu)||pos+i==p2||pos+i==((p2+1u)&0xfffffu)) return PR_COMPARE_LOAD_CALL_REFUSED;
    seg=(uint16_t)(call[3]|((uint16_t)call[4]<<8)); push(ram,cpu,cpu->cs);push(ram,cpu,0x0135u);cpu->cs=seg;cpu->ip=0x002au;
    if(pr_slot_a_save_loader_compose(ram,size,stub,5u,body,310u,system_cs,max_operands,max_strings,cpu,&report->loader)==PR_SLOT_A_LOAD_REFUSED) return PR_COMPARE_LOAD_STUB_REFUSED;
    return PR_COMPARE_LOAD_REACHED;
}
