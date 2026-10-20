/* U25 entry 20 [1CC3,1DF0) composed with the raw SYSTEM Real models; docs/re/U25_ENTRY20_1CC3_CPU.md.
 * The translation of the body follows the merged overlay_u25_entry20_1cc3.c, with its three call-site
 * callbacks replaced by pr_system_real_*. */
#include "poolrad/recovered/overlay_u25_entry20_1cc3_cpu.h"
#define MASK (PR_U25_1CC3_CPU_RAM_SIZE-1u)
#define COPY_CPU(to,from) do { \
    (to).ax=(from).ax; (to).bx=(from).bx; (to).cx=(from).cx; \
    (to).dx=(from).dx; (to).si=(from).si; (to).di=(from).di; \
    (to).bp=(from).bp; (to).sp=(from).sp; (to).cs=(from).cs; \
    (to).ip=(from).ip; (to).ds=(from).ds; (to).es=(from).es; \
    (to).ss=(from).ss; (to).flags=(from).flags; \
} while(0)
_Static_assert(sizeof(pr_u25_1cc3_cpu_state)==sizeof(pr_system_real_cpu),"SYSTEM Real CPU layout differs");
typedef struct { uint8_t *r; pr_u25_1cc3_cpu_state *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return word_at(m->r,physical(s,o)); }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=physical(s,o);m->r[p]=(uint8_t)v;m->r[(p+1)&MASK]=(uint8_t)(v>>8);
}
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void les_record(machine *m) {
    size_t p=physical(m->c->ss,(uint16_t)(m->c->bp+6u));
    m->c->di=word_at(m->r,p);m->c->es=word_at(m->r,(p+2)&MASK);
}
/* LCALL <segment>:<entry> at `site`: operands checked before the pushes, the callee entered through the real frame. */
static pr_u25_1cc3_cpu_status call_system(machine *m,uint16_t site,uint16_t entry,pr_u25_1cc3_cpu_diagnostic *d) {
    pr_u25_1cc3_cpu_state *c=m->c;uint16_t caller=c->cs,continuation=(uint16_t)(site+5u),segment;
    pr_system_real_cpu inner;pr_system_real_status status;
    c->ip=site;d->call_site=site;
    if(m->r[physical(caller,site)]!=0x9A || rd(m,caller,(uint16_t)(site+1u))!=entry)return PR_U25_1CC3_CPU_CODE;
    segment=rd(m,caller,(uint16_t)(site+3u));
    push(m,caller);push(m,continuation);c->cs=segment;c->ip=entry;COPY_CPU(inner,*c);
    switch(entry) {
    case 0x119Au: status=pr_system_real_from_long_119a(m->r,PR_U25_1CC3_CPU_RAM_SIZE,&inner);break;
    case 0x1186u: status=pr_system_real_mul_1186(m->r,PR_U25_1CC3_CPU_RAM_SIZE,&inner);break;
    default:      status=pr_system_real_trunc_119e(m->r,PR_U25_1CC3_CPU_RAM_SIZE,&inner);break;
    }
    COPY_CPU(*c,inner);++d->calls;d->callee_status=(int)status;
    if(status!=PR_SYSTEM_REAL_RETURNED)return status==PR_SYSTEM_REAL_TRANSFER?PR_U25_1CC3_CPU_TRANSFER:PR_U25_1CC3_CPU_CALLEE;
    if(c->cs!=caller || c->ip!=continuation)return PR_U25_1CC3_CPU_TRANSFER;
    return PR_U25_1CC3_CPU_OK;
}
pr_u25_1cc3_cpu_status pr_u25_entry20_1cc3_cpu(uint8_t *ram,size_t size,pr_u25_1cc3_cpu_state *c,
    pr_u25_1cc3_cpu_diagnostic *d)
{
    machine m={ram,c};pr_u25_1cc3_cpu_diagnostic fallback={-1,0,0};
    pr_u25_1cc3_cpu_status status;unsigned k;size_t p;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || c==NULL || size!=PR_U25_1CC3_CPU_RAM_SIZE || c->ip!=0x1CC3u || c->sp<0x46u || c->sp>0xFFF2u)
        return PR_U25_1CC3_CPU_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;                                       /* 1CC3 PUSH BP; MOV BP,SP */
    for(k=0;k<7u;++k) {
        uint16_t field=(uint16_t)(0x88u+2u*k),site=(uint16_t)(0x1CD0u+0x2Au*k);
        les_record(&m);c->ax=rd(&m,c->es,(uint16_t)(c->di+field));     /* LES DI,[BP+6]; MOV AX,ES:[DI+field] */
        c->dx=0;c->flags=(uint16_t)((c->flags&~0x08D5u)|0x0044u);       /* XOR DX,DX */
        status=call_system(&m,site,0x119Au,d);if(status!=PR_U25_1CC3_CPU_OK)return status;
        c->cx=rd(&m,c->ss,(uint16_t)(c->bp+0x0Au));                    /* MOV CX,[BP+0Ah]; MOV SI,[BP+0Ch]; MOV DI,[BP+0Eh] */
        c->si=rd(&m,c->ss,(uint16_t)(c->bp+0x0Cu));
        c->di=rd(&m,c->ss,(uint16_t)(c->bp+0x0Eu));
        status=call_system(&m,(uint16_t)(site+0x0Eu),0x1186u,d);if(status!=PR_U25_1CC3_CPU_OK)return status;
        status=call_system(&m,(uint16_t)(site+0x13u),0x119Eu,d);if(status!=PR_U25_1CC3_CPU_OK)return status;
        les_record(&m);wr(&m,c->es,(uint16_t)(c->di+field),c->ax);      /* LES DI,[BP+6]; MOV ES:[DI+field],AX */
    }
    c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);               /* 1DEC POP BP */
    p=physical(c->ss,c->sp);c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2)&MASK);
    c->sp=(uint16_t)(c->sp+4u+0x0Au);return PR_U25_1CC3_CPU_OK;       /* 1DED RETF 0Ah */
}
