#include "poolrad/replay/slot_a_save_slot_value.h"
#include <stdlib.h>
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v)
{ size_t p; c->sp=(uint16_t)(c->sp-2u); p=phys(c->ss,c->sp); r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_save_slot_value(uint8_t *ram,size_t size,const uint8_t *call,size_t call_size,
    const uint8_t *stub,size_t stub_size,const uint8_t *body,size_t body_size,
    const uint8_t *word,size_t word_size,pr_u1c_3171_cpu *cpu,pr_u25_slot_value_status *status)
{
    size_t p,q,t,u,tag; uint16_t stubseg,bodyseg; unsigned i; uint8_t *scratch; pr_u1c_3171_cpu next; pr_u25_slot_value_cpu h; pr_u25_slot_value_status result;
    _Static_assert(sizeof h==sizeof next,"CPU size");
#define SAME(f) _Static_assert(offsetof(pr_u25_slot_value_cpu,f)==offsetof(pr_u1c_3171_cpu,f),"CPU field " #f)
    SAME(ax); SAME(bx); SAME(cx); SAME(dx); SAME(si); SAME(di); SAME(bp); SAME(sp); SAME(cs); SAME(ip); SAME(ds); SAME(es); SAME(ss); SAME(flags);
#undef SAME
    if(!ram||!call||!stub||!body||!word||!cpu||!status||size!=0x100000u||call_size!=8u||stub_size!=5u||body_size!=128u||word_size!=36u||cpu->ip!=0x02b7u||cpu->sp<0x0100u||cpu->sp>0xfffbu) return 1;
    p=phys(cpu->cs,0x02b7); if(p>size-8u||memcmp(ram+p,call,8u)||call[3]!=0x9au||call[4]!=0x25u||call[5]!=0u) return 1;
    stubseg=(uint16_t)(call[6]|((uint16_t)call[7]<<8)); q=phys(stubseg,0x25);
    if(q>size-5u||memcmp(ram+q,stub,5u)||stub[0]!=0xeau||stub[1]!=0x36u||stub[2]!=1u) return 1;
    bodyseg=(uint16_t)(stub[3]|((uint16_t)stub[4]<<8)); t=phys(bodyseg,0x0136); u=phys(bodyseg,0x06af);
    if(t>size-128u||u>size-36u||memcmp(ram+t,body,128u)||memcmp(ram+u,word,36u)) return 1;
    tag=phys(cpu->ds,0x7006); if(ram[tag]!=2u) return 1;
    /* Conservative frame exclusion keeps the tag and two source bytes stable
     * through the numeric helper's nested stack writes. */
    for(i=0;i<0x100u;i++) { size_t a=phys(cpu->ss,(uint16_t)(cpu->sp-i));
        if(a==tag||a==phys(cpu->ds,0x7086)||a==phys(cpu->ds,0x7046)) return 1; }
    scratch=malloc(size); if(!scratch) return 1; memcpy(scratch,ram,size); next=*cpu;
    next.ax=(uint16_t)((next.ax&0xff00u)|1u); push(scratch,&next,next.ax); push(scratch,&next,next.cs); push(scratch,&next,0x02bfu); next.cs=bodyseg; next.ip=0x0136;
    memcpy(&h,&next,sizeof h); result=pr_u25_slot_value_0136(scratch,size,&h);
    memcpy(ram,scratch,size); memcpy(cpu,&h,sizeof h); *status=result; free(scratch); return 0;
}
