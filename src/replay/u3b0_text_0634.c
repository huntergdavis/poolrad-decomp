#include "poolrad/replay/u3b0_text_0634.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t a(uint16_t s,uint16_t o){return (((size_t)s<<4u)+o)&MASK;}
static uint8_t rb(pr_bus*b,uint16_t s,uint16_t o){return pr_bus_read8(b,a(s,o));}
static uint16_t rw(pr_bus*b,uint16_t s,uint16_t o){size_t p=a(s,o);return (uint16_t)(pr_bus_read8(b,p)|((uint16_t)pr_bus_read8(b,(p+1u)&MASK)<<8u));}
static void wb(pr_bus*b,uint16_t s,uint16_t o,uint8_t v){pr_bus_write8(b,a(s,o),v);}
static void push(pr_bus*b,pr_r0899_cpu*c,uint16_t v){size_t p;c->sp=(uint16_t)(c->sp-2u);p=a(c->ss,c->sp);pr_bus_write8(b,p,(uint8_t)v);pr_bus_write8(b,(p+1u)&MASK,(uint8_t)(v>>8u));}
static int ordinary(uint16_t s){return s<=0x9000u||s>=0xb000u;}
static void arith(pr_r0899_cpu*c,unsigned x,unsigned y,int sub,int inc){
    unsigned r=(sub?x-y:x+y)&255u,p=r,cf=c->flags&1u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|(inc?cf:((sub?x<y:x+y>255u)?1u:0u))|
        ((p&1u)==0?4u:0u)|((x^y^r)&0x10u)|(r==0?0x40u:0u)|(r&0x80u)|
        ((((sub?(x^y):~(x^y))&(x^r))&0x80u)<<4u));
}
static void low(pr_r0899_cpu*c,uint8_t v){c->ax=(uint16_t)((c->ax&0xff00u)|v);}
static void xor_ah(pr_r0899_cpu*c){c->ax&=0xffu;c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|0x44u);}
#define FIELDS(X) X(ax) X(bx) X(cx) X(dx) X(si) X(di) X(bp) X(sp) X(cs) X(ip) X(ds) X(es) X(ss) X(flags)
#define TO_DRAW(f) t.f=c->f;
#define FROM_DRAW(f) c->f=t.f;
pr_u3b0_text_status pr_u3b0_text_0634(pr_bus*b,size_t n,pr_r0899_cpu*c,
    const pr_u3b0_text_limits*l,pr_u3b0_text_diagnostic*d){
    uint8_t v,limit;uint16_t sp,caller,rel;pr_u3b0_01da_cpu t;
    if(!b||!b->ram||!b->ega||!c||!l||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x0634u||
       c->bp<0x0090u||c->sp<0x0048u||c->sp>0xfffcu||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_U3B0_TEXT_ARGUMENT;
    memset(d,0,sizeof *d);
    low(c,rb(b,c->ss,(uint16_t)(c->bp-0x64u)));
    wb(b,c->ss,(uint16_t)(c->bp-0x90u),(uint8_t)c->ax);
    low(c,1u);limit=rb(b,c->ss,(uint16_t)(c->bp-0x90u));arith(c,(uint8_t)c->ax,limit,1,0);
    if((uint8_t)c->ax>limit){c->ip=0x0691u;return PR_U3B0_TEXT_BOUNDARY;}
    wb(b,c->ss,(uint16_t)(c->bp-0x62u),(uint8_t)c->ax);
    for(;;){
        low(c,rb(b,c->ss,(uint16_t)(c->bp-0x62u)));xor_ah(c);c->di=c->ax;
        low(c,rb(b,c->ss,(uint16_t)(c->bp+c->di-0x53u)));
        v=rb(b,c->ss,(uint16_t)(c->bp-0x60u));arith(c,(uint8_t)c->ax,v,1,0);
        if((uint8_t)c->ax==v){
            wb(b,c->ss,(uint16_t)(c->bp-0x63u),1u);wb(b,c->ss,(uint16_t)(c->bp-0x61u),1u);
            for(;;){
                low(c,rb(b,c->ss,(uint16_t)(c->bp-0x61u)));xor_ah(c);c->di=(uint16_t)(c->ax<<1u);
                low(c,rb(b,c->ss,(uint16_t)(c->bp+c->di-0x8fu)));
                v=rb(b,c->ss,(uint16_t)(c->bp-0x62u));arith(c,(uint8_t)c->ax,v,1,0);
                if((uint8_t)c->ax==v)break;
                v=rb(b,c->ss,(uint16_t)(c->bp-0x61u));arith(c,v,1u,0,1);wb(b,c->ss,(uint16_t)(c->bp-0x61u),(uint8_t)(v+1u));
                c->ip=0x0677u;d->site=0x0677u;if(d->inner_jumps==l->inner_jumps)return PR_U3B0_TEXT_LIMIT;++d->inner_jumps;
            }
            low(c,rb(b,c->ss,(uint16_t)(c->bp-0x61u)));wb(b,c->ds,0x6b2bu,(uint8_t)c->ax);
            low(c,rb(b,c->ds,0x6b2bu));sp=c->sp;caller=c->cs;
            push(b,c,c->ax);push(b,c,c->bp);push(b,c,c->cs);c->ip=0x0685u;d->site=0x0685u;
            rel=rw(b,c->cs,0x0686u);
            if(rb(b,c->cs,0x0685u)!=0xe8u||(uint16_t)(0x0688u+rel)!=0x01dau)return PR_U3B0_TEXT_CODE;
            push(b,c,0x0688u);c->ip=0x01dau;
            FIELDS(TO_DRAW)
            d->status=pr_u3b0_entry_01da_cpu(b,n,&t,&l->drawer,&d->drawer);
            FIELDS(FROM_DRAW)
            if(d->status!=PR_U3B0_01DA_OK)return PR_U3B0_TEXT_CALLEE;
            if(c->cs!=caller||c->ip!=0x0688u||c->sp!=sp)return PR_U3B0_TEXT_TRANSFER;
        }
        low(c,rb(b,c->ss,(uint16_t)(c->bp-0x62u)));limit=rb(b,c->ss,(uint16_t)(c->bp-0x90u));arith(c,(uint8_t)c->ax,limit,1,0);
        if((uint8_t)c->ax==limit){c->ip=0x0691u;return PR_U3B0_TEXT_BOUNDARY;}
        c->ip=0x068fu;d->site=0x068fu;if(d->outer_jumps==l->outer_jumps)return PR_U3B0_TEXT_LIMIT;++d->outer_jumps;
        v=rb(b,c->ss,(uint16_t)(c->bp-0x62u));arith(c,v,1u,0,1);wb(b,c->ss,(uint16_t)(c->bp-0x62u),(uint8_t)(v+1u));
    }
}
