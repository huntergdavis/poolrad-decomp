/* Complete U3B0 +0969; see U3B0_ENTRY_0969_CPU.md. */
#include "poolrad/recovered/overlay_u3b0_entry_0969_cpu.h"
#include "poolrad/recovered/overlay_u3b0_entry1_0000.h"
#include "poolrad/recovered/overlay_u3b0_space_scan_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#define MASK ((1u<<20)-1u)
typedef struct { pr_bus *b; pr_u3b0_0969_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
/* DS/SS and all flat-RAM callee operands are outside the EGA aperture. */
static uint16_t at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return at(m->b->ram,physical(s,o)); }
static uint8_t rb(machine *m,uint16_t s,uint16_t o) { return m->b->ram[physical(s,o)]; }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) { size_t p=physical(s,o);m->b->ram[p]=(uint8_t)v;m->b->ram[(p+1)&MASK]=(uint8_t)(v>>8); }
static void wb(machine *m,uint16_t s,uint16_t o,uint8_t v) { m->b->ram[physical(s,o)]=v; }
static uint16_t local(machine *m,int d) { return (uint16_t)(m->c->bp+d); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void al(pr_u3b0_0969_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void arithmetic(pr_u3b0_0969_cpu *c,uint16_t a,uint16_t b,unsigned bits,int subtract,int increment) {
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
static void les_local(machine *m) {
    size_t p=physical(m->c->ss,local(m,-4));
    m->c->di=at(m->b->ram,p);m->c->es=at(m->b->ram,(p+2)&MASK);
}
static void zero_ah(pr_u3b0_0969_cpu *c) { c->ax&=255u;c->flags=(uint16_t)((c->flags&~0x8D5u)|0x44u); }
static void cbw(pr_u3b0_0969_cpu *c) { c->ax=(uint16_t)((c->ax&0x80u)?c->ax|0xFF00u:c->ax&255u); }
static pr_u3b0_0969_status call(machine *m,uint16_t site,uint16_t target,int near_call,
    pr_u3b0_0969_callee which,const pr_u3b0_0969_limits *l,pr_u3b0_0969_diagnostic *d)
{
    pr_u3b0_0969_cpu *c=m->c;uint16_t caller=c->cs,next=(uint16_t)(site+(near_call?3u:5u)),seg=caller;
    c->ip=site;
    if(rb(m,caller,site)!=(near_call?0xE8:0x9A) ||
       rd(m,caller,(uint16_t)(site+1u))!=(near_call?(uint16_t)(target-next):target))return PR_U3B0_0969_CODE;
    if(!near_call) { seg=rd(m,caller,(uint16_t)(site+3u));push(m,caller); }
    push(m,next);c->cs=seg;c->ip=target;d->callee=(int)which;
    if(which==PR_U3B0_0969_SELECT) {
        pr_u3b0_0000_cpu inner;COPY(inner,*c);
        d->status=(int)pr_u3b0_entry1_0000(m->b->ram,1u<<20,&inner);COPY(*c,inner);
    } else if(which==PR_U3B0_0969_LEAD || which==PR_U3B0_0969_TRAIL) {
        pr_u3b0s_cpu inner;COPY(inner,*c);
        d->status=(int)(which==PR_U3B0_0969_LEAD?
            pr_u3b0_lead_spaces_08d7_cpu(m->b->ram,1u<<20,&inner,&d->scan_store_status):
            pr_u3b0_trail_spaces_0923_cpu(m->b->ram,1u<<20,&inner,&d->scan_store_status));COPY(*c,inner);
    } else if(which==PR_U3B0_0969_COPY) {
        pr_system_leaf_cpu inner;COPY(inner,*c);
        d->status=(int)pr_system_strcopy_0b57(m->b->ram,1u<<20,&inner);COPY(*c,inner);
    } else {
        pr_r0709_printer_cpu inner;COPY(inner,*c);
        d->status=(int)pr_r0709_printer_0626_cpu(m->b,1u<<20,&inner,l->printer_loopbacks,l->glyph_loopbacks,&d->printer);COPY(*c,inner);
    }
    if(d->status)return PR_U3B0_0969_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U3B0_0969_TRANSFER;
    return PR_U3B0_0969_OK;
}
#define CALL(site,target,near,which) do { status=call(&m,site,target,near,which,l,d);if(status)return status; } while(0)
static int ordinary(uint16_t s) { return s<=0x9000 || s>=0xB000; }
pr_u3b0_0969_status pr_u3b0_entry_0969_cpu(pr_bus *b,size_t size,pr_u3b0_0969_cpu *c,
    const pr_u3b0_0969_limits *l,pr_u3b0_0969_diagnostic *d)
{
    pr_u3b0_0969_diagnostic fallback={-1,-1,-1,{-1,-1,-1,-1}};machine m={b,c};
    pr_u3b0_0969_status status;size_t p;uint16_t v;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(b==NULL || b->ram==NULL || b->ega==NULL || c==NULL || l==NULL || size!=(1u<<20) ||
       c->ip!=0x969 || c->sp<0x300 || c->sp>0xFFF4 || !ordinary(c->ds) || !ordinary(c->ss))return PR_U3B0_0969_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;arithmetic(c,c->sp,0x106,16,1,0);c->sp=(uint16_t)(c->sp-0x106u);
    push(&m,rd(&m,c->ss,local(&m,8)));c->di=rd(&m,c->ss,local(&m,6));
    push(&m,rd(&m,c->ss,(uint16_t)(c->di+0x16u)));push(&m,rd(&m,c->ss,(uint16_t)(c->di+0x14u)));push(&m,c->cs);
    CALL(0x97F,0,1,PR_U3B0_0969_SELECT);
    wr(&m,c->ss,local(&m,-4),c->ax);wr(&m,c->ss,local(&m,-2),c->dx);les_local(&m);push(&m,c->es);push(&m,c->di);
    push(&m,rd(&m,c->ss,local(&m,6)));push(&m,c->cs);CALL(0x991,0x8D7,1,PR_U3B0_0969_LEAD);
    wb(&m,c->ss,local(&m,-5),(uint8_t)c->ax);al(c,rb(&m,c->ss,local(&m,-5)));zero_ah(c);push(&m,c->ax);
    les_local(&m);push(&m,c->es);push(&m,c->di);push(&m,rd(&m,c->ss,local(&m,6)));push(&m,c->cs);
    CALL(0x9A6,0x923,1,PR_U3B0_0969_TRAIL);
    zero_ah(c);c->dx=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    arithmetic(c,c->ax,c->dx,16,1,0);c->ax=(uint16_t)(c->ax-c->dx);
    arithmetic(c,c->ax,1,16,0,1);c->ax=(uint16_t)(c->ax+1u);wb(&m,c->ss,local(&m,-6),(uint8_t)c->ax);
    al(c,rb(&m,c->ss,local(&m,-5)));zero_ah(c);c->dx=c->ax;c->di=rd(&m,c->ss,local(&m,6));
    al(c,rb(&m,c->ss,(uint16_t)(c->di+0x1Eu)));cbw(c);arithmetic(c,c->ax,c->dx,16,0,0);c->ax=(uint16_t)(c->ax+c->dx);
    arithmetic(c,c->ax,1,16,1,1);c->ax=(uint16_t)(c->ax-1u);push(&m,c->ax);
    c->ax=rd(&m,c->ss,local(&m,8));v=rd(&m,c->ds,0x6B29);arithmetic(c,c->ax,v,16,1,0);c->ax=(uint16_t)(c->ax-v);c->dx=c->ax;
    c->di=rd(&m,c->ss,local(&m,6));al(c,rb(&m,c->ss,(uint16_t)(c->di+0x1Cu)));cbw(c);
    arithmetic(c,c->ax,c->dx,16,0,0);c->ax=(uint16_t)(c->ax+c->dx);push(&m,c->ax);
    c->di=rd(&m,c->ss,local(&m,6));al(c,rb(&m,c->ss,(uint16_t)(c->di+0x20u)));push(&m,c->ax);
    c->di=local(&m,-0x106);push(&m,c->ss);push(&m,c->di);les_local(&m);push(&m,c->es);push(&m,c->di);
    al(c,rb(&m,c->ss,local(&m,-5)));zero_ah(c);push(&m,c->ax);al(c,rb(&m,c->ss,local(&m,-6)));zero_ah(c);push(&m,c->ax);
    CALL(0x9F8,0xB57,0,PR_U3B0_0969_COPY);CALL(0x9FD,0x626,0,PR_U3B0_0969_PRINTER);
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=at(b->ram,p);c->cs=at(b->ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+8u);
    return PR_U3B0_0969_OK;
}
