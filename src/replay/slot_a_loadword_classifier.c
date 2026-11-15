#include "poolrad/replay/slot_a_loadword_classifier.h"
#include <stddef.h>
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
int pr_slot_a_loadword_classifier(uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c,pr_u25_06fb_status *status)
{
    size_t p,a;unsigned i,j;pr_u25_06fb_cpu raw;pr_u25_06fb_status result;
    _Static_assert(sizeof raw==sizeof *c,"CPU size");
#define SAME(f) _Static_assert(offsetof(pr_u25_06fb_cpu,f)==offsetof(pr_u1c_3171_cpu,f),"CPU field " #f)
    SAME(ax);SAME(bx);SAME(cx);SAME(dx);SAME(si);SAME(di);SAME(bp);SAME(sp);SAME(cs);SAME(ip);SAME(ds);SAME(es);SAME(ss);SAME(flags);
#undef SAME
    if(!r||n!=0x100000u||!code||!c||!status||c->ip!=0x06fb||c->sp<4u||c->sp>0xfffau)return 1;
    p=phys(c->cs,0x06fb);if(p>n-91u||memcmp(r+p,code,91u))return 1;
    for(i=0;i<10;i++){a=phys(c->ss,(uint16_t)(c->sp-4u+i));for(j=0;j<91;j++)if(a==p+j)return 1;}
    memcpy(&raw,c,sizeof raw);result=pr_u25_address_class_06fb_cpu(r,n,&raw);memcpy(c,&raw,sizeof raw);*status=result;return 0;
}
