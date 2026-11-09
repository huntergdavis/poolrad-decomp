#include "poolrad/replay/u3b0_clock_draw_0432.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t addr(uint16_t s,uint16_t o){return (((size_t)s<<4u)+o)&MASK;}
static uint8_t rb(pr_bus*b,uint16_t s,uint16_t o){return pr_bus_read8(b,addr(s,o));}
static uint16_t rw(pr_bus*b,uint16_t s,uint16_t o){size_t a=addr(s,o);uint8_t l=pr_bus_read8(b,a),h=pr_bus_read8(b,(a+1u)&MASK);return (uint16_t)(l|((uint16_t)h<<8u));}
static void ww(pr_bus*b,uint16_t s,uint16_t o,uint16_t v){size_t a=addr(s,o);pr_bus_write8(b,a,(uint8_t)v);pr_bus_write8(b,(a+1u)&MASK,(uint8_t)(v>>8u));}
static void push(pr_bus*b,pr_r0899_cpu*c,uint16_t v){c->sp=(uint16_t)(c->sp-2u);ww(b,c->ss,c->sp,v);}
static int ordinary(uint16_t s){return s<=0x9000u||s>=0xb000u;}
static uint16_t add(pr_r0899_cpu*c,uint16_t a,uint16_t b,unsigned carry){
    unsigned x=(unsigned)a+b+carry,r=x&0xffffu,p=r&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(x>0xffffu?1u:0u)|((p&1u)?0u:4u)|
        (((a^b^r)&0x10u)?0x10u:0u)|(r==0u?0x40u:0u)|(r&0x8000u?0x80u:0u)|
        ((~(a^b)&(a^r)&0x8000u)?0x800u:0u));return (uint16_t)r;
}
static void cmp_byte_zero(pr_r0899_cpu*c,uint8_t v){unsigned p=v;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|((p&1u)?0u:4u)|(v==0?0x40u:0u)|(v&0x80u?0x80u:0u));}
#define FIELDS(X) X(ax) X(bx) X(cx) X(dx) X(si) X(di) X(bp) X(sp) X(cs) X(ip) X(ds) X(es) X(ss) X(flags)
#define COPY_TO(f) t.f=c->f;
#define COPY_FROM(f) c->f=t.f;
/* Check opcode/offset before executing the call; segment is read live after the
 * return frame pushes, matching the established replay call-site convention. */
static int call(pr_bus*b,pr_r0899_cpu*c,uint16_t site,uint16_t target){uint16_t seg;
    c->ip=site;if(rb(b,c->cs,site)!=0x9au||rw(b,c->cs,(uint16_t)(site+1u))!=target)return 0;
    push(b,c,c->cs);push(b,c,(uint16_t)(site+5u));seg=rw(b,c->cs,(uint16_t)(site+3u));c->cs=seg;c->ip=target;return 1;}
static pr_u3b0_clock_draw_status clock_call(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u3b0_clock_draw_diagnostic*d,uint16_t site){
    pr_r08ae_time_cpu t;uint16_t cs=c->cs,sp=c->sp;
    if(!call(b,c,site,0x0c3fu))return PR_U3B0_CLOCK_DRAW_CODE;
    {FIELDS(COPY_TO)}
    d->clock_status=pr_r0709_time_0c3f_cpu(b,n,&t,e->time_service,e->time_context,&d->clock);
    {FIELDS(COPY_FROM)}
    if(d->clock_status!=PR_R08AE_TIME_CPU_OK)return PR_U3B0_CLOCK_DRAW_CALLEE;
    return c->cs==cs&&c->ip==(uint16_t)(site+5u)&&c->sp==sp?PR_U3B0_CLOCK_DRAW_REACHED:PR_U3B0_CLOCK_DRAW_TRANSFER;
}
pr_u3b0_clock_draw_status pr_u3b0_clock_draw_0432(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u3b0_clock_draw_diagnostic*d){
    pr_u3b0_clock_draw_status st;uint16_t cs,sp;uint8_t v;
    if(!b||!b->ram||!b->ega||!c||!e||!d||!e->time_service||n!=PR_BUS_RAM_SIZE||c->ip!=0x0432u||
       c->bp<0x0090u||c->bp>0xffe3u||c->sp<0x0140u||c->sp>0xfffcu||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_U3B0_CLOCK_DRAW_ARGUMENT;
    memset(d,0,sizeof *d);d->phase=0;st=clock_call(b,n,c,e,d,0x0432u);if(st)return st;
    ww(b,c->ss,(uint16_t)(c->bp-0x57u),c->ax);ww(b,c->ss,(uint16_t)(c->bp-0x55u),c->dx);
    d->phase=1;st=clock_call(b,n,c,e,d,0x043du);if(st)return st;
    c->ax=add(c,c->ax,0x1eu,0);c->dx=add(c,c->dx,0,(c->flags&1u));
    ww(b,c->ss,(uint16_t)(c->bp-0x5bu),c->ax);ww(b,c->ss,(uint16_t)(c->bp-0x59u),c->dx);
    c->ax=rw(b,c->ss,(uint16_t)(c->bp-0x5bu));c->dx=rw(b,c->ss,(uint16_t)(c->bp-0x59u));
    c->ax=add(c,c->ax,0x32u,0);c->dx=add(c,c->dx,0,(c->flags&1u));
    ww(b,c->ss,(uint16_t)(c->bp-0x5fu),c->ax);ww(b,c->ss,(uint16_t)(c->bp-0x5du),c->dx);
    v=rb(b,c->ss,(uint16_t)(c->bp-0x2au));cmp_byte_zero(c,v);
    if(v){pr_r0709_printer_cpu t;d->phase=2;
        c->ax&=0xff00u;push(b,c,c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|0x18u);push(b,c,c->ax);
        c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp+0x12u)));push(b,c,c->ax);
        c->di=(uint16_t)(c->bp-0x2au);push(b,c,c->ss);push(b,c,c->di);cs=c->cs;sp=(uint16_t)(c->sp+10u);
        if(!call(b,c,0x0475u,0x0626u))return PR_U3B0_CLOCK_DRAW_CODE;
        {FIELDS(COPY_TO)}
        d->printer_status=pr_r0709_printer_0626_cpu(b,n,&t,e->printer_loopbacks,e->glyph_loopbacks,&d->printer);
        {FIELDS(COPY_FROM)}
        if(d->printer_status!=PR_R0709_PRINTER_CPU_OK)return PR_U3B0_CLOCK_DRAW_CALLEE;
        if(c->cs!=cs||c->ip!=0x047au||c->sp!=sp)return PR_U3B0_CLOCK_DRAW_TRANSFER;
    }
    c->ip=0x047au;d->phase=3;d->draw_status=pr_u3b0_draw_wait_047a(b,n,c,&e->draw,&d->draw);
    return PR_U3B0_CLOCK_DRAW_REACHED;
}
