/* Complete U1C +2E7D; see U1C_TEXT_2E7D_CPU.md. Composition idioms follow
 * overlay_u1c_compare_and_0aed_cpu.c and resident_0709_space_05eb_cpu.c. */
#include "poolrad/recovered/overlay_u1c_text_2e7d_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"
#include "poolrad/recovered/overlay_u25_load_word_0e0c.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/resident_0709_glyph_01df_cpu.h"
#define MASK ((1u<<20)-1u)
typedef struct { pr_bus *b; pr_u1c_2e7d_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
/* DS/SS and all flat-RAM callee operands are outside the EGA aperture. */
static uint16_t at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return at(m->b->ram,physical(s,o)); }
static uint8_t rb(machine *m,uint16_t s,uint16_t o) { return m->b->ram[physical(s,o)]; }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) { size_t p=physical(s,o);m->b->ram[p]=(uint8_t)v;m->b->ram[(p+1)&MASK]=(uint8_t)(v>>8); }
static void wb(machine *m,uint16_t s,uint16_t o,uint8_t v) { m->b->ram[physical(s,o)]=v; }
static uint16_t local(machine *m,int d) { return (uint16_t)(m->c->bp+d); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void al(pr_u1c_2e7d_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void arithmetic(pr_u1c_2e7d_cpu *c,uint16_t a,uint16_t b,unsigned bits,int subtract,int increment) {
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
static void inc_byte(machine *m,uint16_t s,uint16_t o) { uint8_t v=rb(m,s,o);arithmetic(m->c,v,1,8,0,1);wb(m,s,o,(uint8_t)(v+1u)); }
#define COPY(to,from) do {     (to).ax=(from).ax; (to).bx=(from).bx; (to).cx=(from).cx;     (to).dx=(from).dx; (to).si=(from).si; (to).di=(from).di;     (to).bp=(from).bp; (to).sp=(from).sp; (to).cs=(from).cs;     (to).ip=(from).ip; (to).ds=(from).ds; (to).es=(from).es;     (to).ss=(from).ss; (to).flags=(from).flags; } while(0)
static pr_u1c_2e7d_status call(machine *m,uint16_t site,uint16_t off,uint16_t stub_ip,
    pr_u1c_2e7d_callee which,const pr_u1c_2e7d_limits *l,pr_u1c_2e7d_diagnostic *d)
{
    pr_u1c_2e7d_cpu *c=m->c;uint16_t caller=c->cs,seg,ip=off,next=(uint16_t)(site+5u);
    c->ip=site;
    if(rb(m,caller,site)!=0x9A || rd(m,caller,(uint16_t)(site+1u))!=off)return PR_U1C_2E7D_CODE;
    seg=rd(m,caller,(uint16_t)(site+3u));
    if(stub_ip!=0xFFFF) {
        if(rb(m,seg,off)!=0xEA || rd(m,seg,(uint16_t)(off+1u))!=stub_ip)return PR_U1C_2E7D_CODE;
        ip=stub_ip;seg=rd(m,seg,(uint16_t)(off+3u));
    }
    push(m,caller);push(m,next);c->cs=seg;c->ip=ip;d->callee=(int)which;
    if(which==PR_U1C_2E7D_FILLCHAR) {
        pr_system_leaf_cpu inner;COPY(inner,*c);
        d->status=(int)pr_system_fillchar_16a8(m->b->ram,1u<<20,&inner);COPY(*c,inner);
    } else if(which==PR_U1C_2E7D_LOADER || which==PR_U1C_2E7D_WORD) {
        pr_u25_0000_cpu inner;COPY(inner,*c);
        d->status=(int)(which==PR_U1C_2E7D_LOADER?
            pr_u25_load_operands_0000(m->b->ram,1u<<20,&inner,l->system_cs,l->max_operands,l->max_string_iterations):
            pr_u25_word_06af(m->b->ram,1u<<20,&inner));COPY(*c,inner);
    } else if(which==PR_U1C_2E7D_LOAD_WORD) {
        pr_u25_load_word_cpu inner;COPY(inner,*c);
        d->status=(int)pr_u25_load_word_0e0c(m->b->ram,1u<<20,&inner);COPY(*c,inner);
    } else {
        pr_r0709_glyph_cpu inner;pr_r0709_glyph_cpu_diagnostic gd;COPY(inner,*c);
        d->status=(int)pr_r0709_glyph_01df_cpu(m->b,1u<<20,&inner,l->max_glyph_loopbacks,&gd);
        COPY(*c,inner);d->glyph_callee=gd.callee;d->glyph_status=gd.status;
    }
    if(d->status!=0)return PR_U1C_2E7D_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U1C_2E7D_TRANSFER;
    return PR_U1C_2E7D_OK;
}
static int plain_segment(uint16_t s) { return s<=0x9000 || s>=0xB000; }
pr_u1c_2e7d_status pr_u1c_text_2e7d_cpu(pr_bus *b,size_t size,pr_u1c_2e7d_cpu *c,
    const pr_u1c_2e7d_limits *l,pr_u1c_2e7d_diagnostic *d)
{
    pr_u1c_2e7d_diagnostic fallback={-1,-1,-1,-1};machine m={b,c};
    pr_u1c_2e7d_status status;size_t loops=0,p;uint16_t v;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(b==NULL || b->ram==NULL || b->ega==NULL || c==NULL || l==NULL || size!=(1u<<20) ||
       c->ip!=0x2E7D || c->sp<0x23A || c->sp>0xFFFC || !plain_segment(c->ds) || !plain_segment(c->ss) ||
       l->max_operands==0 || l->max_string_iterations==0)return PR_U1C_2E7D_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;arithmetic(c,c->sp,8,16,1,0);c->sp=(uint16_t)(c->sp-8u);
    c->di=0x84C8;push(&m,c->ds);push(&m,c->di);c->ax=2;push(&m,c->ax);al(c,0);push(&m,c->ax);
    status=call(&m,0x2E8F,0x16A8,0xFFFF,PR_U1C_2E7D_FILLCHAR,l,d);if(status)return status;
    wb(&m,c->ds,0x84E0,0);al(c,1);push(&m,c->ax);
    status=call(&m,0x2E9C,0x2A,0,PR_U1C_2E7D_LOADER,l,d);if(status)return status;
    al(c,rb(&m,c->ds,0x7086));push(&m,c->ax);al(c,rb(&m,c->ds,0x7046));push(&m,c->ax);
    status=call(&m,0x2EA9,0x4D,0x6AF,PR_U1C_2E7D_WORD,l,d);if(status)return status;
    wr(&m,c->ss,local(&m,-2),c->ax);wb(&m,c->ss,local(&m,-5),0);wb(&m,c->ss,local(&m,-7),0);
    arithmetic(c,rb(&m,c->ds,0x84DE),0,8,1,0);
    if(!(c->flags&0x40)) {
        arithmetic(c,rb(&m,c->ds,0x5E25),13,8,1,0);
        if(!(c->flags&0x41)) { wb(&m,c->ds,0x5E25,5);wb(&m,c->ds,0x5E24,2);wb(&m,c->ds,0x84E2,1); }
    }
    wb(&m,c->ds,0x84DF,1);
    for(;;) {
        al(c,rb(&m,c->ss,local(&m,-5)));c->ax&=255u; /* XOR AH flags are overwritten by ADD. */
        v=rd(&m,c->ss,local(&m,-2));arithmetic(c,c->ax,v,16,0,0);c->ax=(uint16_t)(c->ax+v);
        wr(&m,c->ss,local(&m,-4),c->ax);push(&m,rd(&m,c->ss,local(&m,-4)));
        status=call(&m,0x2EE9,0x57,0xE0C,PR_U1C_2E7D_LOAD_WORD,l,d);if(status)return status;
        arithmetic(c,c->ax,1,16,0,1);c->ax=(uint16_t)(c->ax+1u);wb(&m,c->ss,local(&m,-6),(uint8_t)c->ax);
        arithmetic(c,rb(&m,c->ss,local(&m,-6)),1,8,1,0);
        if(!(c->flags&0x41)) {
            al(c,rb(&m,c->ds,0x5E24));push(&m,c->ax);al(c,rb(&m,c->ds,0x5E25));push(&m,c->ax);
            al(c,15);push(&m,c->ax);al(c,1);push(&m,c->ax);al(c,rb(&m,c->ss,local(&m,-6)));push(&m,c->ax);al(c,0);push(&m,c->ax);
            status=call(&m,0x2F0D,0x1DF,0xFFFF,PR_U1C_2E7D_GLYPH,l,d);if(status)return status;
            inc_byte(&m,c->ds,0x5E24);inc_byte(&m,c->ss,local(&m,-5));
        } else wb(&m,c->ss,local(&m,-7),1);
        arithmetic(c,rb(&m,c->ss,local(&m,-7)),0,8,1,0);
        if(!(c->flags&0x40))break;
        c->ip=0x2F23;if(loops==l->max_loopbacks)return PR_U1C_2E7D_LIMIT;++loops;
    }
    inc_byte(&m,c->ds,0x5E25);c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=at(b->ram,p);c->cs=at(b->ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+4u);
    return PR_U1C_2E7D_OK;
}
