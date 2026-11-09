#include "poolrad/replay/u3b0_draw_wait_047a.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t a(uint16_t s,uint16_t o){return (((size_t)s<<4u)+o)&MASK;}
static uint8_t rb(pr_bus*b,uint16_t s,uint16_t o){return pr_bus_read8(b,a(s,o));}
static uint16_t rw(pr_bus*b,uint16_t s,uint16_t o){size_t p=a(s,o);uint8_t lo,hi;lo=pr_bus_read8(b,p);hi=pr_bus_read8(b,(p+1u)&MASK);return (uint16_t)(lo|((uint16_t)hi<<8u));}
static void wb(pr_bus*b,uint16_t s,uint16_t o,uint8_t v){pr_bus_write8(b,a(s,o),v);}
static void push(pr_bus*b,pr_r0899_cpu*c,uint16_t v){size_t p;c->sp=(uint16_t)(c->sp-2u);p=a(c->ss,c->sp);pr_bus_write8(b,p,(uint8_t)v);pr_bus_write8(b,(p+1u)&MASK,(uint8_t)(v>>8u));}
static int ordinary(uint16_t s){return s<=0x9000u||s>=0xb000u;}
static void add_zero_flags(pr_r0899_cpu*c){unsigned v=c->ax,p=v&255u;p^=p>>4;p^=p>>2;p^=p>>1;c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|((p&1u)==0u?4u:0u)|(v==0u?0x40u:0u)|(v&0x8000u?0x80u:0u));}
#define FIELDS(X) X(ax) X(bx) X(cx) X(dx) X(si) X(di) X(bp) X(sp) X(cs) X(ip) X(ds) X(es) X(ss) X(flags)
#define TO_DRAW(f) t.f=c->f;
#define FROM_DRAW(f) c->f=t.f;
pr_u3b0_draw_wait_status pr_u3b0_draw_wait_047a(pr_bus*b,size_t n,pr_r0899_cpu*c,
    const pr_u3b0_draw_wait_env*e,pr_u3b0_draw_wait_diagnostic*d){
    pr_u3b0_01da_cpu t;uint16_t caller,sp,rel;uint8_t v;
    if(!b||!b->ram||!b->ega||!c||!e||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x047au||
       c->bp<0x0090u||c->bp>0xffe3u||c->sp<0x0116u||c->sp>0xfffcu||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_U3B0_DRAW_WAIT_ARGUMENT;
    memset(d,0,sizeof *d);
    v=rb(b,c->ss,(uint16_t)(c->bp-0x2au));c->ax=v; /* MOV AL; XOR AH,AH */
    add_zero_flags(c); /* ADD AX,0; its flags supersede XOR */
    wb(b,c->ss,(uint16_t)(c->bp-0x65u),(uint8_t)c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,0x6b2bu));
    caller=c->cs;sp=c->sp;push(b,c,c->ax);push(b,c,c->bp);push(b,c,c->cs);c->ip=0x048bu;
    rel=rw(b,c->cs,0x048cu);
    if(rb(b,c->cs,0x048bu)!=0xe8u||(uint16_t)(0x048eu+rel)!=0x01dau)return PR_U3B0_DRAW_WAIT_CODE;
    push(b,c,0x048eu);c->ip=0x01dau;
    FIELDS(TO_DRAW)
    d->drawer_status=pr_u3b0_entry_01da_cpu(b,n,&t,&e->drawer,&d->drawer);
    FIELDS(FROM_DRAW)
    if(d->drawer_status!=PR_U3B0_01DA_OK)return PR_U3B0_DRAW_WAIT_CALLEE;
    if(c->cs!=caller||c->ip!=0x048eu||c->sp!=sp)return PR_U3B0_DRAW_WAIT_TRANSFER;
    d->wait_status=pr_u3b0_wait_048e(b,n,c,&e->pass,&d->wait);
    return PR_U3B0_DRAW_WAIT_REACHED;
}
