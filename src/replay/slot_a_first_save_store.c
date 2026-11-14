#include "poolrad/replay/slot_a_first_save_store.h"
#include <stdlib.h>
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static uint16_t get(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8)); }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v) { size_t p; c->sp=(uint16_t)(c->sp-2u); p=phys(c->ss,c->sp); r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
static int span(const uint8_t *r,size_t p,const uint8_t *ref,size_t n) { return p<=0x100000u-n && !memcmp(r+p,ref,n); }
int pr_slot_a_first_save_store(uint8_t *ram,size_t size,const uint8_t *call,size_t call_size,
 const uint8_t *stub,size_t stub_size,const uint8_t *body,size_t body_size,
 const uint8_t *hook,size_t hook_size,const uint8_t *classify,size_t classify_size,
 pr_u1c_3171_cpu *cpu,pr_u25_store_word_status *status)
{
    size_t p,q,t,h,k,ds,dst,a; uint16_t stubseg,bodyseg,wseg,woff; unsigned i; uint8_t *scratch; pr_u1c_3171_cpu next; pr_u25_store_word_cpu store; pr_u25_store_word_status result;
    _Static_assert(sizeof store==sizeof next,"CPU size");
#define SAME(f) _Static_assert(offsetof(pr_u25_store_word_cpu,f)==offsetof(pr_u1c_3171_cpu,f),"CPU field " #f)
    SAME(ax); SAME(bx); SAME(cx); SAME(dx); SAME(si); SAME(di); SAME(bp); SAME(sp); SAME(cs); SAME(ip); SAME(ds); SAME(es); SAME(ss); SAME(flags);
#undef SAME
    if(!ram||!call||!stub||!body||!hook||!classify||!cpu||!status||size!=0x100000u||call_size!=5u||stub_size!=5u||body_size!=405u||hook_size!=515u||classify_size!=91u||cpu->ip!=0x02c8u||cpu->sp<0x0100u||cpu->sp>0xfffbu) return 1;
    p=phys(cpu->cs,0x02c8); if(!span(ram,p,call,5)||call[0]!=0x9au||call[1]!=0x52u||call[2]!=0u) return 1;
    stubseg=(uint16_t)(call[3]|((uint16_t)call[4]<<8)); q=phys(stubseg,0x52);
    if(!span(ram,q,stub,5)||stub[0]!=0xeau||stub[1]!=0x77u||stub[2]!=0x0cu) return 1;
    bodyseg=(uint16_t)(stub[3]|((uint16_t)stub[4]<<8)); t=phys(bodyseg,0x0c77); h=phys(bodyseg,0x0a74); k=phys(bodyseg,0x06fb);
    if(!span(ram,t,body,405)||!span(ram,h,hook,515)||!span(ram,k,classify,91)) return 1;
    if(get(ram,phys(cpu->ss,cpu->sp))!=11u||get(ram,phys(cpu->ss,(uint16_t)(cpu->sp+2u)))!=0x6e7du) return 1;
    woff=get(ram,phys(cpu->ds,0x49d6)); wseg=get(ram,phys(cpu->ds,0x49d8)); dst=phys(wseg,(uint16_t)(woff+0x06fau)); ds=phys(cpu->ds,0);
    if(((dst-ds)&0xfffffu)<0x10000u||((((dst+1u)&0xfffffu)-ds)&0xfffffu)<0x10000u) return 1;
    for(i=0;i<0x105u;i++) { a=phys(cpu->ss,(uint16_t)(cpu->sp-0x100u+i));
        if(((a-ds)&0xfffffu)<0x10000u || a==dst || a==((dst+1u)&0xfffffu)) return 1; }
    scratch=malloc(size); if(!scratch) return 1; memcpy(scratch,ram,size); next=*cpu;
    push(scratch,&next,next.cs); push(scratch,&next,0x02cdu); next.cs=bodyseg; next.ip=0x0c77u;
    memcpy(&store,&next,sizeof store); result=pr_u25_store_word_0c77(scratch,size,&store);
    memcpy(ram,scratch,size); memcpy(cpu,&store,sizeof store); *status=result; free(scratch); return 0;
}
