/* Complete raw U1C CLEARMONSTERS; see U1C_CLEARMONSTERS_125D_CPU.md. */
#include "poolrad/recovered/overlay_u1c_clearmonsters_125d_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#define MASK ((1u<<20)-1u)
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&MASK]<<8)); }
static uint16_t rd(const uint8_t *r,uint16_t s,uint16_t o) { return at(r,physical(s,o)); }
static void wr(uint8_t *r,uint16_t s,uint16_t o,uint16_t v) { size_t p=physical(s,o);r[p]=(uint8_t)v;r[(p+1u)&MASK]=(uint8_t)(v>>8); }
static void push(uint8_t *r,pr_u1c_cmon_cpu *c,uint16_t v) { c->sp=(uint16_t)(c->sp-2u);wr(r,c->ss,c->sp,v); }
static uint16_t local(const pr_u1c_cmon_cpu *c,int d) { return (uint16_t)(c->bp+d); }
static unsigned parity(uint16_t v) { uint8_t p=(uint8_t)v;p^=p>>4;p^=p>>2;p^=p>>1;return !(p&1u); }
static void logic(pr_u1c_cmon_cpu *c,uint16_t v) {
    c->flags&=(uint16_t)~0x8D5u;
    if(parity(v))c->flags|=4;
    if(!v)c->flags|=0x40;
    if(v&0x8000u)c->flags|=0x80;
}
#define COPY(to,from) do { (to).ax=(from).ax;(to).bx=(from).bx;(to).cx=(from).cx;(to).dx=(from).dx;(to).si=(from).si;(to).di=(from).di;(to).bp=(from).bp;(to).sp=(from).sp;(to).cs=(from).cs;(to).ip=(from).ip;(to).ds=(from).ds;(to).es=(from).es;(to).ss=(from).ss;(to).flags=(from).flags; } while(0)
static pr_u1c_cmon_cpu_status call(uint8_t *r,pr_u1c_cmon_cpu *c,uint16_t site,uint16_t off,
    unsigned max_heap_steps,pr_u1c_cmon_cpu_diagnostic *d)
{
    uint16_t caller=c->cs,system,next=(uint16_t)(site+5u);c->ip=site;
    if(r[physical(caller,site)]!=0x9A || rd(r,caller,(uint16_t)(site+1u))!=off)return PR_U1C_CMON_CPU_CODE;
    system=rd(r,caller,(uint16_t)(site+3u));push(r,c,caller);push(r,c,next);c->cs=system;c->ip=off;
    if(off==0x16A8) {
        pr_system_leaf_cpu inner;COPY(inner,*c);d->callee=0;
        d->status=(int)pr_system_fillchar_16a8(r,1u<<20,&inner);COPY(*c,inner);
    } else { d->callee=1;d->status=(int)pr_system_freemem_0254(r,1u<<20,c,max_heap_steps); }
    if(d->status)return PR_U1C_CMON_CPU_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U1C_CMON_CPU_TRANSFER;
    return PR_U1C_CMON_CPU_RETURNED;
}
pr_u1c_cmon_cpu_status pr_u1c_clearmonsters_125d_cpu(uint8_t *r,size_t size,
    pr_u1c_cmon_cpu *c,unsigned max_loopbacks,unsigned max_heap_steps,pr_u1c_cmon_cpu_diagnostic *d)
{
    pr_u1c_cmon_cpu_diagnostic fallback={-1,-1};pr_u1c_cmon_cpu_status status;
    uint16_t v,result;unsigned loops=0;size_t p;
    if(!d)d=&fallback;
    *d=fallback;
    if(!r || !c || size!=(1u<<20) || c->ip!=0x125D || c->sp<0x32 || c->sp>0xFFFC)return PR_U1C_CMON_CPU_ARGUMENT;
    push(r,c,c->bp);c->bp=c->sp;
    /* SUB supplies CF, retained by INC; INC overwrites the other arithmetic flags. */
    v=c->sp;c->sp=(uint16_t)(v-6u);c->flags=(uint16_t)((c->flags&~1u)|(v<6u));
    v=rd(r,c->ds,0x49ED);result=(uint16_t)(v+1u);wr(r,c->ds,0x49ED,result);
    c->flags&=(uint16_t)~0x8D4u;
    if(parity(result))c->flags|=4;
    if((v&15u)==15u)c->flags|=0x10;
    if(!result)c->flags|=0x40;
    if(result&0x8000u)c->flags|=0x80;
    if(v==0x7FFF)c->flags|=0x800;
    r[physical(c->ds,0x4435)]=0;r[physical(c->ds,0x84E7)]=0;r[physical(c->ds,0x6F83)]=8;
    c->di=0x67F4;push(r,c,c->ds);push(r,c,c->di);c->ax=0x1C;push(r,c,c->ax);c->ax&=0xFF00u;push(r,c,c->ax);
    status=call(r,c,0x1282,0x16A8,max_heap_steps,d);if(status)return status;
    for(;;) {
        c->ax=rd(r,c->ds,0x6810);c->ax|=rd(r,c->ds,0x6812);logic(c,c->ax);
        if(!c->ax)break;
        p=physical(c->ds,0x6810);c->di=at(r,p);c->es=at(r,(p+2u)&MASK);
        c->ax=rd(r,c->es,(uint16_t)(c->di+0x2Au));c->dx=rd(r,c->es,(uint16_t)(c->di+0x2Cu));
        wr(r,c->ss,local(c,-4),c->ax);wr(r,c->ss,local(c,-2),c->dx);
        push(r,c,rd(r,c->ds,0x6812));push(r,c,rd(r,c->ds,0x6810));c->ax=0x3F;push(r,c,c->ax);
        status=call(r,c,0x12AE,0x254,max_heap_steps,d);if(status)return status;
        c->ax=rd(r,c->ss,local(c,-4));c->dx=rd(r,c->ss,local(c,-2));
        wr(r,c->ds,0x6810,c->ax);wr(r,c->ds,0x6812,c->dx);
        c->ip=0x1287;if(loops==max_loopbacks)return PR_U1C_CMON_CPU_LIMIT;++loops;
    }
    c->sp=c->bp;c->bp=rd(r,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=at(r,p);c->cs=at(r,(p+2u)&MASK);c->sp=(uint16_t)(c->sp+4u);
    return PR_U1C_CMON_CPU_RETURNED;
}
