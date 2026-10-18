/* Complete U1C +0111 GOSUB; docs/re/U1C_GOSUB_0111_CPU.md.
 * Live-stub/memory composition follows accepted U1C +00F4/+18F8. */
#include "poolrad/recovered/overlay_u1c_gosub_0111_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"
#include "poolrad/recovered/overlay_u25_alloc_1951_cpu.h"
#include <string.h>
#define MASK (PR_U1C_GOSUB_RAM_SIZE-1u)
_Static_assert(sizeof(pr_u1c_gosub_cpu)==sizeof(pr_u25_0000_cpu),"loader CPU layout");
_Static_assert(sizeof(pr_u1c_gosub_cpu)==sizeof(pr_u25_1951_cpu),"allocator CPU layout");
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *ram,size_t p) { return (uint16_t)(ram[p]|((uint16_t)ram[(p+1u)&MASK]<<8)); }
static uint16_t read16(const uint8_t *ram,uint16_t s,uint16_t o) { return word_at(ram,physical(s,o)); }
static void push(uint8_t *ram,pr_u1c_gosub_cpu *c,uint16_t v) {
    size_t p;c->sp=(uint16_t)(c->sp-2u);p=physical(c->ss,c->sp);
    ram[p]=(uint8_t)v;ram[(p+1u)&MASK]=(uint8_t)(v>>8);
}
static void al(pr_u1c_gosub_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
static pr_u1c_gosub_status call(uint8_t *ram,size_t size,pr_u1c_gosub_cpu *c,
    const pr_u1c_gosub_limits *limits,pr_u1c_gosub_diagnostic *d,
    uint16_t site,uint16_t offset,uint16_t target,pr_u1c_gosub_callee which)
{
    uint16_t caller=c->cs,next=(uint16_t)(site+5u),stub;int status;
    c->ip=site;
    if(ram[physical(caller,site)]!=0x9Au || read16(ram,caller,(uint16_t)(site+1u))!=offset)
        return PR_U1C_GOSUB_CODE;
    stub=read16(ram,caller,(uint16_t)(site+3u));
    if(ram[physical(stub,offset)]!=0xEAu || read16(ram,stub,(uint16_t)(offset+1u))!=target)
        return PR_U1C_GOSUB_CODE;
    push(ram,c,caller);push(ram,c,next);
    c->cs=read16(ram,stub,(uint16_t)(offset+3u));c->ip=target;
    d->callee=which;d->nested_callee=-1;d->nested_status=-1;
    if(which==PR_U1C_GOSUB_LOAD) {
        pr_u25_0000_cpu inner;memcpy(&inner,c,sizeof inner);
        status=(int)pr_u25_load_operands_0000(ram,size,&inner,limits->system_cs,
            limits->max_operands,limits->max_string_iterations);
        memcpy(c,&inner,sizeof inner);
    } else {
        pr_u25_1951_cpu inner;pr_u25_1951_callee nested;
        memcpy(&inner,c,sizeof inner);
        status=(int)pr_u25_alloc_1951_cpu(ram,size,&inner,limits->max_heap_steps,&nested,&d->nested_status);
        d->nested_callee=(int)nested;memcpy(c,&inner,sizeof inner);
    }
    d->status=status;
    if(status!=0)return PR_U1C_GOSUB_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U1C_GOSUB_TRANSFER;
    return PR_U1C_GOSUB_OK;
}
pr_u1c_gosub_status pr_u1c_gosub_0111_cpu(uint8_t *ram,size_t size,
    pr_u1c_gosub_cpu *c,const pr_u1c_gosub_limits *limits,
    pr_u1c_gosub_diagnostic *diagnostic)
{
    pr_u1c_gosub_diagnostic local={PR_U1C_GOSUB_NO_CALLEE,-1,-1,-1};
    pr_u1c_gosub_status status;size_t frame;
    if(diagnostic==NULL)diagnostic=&local;
    *diagnostic=local;
    if(ram==NULL || c==NULL || limits==NULL || size!=PR_U1C_GOSUB_RAM_SIZE ||
       c->ip!=0x0111u || c->sp<0x0232u || c->sp>0xFFFCu ||
       limits->max_operands==0u || limits->max_string_iterations==0u)return PR_U1C_GOSUB_ARGUMENT;
    push(ram,c,c->bp);c->bp=c->sp;                     /* +0111 PUSH BP; MOV BP,SP */
    al(c,1u);push(ram,c,c->ax);                       /* +0114 MOV AL,1; PUSH AX */
    status=call(ram,size,c,limits,diagnostic,0x0117u,0x002Au,0u,PR_U1C_GOSUB_LOAD);
    if(status!=0)return status;
    al(c,1u);push(ram,c,c->ax);                       /* +011C MOV AL,1; PUSH AX */
    status=call(ram,size,c,limits,diagnostic,0x011Fu,0x0075u,0x1951u,PR_U1C_GOSUB_ALLOC);
    if(status!=0)return status;
    c->bp=read16(ram,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u); /* +0124 POP BP */
    frame=physical(c->ss,c->sp);                      /* +0125 RETF */
    c->ip=word_at(ram,frame);c->cs=word_at(ram,(frame+2u)&MASK);
    c->sp=(uint16_t)(c->sp+4u);return PR_U1C_GOSUB_OK;
}
