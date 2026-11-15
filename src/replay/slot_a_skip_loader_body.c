#include "poolrad/replay/slot_a_skip_loader_body.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static uint16_t rd(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8)); }
int pr_slot_a_skip_loader_body(uint8_t *ram,size_t size,const uint8_t *body,size_t body_size,uint16_t system_cs,pr_u1c_3171_cpu *cpu,pr_u25_0000_status *status)
{
    size_t p,ds,src[5],a;uint16_t pc,off,seg;unsigned i,j;pr_u25_0000_cpu h;pr_u25_0000_status result;
    _Static_assert(sizeof h==sizeof *cpu,"CPU size");
#define SAME(f) _Static_assert(offsetof(pr_u25_0000_cpu,f)==offsetof(pr_u1c_3171_cpu,f),"CPU field " #f)
    SAME(ax);SAME(bx);SAME(cx);SAME(dx);SAME(si);SAME(di);SAME(bp);SAME(sp);SAME(cs);SAME(ip);SAME(ds);SAME(es);SAME(ss);SAME(flags);
#undef SAME
    if(!ram||size!=0x100000u||!body||body_size!=310u||!cpu||!status||cpu->ip!=0u||cpu->sp<0x022au||cpu->sp>0xfffau)return 1;
    p=phys(cpu->cs,0);if(p>size-310u||memcmp(ram+p,body,310u)||(uint8_t)rd(ram,phys(cpu->ss,(uint16_t)(cpu->sp+4u)))!=2u)return 1;
    ds=phys(cpu->ds,0);pc=rd(ram,phys(cpu->ds,0x49edu));off=rd(ram,phys(cpu->ds,0x49deu));seg=rd(ram,phys(cpu->ds,0x49e0u));
    for(i=0;i<5u;i++){src[i]=phys(seg,(uint16_t)(off+pc+0x6701u+i));if(((src[i]-ds)&0xfffffu)<0x10000u)return 1;}
    if(ram[src[0]]!=0u||ram[src[2]]!=2u)return 1;
    for(i=0;i<=0x22au;i++){a=phys(cpu->ss,(uint16_t)(cpu->sp-i));if(((a-ds)&0xfffffu)<0x10000u)return 1;for(j=0;j<5u;j++)if(a==src[j])return 1;}
    for(i=1u;i<=5u;i++){a=phys(cpu->ss,(uint16_t)(cpu->sp+i));if(((a-ds)&0xfffffu)<0x10000u)return 1;for(j=0;j<5u;j++)if(a==src[j])return 1;}
    memcpy(&h,cpu,sizeof h);result=pr_u25_load_operands_0000(ram,size,&h,system_cs,2u,1u);memcpy(cpu,&h,sizeof h);*status=result;return 0;
}
