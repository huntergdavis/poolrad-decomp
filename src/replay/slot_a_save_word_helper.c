#include "poolrad/replay/slot_a_save_word_helper.h"
#include <stdlib.h>
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v)
{ size_t p; c->sp=(uint16_t)(c->sp-2u); p=phys(c->ss,c->sp); r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
int pr_slot_a_save_word_helper(uint8_t *ram,size_t size,const uint8_t *call,
    size_t call_size,const uint8_t *stub,size_t stub_size,const uint8_t *body,
    size_t body_size,pr_u1c_3171_cpu *cpu,pr_u25_0000_status *status)
{
    size_t p,q,t; uint16_t stubseg,bodyseg; uint8_t *scratch; pr_u1c_3171_cpu next; pr_u25_0000_cpu helper; pr_u25_0000_status outcome;
    _Static_assert(sizeof helper==sizeof next,"CPU size");
#define SAME(f) _Static_assert(offsetof(pr_u25_0000_cpu,f)==offsetof(pr_u1c_3171_cpu,f),"CPU field " #f)
    SAME(ax); SAME(bx); SAME(cx); SAME(dx); SAME(si); SAME(di); SAME(bp); SAME(sp); SAME(cs); SAME(ip); SAME(ds); SAME(es); SAME(ss); SAME(flags);
#undef SAME
    if(!ram||!call||!stub||!body||!cpu||!status||size!=0x100000u||call_size!=5u||stub_size!=5u||body_size!=36u||cpu->ip!=0x02a8u) return 1;
    p=phys(cpu->cs,0x02a8); if(p>size-5u||memcmp(ram+p,call,5u)||call[0]!=0x9au||call[1]!=0x4du||call[2]!=0u) return 1;
    stubseg=(uint16_t)(call[3]|((uint16_t)call[4]<<8)); q=phys(stubseg,0x004d);
    if(q>size-5u||memcmp(ram+q,stub,5u)||stub[0]!=0xeau||stub[1]!=0xafu||stub[2]!=6u) return 1;
    bodyseg=(uint16_t)(stub[3]|((uint16_t)stub[4]<<8)); t=phys(bodyseg,0x06af);
    if(t>size-36u||memcmp(ram+t,body,36u)) return 1;
    scratch=malloc(size); if(!scratch) return 1; memcpy(scratch,ram,size); next=*cpu;
    push(scratch,&next,next.cs); push(scratch,&next,0x02adu); next.cs=bodyseg; next.ip=0x06afu;
    memcpy(&helper,&next,sizeof helper); outcome=pr_u25_word_06af(scratch,size,&helper);
    memcpy(ram,scratch,size); memcpy(cpu,&helper,sizeof helper); *status=outcome; free(scratch); return 0;
}
