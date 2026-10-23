/* Complete U3B0 +01DA; see U3B0_ENTRY_01DA_CPU.md. */
#include "poolrad/recovered/overlay_u3b0_entry_01da_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#define MASK ((1u<<20)-1u)
typedef struct { pr_bus *b; pr_u3b0_01da_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
/* DS/SS and all flat-RAM callee operands are outside the EGA aperture. */
static uint16_t at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return at(m->b->ram,physical(s,o)); }
static uint8_t rb(machine *m,uint16_t s,uint16_t o) { return m->b->ram[physical(s,o)]; }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) { size_t p=physical(s,o);m->b->ram[p]=(uint8_t)v;m->b->ram[(p+1)&MASK]=(uint8_t)(v>>8); }
static void wb(machine *m,uint16_t s,uint16_t o,uint8_t v) { m->b->ram[physical(s,o)]=v; }
static uint16_t local(machine *m,int d) { return (uint16_t)(m->c->bp+d); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void al(pr_u3b0_01da_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void arithmetic(pr_u3b0_01da_cpu *c,uint16_t a,uint16_t b,unsigned bits,int subtract,int increment) {
    uint32_t mask=bits==8?255u:65535u,sign=bits==8?128u:32768u;
    uint32_t full=subtract?(uint32_t)a-b:(uint32_t)a+b,r=full&mask;
    c->flags&=(uint16_t)~(increment?0x8D4u:0x8D5u);
    if(!increment && (subtract?a<b:full>mask))c->flags|=1;
    if(parity((uint8_t)r))c->flags|=4;
    if((a^b^r)&0x10)c->flags|=0x10;
    if(r==0)c->flags|=0x40;
    if(r&sign)c->flags|=0x80;
    if((subtract?(a^b):~(a^b))&(a^r)&sign)c->flags|=0x800;
}
#define COPY(to,from) do {     (to).ax=(from).ax; (to).bx=(from).bx; (to).cx=(from).cx;     (to).dx=(from).dx; (to).si=(from).si; (to).di=(from).di;     (to).bp=(from).bp; (to).sp=(from).sp; (to).cs=(from).cs;     (to).ip=(from).ip; (to).ds=(from).ds; (to).es=(from).es;     (to).ss=(from).ss; (to).flags=(from).flags; } while(0)

static void zero_ah(pr_u3b0_01da_cpu *c) { c->ax&=255u;c->flags=(uint16_t)((c->flags&~0x8D5u)|0x44u); }
static uint16_t link(machine *m) { m->c->di=rd(m,m->c->ss,local(m,6));return m->c->di; }
static void add_di(machine *m) { arithmetic(m->c,m->c->di,m->c->ax,16,0,0);m->c->di=(uint16_t)(m->c->di+m->c->ax); }
static void add_ax(pr_u3b0_01da_cpu *c,uint16_t v) { arithmetic(c,c->ax,v,16,0,0);c->ax=(uint16_t)(c->ax+v); }
static void sub_ax(pr_u3b0_01da_cpu *c,uint16_t v) { arithmetic(c,c->ax,v,16,1,0);c->ax=(uint16_t)(c->ax-v); }
static void selection(machine *m) {
    al(m->c,rb(m,m->c->ss,local(m,8)));zero_ah(m->c);
    arithmetic(m->c,m->c->ax,m->c->ax,16,0,0);m->c->ax=(uint16_t)(m->c->ax<<1);m->c->flags|=0x10u; /* SHL AX,1 via ADD AX,AX; pinned SHL sets AF */
    link(m);add_di(m);
}
static void character(machine *m) {
    al(m->c,rb(m,m->c->ss,local(m,-1)));zero_ah(m->c);link(m);add_di(m);
    al(m->c,rb(m,m->c->ss,(uint16_t)(m->c->di-0x53u)));
}
static pr_u3b0_01da_status call(machine *m,uint16_t site,uint16_t target,int which,
    const pr_u3b0_01da_limits *l,pr_u3b0_01da_diagnostic *d)
{
    pr_u3b0_01da_cpu *c=m->c;uint16_t caller=c->cs,next=(uint16_t)(site+5u),seg;
    c->ip=site;
    if(rb(m,caller,site)!=0x9A || rd(m,caller,(uint16_t)(site+1u))!=target)return PR_U3B0_01DA_CODE;
    seg=rd(m,caller,(uint16_t)(site+3u));push(m,caller);push(m,next);c->cs=seg;c->ip=target;d->callee=which;
    if(which==PR_U3B0_01DA_SET) {
        pr_system_leaf_cpu inner;COPY(inner,*c);
        d->status=(int)pr_system_set_in_0dc0(m->b->ram,1u<<20,&inner);COPY(*c,inner);
    } else {
        pr_r0709_glyph_cpu inner;COPY(inner,*c);
        d->status=(int)pr_r0709_glyph_01df_cpu(m->b,1u<<20,&inner,l->glyph_loopbacks,&d->glyph);COPY(*c,inner);
    }
    if(d->status)return PR_U3B0_01DA_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U3B0_01DA_TRANSFER;
    return PR_U3B0_01DA_OK;
}
static pr_u3b0_01da_status emit(machine *m,uint16_t site,int color_disp,
    const pr_u3b0_01da_limits *l,pr_u3b0_01da_diagnostic *d)
{
    pr_u3b0_01da_cpu *c=m->c;
    al(c,rb(m,c->ss,local(m,-1)));zero_ah(c);c->dx=c->ax;
    al(c,rb(m,c->ss,(uint16_t)(link(m)-0x65u)));zero_ah(c);add_ax(c,c->dx);
    arithmetic(c,c->ax,1,16,1,1);c->ax=(uint16_t)(c->ax-1u);push(m,c->ax);
    al(c,24);push(m,c->ax);al(c,rb(m,c->ss,(uint16_t)(link(m)+color_disp)));push(m,c->ax);
    al(c,1);push(m,c->ax);character(m);push(m,c->ax);al(c,1);push(m,c->ax);
    return call(m,site,0x1DF,PR_U3B0_01DA_GLYPH,l,d);
}
static void tail_sum(machine *m) {
    al(m->c,rb(m,m->c->ss,(uint16_t)(link(m)-0x64u)));zero_ah(m->c);m->c->dx=m->c->ax;
    al(m->c,rb(m,m->c->ss,(uint16_t)(link(m)-0x65u)));zero_ah(m->c);add_ax(m->c,m->c->dx);
}
static int ordinary(uint16_t s) { return s<=0x9000 || s>=0xB000; }
pr_u3b0_01da_status pr_u3b0_entry_01da_cpu(pr_bus *b,size_t size,pr_u3b0_01da_cpu *c,
    const pr_u3b0_01da_limits *l,pr_u3b0_01da_diagnostic *d)
{
    pr_u3b0_01da_diagnostic fallback={-1,-1,{-1,-1}};machine m={b,c};
    pr_u3b0_01da_status status;uint16_t v;size_t loops=0,p;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(b==NULL || b->ram==NULL || b->ega==NULL || c==NULL || l==NULL || size!=(1u<<20) ||
       c->ip!=0x1DA || c->sp<0x40 || c->sp>0xFFF4 || !ordinary(c->ds) || !ordinary(c->ss))return PR_U3B0_01DA_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;arithmetic(c,c->sp,2,16,1,0);c->sp=(uint16_t)(c->sp-2u);
    v=rb(&m,c->ss,(uint16_t)(link(&m)-0x64u));arithmetic(c,v,0,8,1,0);
    if(c->flags&0x40u)goto done;
    al(c,rb(&m,c->ss,(uint16_t)(link(&m)-0x64u)));wb(&m,c->ss,local(&m,-2),(uint8_t)c->ax);
    al(c,1);arithmetic(c,(uint8_t)c->ax,rb(&m,c->ss,local(&m,-2)),8,1,0);
    if(!(c->flags&0x41u))goto tail;
    wb(&m,c->ss,local(&m,-1),(uint8_t)c->ax);
    for(;;) {
        selection(&m);al(c,rb(&m,c->ss,(uint16_t)(c->di-0x8Fu)));
        arithmetic(c,(uint8_t)c->ax,rb(&m,c->ss,local(&m,-1)),8,1,0);
        if(!(c->flags&0x41u))goto classify;
        selection(&m);al(c,rb(&m,c->ss,(uint16_t)(c->di-0x8Eu)));
        arithmetic(c,(uint8_t)c->ax,rb(&m,c->ss,local(&m,-1)),8,1,0);
        if(c->flags&1u)goto classify;
        v=rb(&m,c->ss,(uint16_t)(link(&m)+14u));arithmetic(c,v,0,8,1,0);
        if(c->flags&0x40u)goto classify;
        status=emit(&m,0x273,14,l,d);if(status)return status;
        goto next;
classify:
        character(&m);push(&m,c->ax);c->di=0x1BA;push(&m,c->cs);push(&m,c->di);
        status=call(&m,0x28F,0xDC0,PR_U3B0_01DA_SET,l,d);if(status)return status;
        if(c->flags&0x40u)status=emit(&m,0x305,16,l,d);
        else status=emit(&m,0x2CA,14,l,d);
        if(status)return status;
next:
        al(c,rb(&m,c->ss,local(&m,-1)));arithmetic(c,(uint8_t)c->ax,rb(&m,c->ss,local(&m,-2)),8,1,0);
        if(c->flags&0x40u)break;
        c->ip=0x312;if(loops==l->caller_loopbacks)return PR_U3B0_01DA_LIMIT;++loops;
        v=rb(&m,c->ss,local(&m,-1));arithmetic(c,v,1,8,0,1);wb(&m,c->ss,local(&m,-1),(uint8_t)(v+1u));
    }
tail:
    tail_sum(&m);arithmetic(c,c->ax,39,16,1,0);
    if(!!(c->flags&0x80u)==!!(c->flags&0x800u))goto done;
    tail_sum(&m);push(&m,c->ax);al(c,24);push(&m,c->ax);al(c,0);push(&m,c->ax);
    al(c,rb(&m,c->ss,(uint16_t)(link(&m)-0x65u)));zero_ah(c);c->cx=c->ax;
    al(c,rb(&m,c->ss,(uint16_t)(link(&m)-0x64u)));zero_ah(c);c->dx=c->ax;
    c->ax=39;sub_ax(c,c->dx);sub_ax(c,c->cx);arithmetic(c,c->ax,1,16,0,1);c->ax=(uint16_t)(c->ax+1u);push(&m,c->ax);
    al(c,0x20);push(&m,c->ax);al(c,1);push(&m,c->ax);status=call(&m,0x372,0x1DF,PR_U3B0_01DA_GLYPH,l,d);if(status)return status;
done:
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=at(b->ram,p);c->cs=at(b->ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+8u);
    return PR_U3B0_01DA_OK;
}
