/* Complete U7D +05F0; see U7D_ENTRY_05F0_CPU.md. */
#include "poolrad/recovered/overlay_u7d_entry_05f0_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#define MASK ((1u<<20)-1u)
typedef struct { pr_bus *b; pr_u7d_05f0_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
/* DS/SS and all flat-RAM callee operands are outside the EGA aperture. */
static uint16_t at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return at(m->b->ram,physical(s,o)); }
static uint8_t rb(machine *m,uint16_t s,uint16_t o) { return m->b->ram[physical(s,o)]; }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) { size_t p=physical(s,o);m->b->ram[p]=(uint8_t)v;m->b->ram[(p+1)&MASK]=(uint8_t)(v>>8); }
static void wb(machine *m,uint16_t s,uint16_t o,uint8_t v) { m->b->ram[physical(s,o)]=v; }
static uint16_t local(machine *m,int d) { return (uint16_t)(m->c->bp+d); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void al(pr_u7d_05f0_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void arithmetic(pr_u7d_05f0_cpu *c,uint16_t a,uint16_t b,unsigned bits,int subtract,int increment) {
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
static void zero_ah(pr_u7d_05f0_cpu *c) { c->ax&=255u;c->flags=(uint16_t)((c->flags&~0x8D5u)|0x44u); }
static void cbw(pr_u7d_05f0_cpu *c) { c->ax=(uint16_t)((c->ax&0x80u)?c->ax|0xFF00u:c->ax&255u); }
static pr_u7d_05f0_status call(machine *m,uint16_t site,uint16_t target,int near_call,
    pr_u7d_05f0_callee which,const pr_u7d_05f0_limits *l,pr_u7d_05f0_diagnostic *d)
{
    pr_u7d_05f0_cpu *c=m->c;uint16_t caller=c->cs,next=(uint16_t)(site+(near_call?3u:5u)),seg=caller;
    c->ip=site;
    if(rb(m,caller,site)!=(near_call?0xE8:0x9A) ||
       rd(m,caller,(uint16_t)(site+1u))!=(near_call?(uint16_t)(target-next):target))return PR_U7D_05F0_CODE;
    if(!near_call) { seg=rd(m,caller,(uint16_t)(site+3u));push(m,caller); }
    push(m,next);c->cs=seg;c->ip=target;d->callee=(int)which;
    if(which==PR_U7D_05F0_LOAD) {
        pr_system_leaf_cpu inner;COPY(inner,*c);
        d->status=(int)pr_system_strload_0b0b(m->b->ram,1u<<20,&inner);COPY(*c,inner);
    } else if(which==PR_U7D_05F0_FORMAT) {
        pr_u7d_0565_cpu inner;COPY(inner,*c);
        d->status=(int)pr_u7d_two_digits_0565_cpu(m->b->ram,1u<<20,&inner,&d->format_callee,&d->format_status);COPY(*c,inner);
    } else {
        pr_r0709_printer_cpu inner;COPY(inner,*c);
        d->status=(int)pr_r0709_printer_0626_cpu(m->b,1u<<20,&inner,l->printer_loopbacks,l->glyph_loopbacks,&d->printer);COPY(*c,inner);
    }
    if(d->status)return PR_U7D_05F0_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U7D_05F0_TRANSFER;
    return PR_U7D_05F0_OK;
}
#define CALL(site,target,near,which) do { status=call(&m,site,target,near,which,l,d);if(status)return status; } while(0)
static void twice_di(pr_u7d_05f0_cpu *c) {
    uint16_t v=c->di;arithmetic(c,v,v,16,0,0);c->di=(uint16_t)(v<<1);
    c->flags&=(uint16_t)~0x10u; /* normal/386 SHL1 clears undefined AF. */
}
static void push_local(machine *m,int displacement) {
    m->c->di=local(m,displacement);push(m,m->c->ss);push(m,m->c->di);
}
static void inc_ax(pr_u7d_05f0_cpu *c) { arithmetic(c,c->ax,1,16,0,1);c->ax=(uint16_t)(c->ax+1u); }
static int ordinary(uint16_t s) { return s<=0x9000 || s>=0xB000; }
pr_u7d_05f0_status pr_u7d_entry_05f0_cpu(pr_bus *b,size_t size,pr_u7d_05f0_cpu *c,
    const pr_u7d_05f0_limits *l,pr_u7d_05f0_diagnostic *d)
{
    pr_u7d_05f0_diagnostic fallback={-1,-1,PR_U7D_0565_NO_CALLEE,-1,{-1,-1,-1,-1}};machine m={b,c};
    pr_u7d_05f0_status status;size_t p,loops=0;uint16_t v;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(b==NULL || b->ram==NULL || b->ega==NULL || c==NULL || l==NULL || size!=(1u<<20) ||
       c->ip!=0x5F0 || c->sp<0x500 || c->sp>0xFFF6 || !ordinary(c->ds) || !ordinary(c->ss))return PR_U7D_05F0_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;arithmetic(c,c->sp,0x110,16,1,0);c->sp=(uint16_t)(c->sp-0x110u);
    wb(&m,c->ss,local(&m,-1),0);
    for(;;) {
        al(c,rb(&m,c->ss,local(&m,-1)));cbw(c);c->di=c->ax;twice_di(c);
        wr(&m,c->ss,(uint16_t)(c->bp+c->di-15u),10);
        v=rb(&m,c->ss,local(&m,-1));arithmetic(c,v,6,8,1,0);if(c->flags&0x40u)break;
        c->ip=0x611;if(loops==l->caller_loopbacks)return PR_U7D_05F0_LIMIT;++loops;
        v=rb(&m,c->ss,local(&m,-1));arithmetic(c,v,1,8,0,1);wb(&m,c->ss,local(&m,-1),(uint8_t)(v+1u));
    }
    al(c,rb(&m,c->ss,local(&m,6)));cbw(c);c->di=c->ax;twice_di(c);wr(&m,c->ss,(uint16_t)(c->bp+c->di-15u),15);
    al(c,1);push(&m,c->ax);al(c,0x11);push(&m,c->ax);al(c,10);push(&m,c->ax);
    push_local(&m,-0x1B);c->di=0x5E3;push(&m,c->cs);push(&m,c->di);
    CALL(0x633,0xB0B,0,PR_U7D_05F0_LOAD);CALL(0x638,0x626,0,PR_U7D_05F0_PRINTER);
    wb(&m,c->ss,local(&m,-0x10),11);wb(&m,c->ss,local(&m,-1),4);
    for(;;) {
        al(c,rb(&m,c->ss,local(&m,-0x10)));zero_ah(c);inc_ax(c);push(&m,c->ax);al(c,0x11);push(&m,c->ax);
        al(c,rb(&m,c->ss,local(&m,-1)));cbw(c);c->di=c->ax;twice_di(c);
        al(c,rb(&m,c->ss,(uint16_t)(c->bp+c->di-15u)));push(&m,c->ax);push_local(&m,-0x110);
        al(c,rb(&m,c->ss,local(&m,-1)));cbw(c);c->di=c->ax;twice_di(c);al(c,rb(&m,c->ds,(uint16_t)(c->di+0x6DC2u)));
        push(&m,c->ax);push(&m,c->cs);CALL(0x674,0x565,1,PR_U7D_05F0_FORMAT);CALL(0x677,0x626,0,PR_U7D_05F0_PRINTER);
        al(c,rb(&m,c->ss,local(&m,-0x10)));zero_ah(c);inc_ax(c);inc_ax(c);inc_ax(c);push(&m,c->ax);
        al(c,0x11);push(&m,c->ax);al(c,10);push(&m,c->ax);push_local(&m,-0x12);
        c->di=0x5EE;push(&m,c->cs);push(&m,c->di);CALL(0x695,0xB0B,0,PR_U7D_05F0_LOAD);CALL(0x69A,0x626,0,PR_U7D_05F0_PRINTER);
        v=rb(&m,c->ss,local(&m,-0x10));arithmetic(c,v,3,8,0,0);wb(&m,c->ss,local(&m,-0x10),(uint8_t)(v+3u));
        v=rb(&m,c->ss,local(&m,-1));arithmetic(c,v,3,8,1,0);if(c->flags&0x40u)break;
        c->ip=0x6A7;if(loops==l->caller_loopbacks)return PR_U7D_05F0_LIMIT;++loops;
        v=rb(&m,c->ss,local(&m,-1));arithmetic(c,v,1,8,1,1);wb(&m,c->ss,local(&m,-1),(uint8_t)(v-1u));
    }
    al(c,rb(&m,c->ss,local(&m,-0x10)));zero_ah(c);inc_ax(c);push(&m,c->ax);al(c,0x11);push(&m,c->ax);
    al(c,rb(&m,c->ss,local(&m,-11)));push(&m,c->ax);push_local(&m,-0x110);c->ax=10;
    { uint32_t product=(uint32_t)c->ax*rd(&m,c->ds,0x6DC6);c->ax=(uint16_t)product;c->dx=(uint16_t)(product>>16); }
    /* ADD immediately overwrites MUL's arithmetic flags before any stop. */
    v=rd(&m,c->ds,0x6DC4);arithmetic(c,c->ax,v,16,0,0);c->ax=(uint16_t)(c->ax+v);push(&m,c->ax);push(&m,c->cs);
    CALL(0x6CA,0x565,1,PR_U7D_05F0_FORMAT);CALL(0x6CD,0x626,0,PR_U7D_05F0_PRINTER);
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=at(b->ram,p);c->cs=at(b->ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+6u);
    return PR_U7D_05F0_OK;
}
