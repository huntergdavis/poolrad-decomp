#include "poolrad/replay/u68_prompt_098f.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t at(uint16_t s,uint16_t o){return (((size_t)s<<4u)+o)&MASK;}
static uint8_t rb(pr_bus*b,uint16_t s,uint16_t o){return pr_bus_read8(b,at(s,o));}
static uint16_t rw(pr_bus*b,uint16_t s,uint16_t o){size_t p=at(s,o);uint8_t l=pr_bus_read8(b,p),h=pr_bus_read8(b,(p+1u)&MASK);return (uint16_t)(l|((uint16_t)h<<8u));}
static void wb(pr_bus*b,uint16_t s,uint16_t o,uint8_t v){pr_bus_write8(b,at(s,o),v);}
static void ww(pr_bus*b,uint16_t s,uint16_t o,uint16_t v){size_t p=at(s,o);pr_bus_write8(b,p,(uint8_t)v);pr_bus_write8(b,(p+1u)&MASK,(uint8_t)(v>>8u));}
static void push(pr_bus*b,pr_r0899_cpu*c,uint16_t v){c->sp=(uint16_t)(c->sp-2u);ww(b,c->ss,c->sp,v);}
static int ordinary(uint16_t s){return s<0x9000u||s>=0xb000u;}
static void cmp8(pr_r0899_cpu*c,uint8_t a,uint8_t b){unsigned r=(a-b)&255u,p=r;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(a<b?1u:0u)|((p&1u)?0u:4u)|(((a^b^r)&0x10u)?0x10u:0u)|(r==0?0x40u:0u)|(r&0x80u?0x80u:0u)|(((a^b)&(a^r)&0x80u)?0x800u:0u));}
static void xorax(pr_r0899_cpu*c){c->ax=0;c->flags=(uint16_t)((c->flags&~0x08d5u)|0x44u);}
#define FIELDS(X) X(ax) X(bx) X(cx) X(dx) X(si) X(di) X(bp) X(sp) X(cs) X(ip) X(ds) X(es) X(ss) X(flags)
#define TO(f) t.f=c->f;
#define FROM(f) c->f=t.f;
void pr_u68_prompt_init(pr_u68_prompt*s){if(s)memset(s,0,sizeof *s);}
static pr_u68_prompt_status finish(pr_u68_prompt*s,pr_u68_prompt_status st){s->phase=PR_U68_PROMPT_FINISHED;return st;}
/* The flat preflight may reread the map pointer after stack and first window writes.
 * Reject those aliases before checking its segment, rather than assuming stability. */
static int preflight_safe(pr_bus*b,const pr_r0899_cpu*c){
    size_t ptr=at(c->ds,0x6a5cu),window_ptr=at(c->ds,0x49d6u),wp,wp_next;uint16_t wo,ws,stub;unsigned i,j;int delta;
    stub=rw(b,c->cs,0x06c3u);if(!ordinary(stub))return 0;
    wo=rw(b,c->ds,0x49d6u);ws=rw(b,c->ds,0x49d8u);wp=at(ws,(uint16_t)(wo+0x05aau));wp_next=at(ws,(uint16_t)(wo+0x05abu));
    for(i=0;i<8u;i++){size_t p=(i<4u?ptr+i:window_ptr+i-4u)&MASK;
        if(i<4u&&(p==wp||p==wp_next))return 0;
        for(delta=-0x100;delta<=3;delta++){size_t q=at(c->ss,(uint16_t)(c->sp+delta));
            for(j=0;j<2u;j++)if(p==((q+j)&MASK))return 0;}}
    return ordinary(rw(b,c->ds,0x6a5eu));
}
static pr_u68_prompt_status after_prompt(pr_u68_prompt*s,pr_u68_prompt_diagnostic*d){
    pr_bus*b=s->bus;pr_r0899_cpu*c=s->cpu;uint16_t rel,cs,sp;uint8_t v;pr_u68_068d_cpu t;
    if(c->cs!=s->caller_cs||c->ip!=0x09d5u||c->bp!=s->caller_bp||c->sp!=s->caller_sp)return finish(s,PR_U68_PROMPT_TRANSFER);
    d->stage=3;wb(b,c->ss,(uint16_t)(c->bp-3u),(uint8_t)c->ax);v=rb(b,c->ss,(uint16_t)(c->bp-4u));cmp8(c,v,0);
    if(!v){c->ip=0x09e1u;return finish(s,PR_U68_PROMPT_OUTSIDE);}
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-3u)));cmp8(c,(uint8_t)c->ax,0x48u);
    if((uint8_t)c->ax!=0x48u){c->ip=0x0abcu;return finish(s,PR_U68_PROMPT_OUTSIDE);}
    push(b,c,c->cs);c->ip=0x0ab2u;rel=rw(b,c->cs,0x0ab3u);
    if(rb(b,c->cs,0x0ab2u)!=0xe8u||(uint16_t)(0x0ab5u+rel)!=0x068du)return finish(s,PR_U68_PROMPT_CODE);
    if(!preflight_safe(b,c))return finish(s,PR_U68_PROMPT_PROFILE);
    cs=c->cs;sp=(uint16_t)(c->sp+2u);push(b,c,0x0ab5u);c->ip=0x068du;
    {FIELDS(TO)}d->preflight_status=pr_u68_up_preflight_068d_cpu(b,PR_BUS_RAM_SIZE,&t);{FIELDS(FROM)}
    if(d->preflight_status!=PR_U68_068D_RETURNED)return finish(s,PR_U68_PROMPT_CALLEE);
    if(c->cs!=cs||c->ip!=0x0ab5u||c->sp!=sp)return finish(s,PR_U68_PROMPT_TRANSFER);
    wb(b,c->ss,(uint16_t)(c->bp-2u),1);c->ip=0x0b9bu;d->stage=4;return finish(s,PR_U68_PROMPT_DISPLAY);
}
static pr_u68_prompt_status session_result(pr_u68_prompt*s,pr_u68_prompt_diagnostic*d,pr_u3b0_session_status st){
    d->session_status=st;if(st==PR_U3B0_SESSION_YIELD){s->phase=PR_U68_PROMPT_WAITING;return PR_U68_PROMPT_YIELD;}
    if(st==PR_U3B0_SESSION_RETURN)return after_prompt(s,d);
    return finish(s,PR_U68_PROMPT_STOP);
}
pr_u68_prompt_status pr_u68_prompt_start(pr_u68_prompt*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u68_prompt_diagnostic*d){
    uint16_t cs,sp,stub,target;uint8_t v;pr_system_leaf_cpu t;pr_u3b0_session_status st;
    if(!s||!b||!b->ram||!b->ega||!c||!e||!d||s->phase!=PR_U68_PROMPT_EMPTY||n!=PR_BUS_RAM_SIZE||c->ip!=0x098fu||
       c->bp<0x0090u||c->bp>0xffe3u||c->sp<0x01ecu||c->sp>0xfffcu||(uint16_t)(c->bp-0x3cu)!=c->sp||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss)||
       e->draw.pass.target.cs!=c->cs||e->draw.pass.target.ip!=0x09d5u)return PR_U68_PROMPT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->env=e;s->caller_cs=c->cs;s->caller_bp=c->bp;s->caller_sp=c->sp;
    c->di=rw(b,c->ds,0x49d6u);c->es=rw(b,c->ds,0x49d8u);xorax(c);
    if((uint16_t)(c->di+0x0592u)==0xffffu){c->ip=0x0995u;return finish(s,PR_U68_PROMPT_PROFILE);}
    ww(b,c->es,(uint16_t)(c->di+0x0592u),c->ax);
    v=rb(b,c->ds,0x49fau);c->ax=(uint16_t)((c->ax&0xff00u)|v);cmp8(c,v,1);
    if(v!=1){c->ip=0x0bacu;return finish(s,PR_U68_PROMPT_OUTSIDE);}
    wb(b,c->ss,(uint16_t)(c->bp-2u),0);c->di=(uint16_t)(c->bp-0x33u);push(b,c,c->ss);push(b,c,c->di);
    c->di=0x097au;push(b,c,c->cs);push(b,c,c->di);c->ip=0x09b2u;d->stage=1;
    if(rb(b,c->cs,0x09b2u)!=0x9au||rw(b,c->cs,0x09b3u)!=0x0b0bu)return finish(s,PR_U68_PROMPT_CODE);
    cs=c->cs;sp=(uint16_t)(c->sp+4u);push(b,c,cs);push(b,c,0x09b7u);c->cs=rw(b,cs,0x09b5u);c->ip=0x0b0bu;
    {FIELDS(TO)}d->strload_status=pr_system_strload_0b0b(b->ram,n,&t);{FIELDS(FROM)}
    if(d->strload_status!=PR_SYSTEM_LEAF_RETURNED)return finish(s,PR_U68_PROMPT_CALLEE);
    if(c->cs!=cs||c->ip!=0x09b7u||c->sp!=sp)return finish(s,PR_U68_PROMPT_TRANSFER);
    c->di=0x04b6u;push(b,c,c->ds);push(b,c,c->di);
    c->ax=(uint16_t)((c->ax&0xff00u)|0x0du);push(b,c,c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|0x0au);push(b,c,c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|0x0fu);push(b,c,c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|1u);push(b,c,c->ax);
    c->ax&=0xff00u;push(b,c,c->ax);c->di=(uint16_t)(c->bp-4u);push(b,c,c->ss);push(b,c,c->di);c->ip=0x09d0u;d->stage=2;
    if(rb(b,c->cs,0x09d0u)!=0x9au||rw(b,c->cs,0x09d1u)!=0x002au)return finish(s,PR_U68_PROMPT_CODE);
    cs=c->cs;push(b,c,cs);push(b,c,0x09d5u);stub=rw(b,cs,0x09d3u);c->cs=stub;c->ip=0x002au;
    if(!ordinary(stub))return finish(s,PR_U68_PROMPT_PROFILE);
    if(rb(b,stub,0x002au)!=0xeau||rw(b,stub,0x002bu)!=0x03bdu)return finish(s,PR_U68_PROMPT_OVERLAY);
    target=rw(b,stub,0x002du);if(!ordinary(target))return finish(s,PR_U68_PROMPT_PROFILE);
    c->cs=target;c->ip=0x03bdu;pr_u3b0_prompt_session_init(&s->prompt);
    st=pr_u3b0_prompt_session_start(&s->prompt,b,n,c,e,&d->session);return session_result(s,d,st);
}
pr_u68_prompt_status pr_u68_prompt_resume(pr_u68_prompt*s,pr_u68_prompt_diagnostic*d){pr_u3b0_session_status st;
    if(!s||!d||s->phase!=PR_U68_PROMPT_WAITING)return PR_U68_PROMPT_ARGUMENT;
    memset(d,0,sizeof *d);d->stage=2;st=pr_u3b0_prompt_session_resume(&s->prompt,&d->session);
    if(st==PR_U3B0_SESSION_ARGUMENT)return PR_U68_PROMPT_ARGUMENT;
    return session_result(s,d,st);
}
