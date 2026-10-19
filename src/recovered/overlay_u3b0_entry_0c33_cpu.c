/* Complete U3B0 +0C33; see U3B0_ENTRY_0C33_CPU.md. */
#include "poolrad/recovered/overlay_u3b0_entry_0c33_cpu.h"
#include "poolrad/recovered/overlay_u3b0_step_scan_0b03_cpu.h"
#define MASK ((1u<<20)-1u)
typedef struct { pr_bus *b; pr_u3b0_0c33_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
/* DS/SS and all flat-RAM callee operands are outside the EGA aperture. */
static uint16_t at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return at(m->b->ram,physical(s,o)); }
static uint8_t rb(machine *m,uint16_t s,uint16_t o) { return m->b->ram[physical(s,o)]; }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) { size_t p=physical(s,o);m->b->ram[p]=(uint8_t)v;m->b->ram[(p+1)&MASK]=(uint8_t)(v>>8); }
static uint16_t local(machine *m,int d) { return (uint16_t)(m->c->bp+d); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void arithmetic(pr_u3b0_0c33_cpu *c,uint16_t a,uint16_t b,unsigned bits,int subtract,int increment) {
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

static uint16_t link(machine *m) { m->c->di=rd(m,m->c->ss,local(m,6));return m->c->di; }
static void index_pointer(machine *m) {
    size_t p=physical(m->c->ss,(uint16_t)(link(m)+10u));
    m->c->di=at(m->b->ram,p);m->c->es=at(m->b->ram,(p+2)&MASK);
}
static int ge(const pr_u3b0_0c33_cpu *c) { return !!(c->flags&0x80u)==!!(c->flags&0x800u); }
static pr_u3b0_0c33_status call(machine *m,uint16_t site,uint16_t target,int which,
    const pr_u3b0_0c33_limits *l,pr_u3b0_0c33_diagnostic *d)
{
    pr_u3b0_0c33_cpu *c=m->c;uint16_t caller=c->cs,next=(uint16_t)(site+3u);
    c->ip=site;
    if(rb(m,caller,site)!=0xE8 || rd(m,caller,(uint16_t)(site+1u))!=(uint16_t)(target-next))return PR_U3B0_0C33_CODE;
    push(m,next);c->ip=target;d->callee=which;
    if(which==PR_U3B0_0C33_STEP) {
        pr_u3b0_0b03_cpu inner;COPY(inner,*c);
        d->status=(int)pr_u3b0_step_scan_0b03_cpu(m->b->ram,1u<<20,&inner,l->step_loopbacks,&d->step_status);COPY(*c,inner);
    } else {
        pr_u3b0_07a4_cpu inner;COPY(inner,*c);
        d->status=(int)pr_u3b0_entry_07a4_cpu(m->b,1u<<20,&inner,&l->display,&d->display);COPY(*c,inner);
    }
    if(d->status)return PR_U3B0_0C33_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U3B0_0C33_TRANSFER;
    return PR_U3B0_0C33_OK;
}
static int ordinary(uint16_t s) { return s<=0x9000 || s>=0xB000; }
pr_u3b0_0c33_status pr_u3b0_entry_0c33_cpu(pr_bus *b,size_t size,pr_u3b0_0c33_cpu *c,
    const pr_u3b0_0c33_limits *l,pr_u3b0_0c33_diagnostic *d)
{
    pr_u3b0_0c33_diagnostic fallback={-1,-1,-1,{-1,-1,{-1,-1,{-1,-1}},{-1,-1,-1,-1},{-1,-1}}};
    machine m={b,c};pr_u3b0_0c33_status status;uint16_t v;size_t p;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(b==NULL || b->ram==NULL || b->ega==NULL || c==NULL || l==NULL || size!=(1u<<20) ||
       c->ip!=0xC33 || c->sp<0x320 || c->sp>0xFFF4 || !ordinary(c->ds) || !ordinary(c->ss))return PR_U3B0_0C33_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;arithmetic(c,c->sp,2,16,1,0);c->sp=(uint16_t)(c->sp-2u);
    index_pointer(&m);c->ax=rd(&m,c->es,c->di);v=rd(&m,c->ds,0x6B29);
    arithmetic(c,c->ax,v,16,1,0);c->ax=(uint16_t)(c->ax-v);wr(&m,c->ss,local(&m,-2),c->ax);
    v=rb(&m,c->ss,local(&m,8));arithmetic(c,v,0,8,1,0);
    if(!(c->flags&0x40u)) {
        c->ax=rd(&m,c->ss,(uint16_t)(link(&m)-0x59u));v=rd(&m,c->ds,0x6B29);
        arithmetic(c,v,c->ax,16,0,0);wr(&m,c->ds,0x6B29,(uint16_t)(v+c->ax));
        c->ax=rd(&m,c->ss,(uint16_t)(link(&m)-0x56u));v=rd(&m,c->ss,(uint16_t)(link(&m)-0x59u));
        arithmetic(c,c->ax,v,16,1,0);c->ax=(uint16_t)(c->ax-v);arithmetic(c,c->ax,rd(&m,c->ds,0x6B29),16,1,0);
        if(!ge(c)) {
            c->ax=rd(&m,c->ss,(uint16_t)(link(&m)-0x56u));v=rd(&m,c->ss,(uint16_t)(link(&m)-0x59u));
            arithmetic(c,c->ax,v,16,1,0);c->ax=(uint16_t)(c->ax-v);wr(&m,c->ds,0x6B29,c->ax);
        }
    } else {
        c->ax=rd(&m,c->ss,(uint16_t)(link(&m)-0x59u));v=rd(&m,c->ds,0x6B29);
        arithmetic(c,v,c->ax,16,1,0);wr(&m,c->ds,0x6B29,(uint16_t)(v-c->ax));arithmetic(c,rd(&m,c->ds,0x6B29),0,16,1,0);
        if(!ge(c)) { c->ax=0;c->flags=(uint16_t)((c->flags&~0x8D5u)|0x44u);wr(&m,c->ds,0x6B29,0); }
    }
    c->ax=rd(&m,c->ds,0x6B29);v=rd(&m,c->ss,local(&m,-2));arithmetic(c,c->ax,v,16,0,0);c->ax=(uint16_t)(c->ax+v);
    index_pointer(&m);wr(&m,c->es,c->di,c->ax);
    c->ax=(uint16_t)((c->ax&0xFF00u)|rb(&m,c->ss,local(&m,8)));push(&m,c->ax);push(&m,rd(&m,c->ss,local(&m,6)));push(&m,c->cs);
    status=call(&m,0xCB1,0xB03,PR_U3B0_0C33_STEP,l,d);if(status)return status;
    push(&m,rd(&m,c->ds,0x6B29));push(&m,rd(&m,c->ss,local(&m,6)));push(&m,c->cs);
    status=call(&m,0xCBC,0x7A4,PR_U3B0_0C33_DISPLAY,l,d);if(status)return status;
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=at(b->ram,p);c->cs=at(b->ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+8u);
    return PR_U3B0_0C33_OK;
}
