#include "poolrad/replay/u3b0_entry_03bd.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t at(uint16_t s,uint16_t o){return (((size_t)s<<4u)+o)&MASK;}
static uint8_t rb(pr_bus*b,uint16_t s,uint16_t o){return pr_bus_read8(b,at(s,o));}
static uint16_t rw(pr_bus*b,uint16_t s,uint16_t o){size_t a=at(s,o);uint8_t l=pr_bus_read8(b,a),h=pr_bus_read8(b,(a+1u)&MASK);return (uint16_t)(l|((uint16_t)h<<8u));}
static void wb(pr_bus*b,uint16_t s,uint16_t o,uint8_t v){pr_bus_write8(b,at(s,o),v);}
static void ww(pr_bus*b,uint16_t s,uint16_t o,uint16_t v){size_t a=at(s,o);pr_bus_write8(b,a,(uint8_t)v);pr_bus_write8(b,(a+1u)&MASK,(uint8_t)(v>>8u));}
static void push(pr_bus*b,pr_r0899_cpu*c,uint16_t v){c->sp=(uint16_t)(c->sp-2u);ww(b,c->ss,c->sp,v);}
/* Excludes any aperture byte even when a 16-bit word crosses the end of a segment window. */
static int ordinary(uint16_t s){return s<0x9000u||s>=0xb000u;}
static uint16_t sub(pr_r0899_cpu*c,uint16_t a,uint16_t b,unsigned bits){unsigned m=bits==8?0xffu:0xffffu,sign=bits==8?0x80u:0x8000u,r=(a-b)&m,p=r&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(a<b?1u:0u)|((p&1u)?0u:4u)|(((a^b^r)&0x10u)?0x10u:0u)|(r==0?0x40u:0u)|(r&sign?0x80u:0u)|(((a^b)&(a^r)&sign)?0x800u:0u));return (uint16_t)r;}
#define FIELDS(X) X(ax) X(bx) X(cx) X(dx) X(si) X(di) X(bp) X(sp) X(cs) X(ip) X(ds) X(es) X(ss) X(flags)
#define TO(f) t.f=c->f;
#define FROM(f) c->f=t.f;
static pr_u3b0_entry_status store(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_u3b0_entry_diagnostic*d,uint16_t site,uint16_t srcarg,uint16_t dest){
    uint16_t srcseg,cs,sp;pr_system_0b25_cpu t;
    c->di=rw(b,c->ss,(uint16_t)(c->bp+srcarg));c->es=rw(b,c->ss,(uint16_t)(c->bp+srcarg+2u));
    push(b,c,c->es);push(b,c,c->di);c->di=(uint16_t)(c->bp-dest);push(b,c,c->ss);push(b,c,c->di);c->ax=0x28u;push(b,c,c->ax);c->ip=site;
    if(rb(b,c->cs,site)!=0x9au||rw(b,c->cs,(uint16_t)(site+1u))!=0x0b25u)return PR_U3B0_ENTRY_CODE;
    /* Read the live pushed source segment: flat-RAM leaves cannot touch EGA. */
    srcseg=rw(b,c->ss,(uint16_t)(c->sp+8u));if(!ordinary(srcseg))return PR_U3B0_ENTRY_PROFILE;
    cs=c->cs;sp=(uint16_t)(c->sp+10u);push(b,c,cs);push(b,c,(uint16_t)(site+5u));c->cs=rw(b,cs,(uint16_t)(site+3u));c->ip=0x0b25u;
    {FIELDS(TO)} d->store_status=pr_system_strstore_0b25(b->ram,n,&t); {FIELDS(FROM)}
    if(d->store_status!=PR_SYSTEM_0B25_RETURNED)return PR_U3B0_ENTRY_CALLEE;
    return c->cs==cs&&c->ip==(uint16_t)(site+5u)&&c->sp==sp?PR_U3B0_ENTRY_REACHED:PR_U3B0_ENTRY_TRANSFER;
}
pr_u3b0_entry_status pr_u3b0_entry_03bd(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u3b0_entry_diagnostic*d){
    pr_u3b0_entry_status st;uint8_t v;uint16_t cs,sp,rel;pr_u3b0_00cb_cpu t;
    if(!b||!b->ram||!b->ega||!c||!e||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x03bdu||c->sp<0x01d2u||c->sp>0xffe4u||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_U3B0_ENTRY_ARGUMENT;
    memset(d,0,sizeof *d);d->split_callee_status=-1;push(b,c,c->bp);c->bp=c->sp;c->sp=sub(c,c->sp,0x90u,16);
    d->phase=0;st=store(b,n,c,d,0x03d2u,0x18u,0x2au);if(st)return st;
    d->phase=1;st=store(b,n,c,d,0x03e5u,0x14u,0x53u);if(st)return st;
    wb(b,c->ds,0x6b2cu,0);v=rb(b,c->ss,(uint16_t)(c->bp+0x10u));sub(c,v,0,8);
    if(!v){v=rb(b,c->ss,(uint16_t)(c->bp+0x0eu));sub(c,v,0,8);}
    c->ax=(uint16_t)((c->ax&0xff00u)|(v?1u:0u));wb(b,c->ss,(uint16_t)(c->bp-0x8fu),(uint8_t)c->ax);
    c->di=(uint16_t)(c->bp-0x53u);push(b,c,c->ss);push(b,c,c->di);push(b,c,c->bp);push(b,c,c->cs);
    d->phase=2;c->ip=0x040cu;rel=rw(b,c->cs,0x040du);
    if(rb(b,c->cs,0x040cu)!=0xe8u||(uint16_t)(0x040fu+rel)!=0x00cbu)return PR_U3B0_ENTRY_CODE;
    cs=c->cs;sp=(uint16_t)(c->sp+8u);push(b,c,0x040fu);c->ip=0x00cbu;
    {FIELDS(TO)} d->split_status=pr_u3b0_item_split_00cb_cpu(b->ram,n,&t,&d->split_callee_status); {FIELDS(FROM)}
    if(d->split_status!=PR_U3B0_00CB_OK)return PR_U3B0_ENTRY_CALLEE;
    if(c->cs!=cs||c->ip!=0x040fu||c->sp!=sp)return PR_U3B0_ENTRY_TRANSFER;
    v=rb(b,c->ds,0x6b2bu);c->ax=(uint16_t)((c->ax&0xff00u)|v);
    sub(c,v,rb(b,c->ss,(uint16_t)(c->bp-0x8eu)),8);if(!(c->flags&1u)&&!(c->flags&0x40u))wb(b,c->ds,0x6b2bu,1);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-0x53u)));wb(b,c->ss,(uint16_t)(c->bp-0x64u),(uint8_t)c->ax);
    wb(b,c->ss,(uint16_t)(c->bp-0x60u),0);c->di=rw(b,c->ss,(uint16_t)(c->bp+6u));c->es=rw(b,c->ss,(uint16_t)(c->bp+8u));wb(b,c->es,c->di,0);
    wb(b,c->ss,(uint16_t)(c->bp-0x63u),0);c->ip=0x0432u;d->phase=3;
    d->clock_draw_status=pr_u3b0_clock_draw_0432(b,n,c,e,&d->clock_draw);return PR_U3B0_ENTRY_REACHED;
}
