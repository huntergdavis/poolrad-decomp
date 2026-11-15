#include "poolrad/replay/slot_a_and_compare_helper.h"
#include <stdlib.h>
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static uint16_t rd(const uint8_t *r,size_t p){return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8));}
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v){size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8);}
static int span(const uint8_t *r,size_t p,const uint8_t *v,size_t n){return p<=0x100000u-n&&!memcmp(r+p,v,n);}
int pr_slot_a_and_compare_helper(uint8_t *ram,size_t size,const pr_and_compare_refs *r,uint8_t synthetic,pr_u1c_3171_cpu *cpu,pr_u25_18eb_status *status)
{
    size_t p,ds,a;uint16_t stubseg,bodyseg,fillseg;unsigned i;uint8_t *copy;pr_u1c_3171_cpu next;pr_u25_18eb_cpu h;pr_u25_18eb_status result;
    _Static_assert(sizeof h==sizeof next,"CPU size");
#define SAME(f) _Static_assert(offsetof(pr_u25_18eb_cpu,f)==offsetof(pr_u1c_3171_cpu,f),"CPU field " #f)
    SAME(ax);SAME(bx);SAME(cx);SAME(dx);SAME(si);SAME(di);SAME(bp);SAME(sp);SAME(cs);SAME(ip);SAME(ds);SAME(es);SAME(ss);SAME(flags);
#undef SAME
    if(!ram||!r||!r->call||!r->stub||!r->body||!r->fill||!cpu||!status||size!=0x100000u||cpu->ip!=0x0e47u||cpu->bp<7u||cpu->sp<0x0100u||cpu->sp>0xfffbu)return 1;
    if(rd(ram,phys(cpu->ss,cpu->sp))!=(uint16_t)(synthetic&0x7fu)||rd(ram,phys(cpu->ss,(uint16_t)(cpu->sp+2u)))!=0||rd(ram,phys(cpu->ss,(uint16_t)(cpu->bp-4u)))!=synthetic)return 1;
    p=phys(cpu->cs,0x0e47);if(!span(ram,p,r->call,5)||r->call[0]!=0x9a||r->call[1]!=0x70||r->call[2]!=0)return 1;
    stubseg=(uint16_t)(r->call[3]|((uint16_t)r->call[4]<<8));p=phys(stubseg,0x70);if(!span(ram,p,r->stub,5)||r->stub[0]!=0xea||r->stub[1]!=0xeb||r->stub[2]!=0x18)return 1;
    bodyseg=(uint16_t)(r->stub[3]|((uint16_t)r->stub[4]<<8));p=phys(bodyseg,0x18eb);if(!span(ram,p,r->body,102)||r->body[15]!=0x9a||r->body[16]!=0xa8||r->body[17]!=0x16)return 1;
    fillseg=(uint16_t)(r->body[18]|((uint16_t)r->body[19]<<8));if(!span(ram,phys(fillseg,0x16a8),r->fill,20))return 1;
    ds=phys(cpu->ds,0);for(i=0;i<0x104u;i++){a=phys(cpu->ss,(uint16_t)(cpu->sp-0x100u+i));if(((a-ds)&0xfffffu)<0x10000u)return 1;}
    for(i=0;i<2u;i++){a=phys(cpu->ss,(uint16_t)(cpu->bp-4u+i));if(((a-ds)&0xfffffu)<0x10000u)return 1;}
    copy=malloc(size);if(!copy)return 1;memcpy(copy,ram,size);next=*cpu;push(copy,&next,next.cs);push(copy,&next,0x0e4c);next.cs=bodyseg;next.ip=0x18eb;
    memcpy(&h,&next,sizeof h);result=pr_u25_compare_words_18eb_cpu(copy,size,&h);memcpy(ram,copy,size);memcpy(cpu,&h,sizeof h);*status=result;free(copy);return 0;
}
