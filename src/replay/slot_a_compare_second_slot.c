#include "poolrad/replay/slot_a_compare_second_slot.h"
#include <stdlib.h>
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v) { size_t p; c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
static int span(const uint8_t *r,size_t p,const uint8_t *ref,size_t n) { return p<=0x100000u-n && !memcmp(r+p,ref,n); }
int pr_slot_a_compare_second_slot(uint8_t *ram,size_t size,const pr_compare_slot_refs *r,
 pr_u1c_3171_cpu *cpu,pr_u25_slot_value_status *status)
{
    size_t p,q,ds,a,local;uint16_t stubseg,bodyseg;unsigned i;uint8_t *scratch;pr_u1c_3171_cpu next;pr_u25_slot_value_cpu h;pr_u25_slot_value_status result;
    _Static_assert(sizeof h==sizeof next,"CPU size");
#define SAME(f) _Static_assert(offsetof(pr_u25_slot_value_cpu,f)==offsetof(pr_u1c_3171_cpu,f),"CPU field " #f)
    SAME(ax);SAME(bx);SAME(cx);SAME(dx);SAME(si);SAME(di);SAME(bp);SAME(sp);SAME(cs);SAME(ip);SAME(ds);SAME(es);SAME(ss);SAME(flags);
#undef SAME
    if(!ram||!r||!r->call||!r->stub||!r->slot||!r->word||!r->load||!r->classify||!r->hook||!r->party||!cpu||!status||size!=0x100000u||cpu->ip!=0x0186u||cpu->bp<0x0304u||cpu->sp!=(uint16_t)(cpu->bp-0x0204u)) return 1;
    p=phys(cpu->cs,0x0186);if(!span(ram,p,r->call,11)||r->call[6]!=0x9au||r->call[7]!=0x25u||r->call[8]!=0u) return 1;
    stubseg=(uint16_t)(r->call[9]|((uint16_t)r->call[10]<<8));q=phys(stubseg,0x25);if(!span(ram,q,r->stub,5)||r->stub[0]!=0xeau||r->stub[1]!=0x36u||r->stub[2]!=1u) return 1;
    bodyseg=(uint16_t)(r->stub[3]|((uint16_t)r->stub[4]<<8));
    if(!span(ram,phys(bodyseg,0x0136),r->slot,128)||!span(ram,phys(bodyseg,0x06af),r->word,36)||!span(ram,phys(bodyseg,0x0e0c),r->load,327)||!span(ram,phys(bodyseg,0x06fb),r->classify,91)||!span(ram,phys(bodyseg,0x07bc),r->hook,696)||!span(ram,phys(bodyseg,0x0756),r->party,102)) return 1;
    ds=phys(cpu->ds,0);if(ram[phys(cpu->ds,0x7007)]!=0u) return 1;
    local=phys(cpu->ss,(uint16_t)(cpu->bp-2u));
    for(i=0;i<2u;i++){a=(local+i)&0xfffffu;if(((a-ds)&0xfffffu)<0x10000u) return 1;}
    for(i=0;i<0x101u;i++){a=phys(cpu->ss,(uint16_t)(cpu->sp-i));if(((a-ds)&0xfffffu)<0x10000u||a==local||a==((local+1u)&0xfffffu)) return 1;}
    scratch=malloc(size);if(!scratch) return 1;memcpy(scratch,ram,size);next=*cpu;scratch[local]=(uint8_t)next.ax;scratch[(local+1u)&0xfffffu]=(uint8_t)(next.ax>>8);next.ax=(uint16_t)((next.ax&0xff00u)|2u);push(scratch,&next,next.ax);push(scratch,&next,next.cs);push(scratch,&next,0x0191u);next.cs=bodyseg;next.ip=0x0136u;
    memcpy(&h,&next,sizeof h);result=pr_u25_slot_value_0136(scratch,size,&h);memcpy(ram,scratch,size);memcpy(cpu,&h,sizeof h);*status=result;free(scratch);return 0;
}
